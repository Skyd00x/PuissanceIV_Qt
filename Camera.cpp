#include "Camera.hpp"
#include <QDebug>
#include <numeric>
#include <fstream>

// ======================================================
// ================ CONSTRUCTEUR / DESTRUCTEUR ===========
// ======================================================
Camera::Camera(QObject *parent)
    : QObject(parent)
{
    moveToThread(&workerThread);
    connect(&workerThread, &QThread::started, this, [this]() {
        qDebug() << "Capture thread démarré.";
        captureLoop();
    });
    connect(&workerThread, &QThread::finished, this, [this]() {
        if (cap.isOpened()) cap.release();
        qDebug() << "Caméra libérée.";
    });

    loadCalibration();
}

Camera::~Camera() { stop(); }

bool Camera::isAvailable()
{
    cv::VideoCapture testCap(0, cv::CAP_DSHOW);
    bool ok = testCap.isOpened();
    if (ok) testCap.release();
    return ok;
}

void Camera::start()
{
    if (running) return;
    running = true;
    workerThread.start();
}

void Camera::stop()
{
    running = false;
    if (workerThread.isRunning()) {
        workerThread.quit();
        workerThread.wait();
    }
    if (cap.isOpened()) cap.release();
}

cv::Mat Camera::getFrame()
{
    QMutexLocker locker(&frameMutex);
    return frame.clone();
}

QImage Camera::matToQImage(const cv::Mat &mat)
{
    if (mat.empty()) return QImage();
    if (mat.type() == CV_8UC3)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888).copy();
    else if (mat.type() == CV_8UC1)
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    return QImage();
}

// ======================================================
// ====================== CAPTURE ========================
// ======================================================
void Camera::captureLoop()
{
    cap.open(0, cv::CAP_DSHOW);
    if (!cap.isOpened()) {
        qWarning() << "Erreur : impossible d'ouvrir la caméra.";
        running = false;
        return;
    }

    qDebug() << "Caméra ouverte avec succès.";

    while (running) {
        cv::Mat temp;
        cap >> temp;
        if (temp.empty()) continue;

        {
            QMutexLocker locker(&frameMutex);
            frame = temp.clone();
        }

        QImage img = matToQImage(temp);
        emit frameReady(img);

        QThread::msleep(30);
        QCoreApplication::processEvents();
    }

    cap.release();
    qDebug() << "Capture stoppée.";
}

// ======================================================
// ================== MODE DEBUG MOSAIQUE ================
// ======================================================
void Camera::debugVision()
{
    cv::VideoCapture debugCap(0, cv::CAP_DSHOW);
    if (!debugCap.isOpened()) {
        std::cerr << "Impossible d'ouvrir la caméra pour le mode debug." << std::endl;
        return;
    }

    std::cout << "[DEBUG] Mode vision robuste calibré. Appuie sur ÉCHAP pour quitter." << std::endl;

    while (true) {
        cv::Mat frame;
        debugCap >> frame;
        if (frame.empty()) continue;

        // === Prétraitement ===
        cv::Mat corrected = preprocessLighting(frame);

        // === Conversion et seuillage ===
        cv::Mat gray, thresh;
        cv::cvtColor(corrected, gray, cv::COLOR_BGR2GRAY);
        cv::adaptiveThreshold(gray, thresh, 255,
                              cv::ADAPTIVE_THRESH_GAUSSIAN_C,
                              cv::THRESH_BINARY_INV, 41, 5);

        // === Détection grille / trous ===
        cv::Rect gridRect;
        std::vector<cv::Vec3f> circles = detectGridAndCircles(thresh, gridRect);

        // === Calibration auto (si pas encore faite) ===
        if (!calibrated && !circles.empty()) {
            calibrateColors(corrected, circles);
            calibrated = true;
            saveCalibration();
            std::cout << "[INFO] Calibration couleur effectuée." << std::endl;
        }

        // === Classification et dessin ===
        cv::Mat classified = corrected.clone();
        classifyTokens(corrected, classified, circles);
        cv::Mat gridView = drawGridAndTokens(classified, gridRect, circles);

        // === Debug visuel ===
        cv::Mat edges;
        cv::Canny(gray, edges, 60, 150);

        // Redimensionnement
        cv::resize(frame, frame, cv::Size(480,270));
        cv::resize(gray, gray, cv::Size(480,270));
        cv::resize(thresh, thresh, cv::Size(480,270));
        cv::resize(edges, edges, cv::Size(480,270));
        cv::resize(gridView, gridView, cv::Size(480,270));

        // Conversion couleurs
        cv::cvtColor(gray, gray, cv::COLOR_GRAY2BGR);
        cv::cvtColor(thresh, thresh, cv::COLOR_GRAY2BGR);
        cv::cvtColor(edges, edges, cv::COLOR_GRAY2BGR);

        // Mosaïque
        cv::Mat top, bottom, empty = cv::Mat::zeros(gridView.size(), gridView.type());
        cv::hconcat(frame, gray, top);
        cv::hconcat(thresh, edges, bottom);
        cv::Mat middle;
        cv::vconcat(gridView, empty, middle);
        cv::Mat leftRight, combined;
        cv::vconcat(top, bottom, leftRight);
        cv::hconcat(leftRight, middle, combined);

        cv::imshow("Debug Vision Pipeline", combined);
        int key = cv::waitKey(1);
        if (key == 27) break; // ESC
    }

    debugCap.release();
    cv::destroyAllWindows();
}

// ======================================================
// =========== PRETRAITEMENT / LUMIERE ===================
// ======================================================
cv::Mat Camera::preprocessLighting(const cv::Mat &input)
{
    cv::Mat lab, balanced, result;
    cv::cvtColor(input, lab, cv::COLOR_BGR2Lab);
    std::vector<cv::Mat> lab_planes;
    cv::split(lab, lab_planes);
    cv::Ptr<cv::CLAHE> clahe = cv::createCLAHE(3.0, cv::Size(8,8));
    clahe->apply(lab_planes[0], lab_planes[0]);
    cv::merge(lab_planes, lab);
    cv::cvtColor(lab, balanced, cv::COLOR_Lab2BGR);

    cv::Mat gray;
    cv::cvtColor(balanced, gray, cv::COLOR_BGR2GRAY);
    double meanVal = cv::mean(gray)[0];
    double gamma = (meanVal < 80) ? 1.5 : (meanVal > 180 ? 0.7 : 1.0);

    cv::Mat lut(1,256,CV_8U);
    for(int i=0;i<256;i++)
        lut.at<uchar>(i)=cv::saturate_cast<uchar>(pow(i/255.0,gamma)*255.0);
    cv::LUT(balanced,lut,result);

    return result;
}

// ======================================================
// =========== DETECTION STRUCTURELLE ====================
// ======================================================
std::vector<cv::Vec3f> Camera::detectGridAndCircles(const cv::Mat &thresh, cv::Rect &gridRect)
{
    std::vector<cv::Vec3f> circles;
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(thresh, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    gridRect = cv::Rect();
    double maxArea = 0;
    for (auto &c : contours) {
        double area = cv::contourArea(c);
        if (area < 5000) continue;
        cv::Rect r = cv::boundingRect(c);
        if (r.area() > maxArea) {
            maxArea = r.area();
            gridRect = r;
        }
    }

    cv::Mat roi = (gridRect.area() > 0) ? thresh(gridRect).clone() : thresh.clone();
    std::vector<std::vector<cv::Point>> innerContours;
    cv::findContours(roi, innerContours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    for (auto &c : innerContours) {
        float radius;
        cv::Point2f center;
        cv::minEnclosingCircle(c, center, radius);
        double area = cv::contourArea(c);
        double circleArea = CV_PI * radius * radius;
        if (radius < 8 || radius > 60) continue;
        if (area / circleArea < 0.65) continue;
        if (gridRect.area() > 0) {
            center.x += gridRect.x;
            center.y += gridRect.y;
        }
        circles.push_back(cv::Vec3f(center.x, center.y, radius));
    }

    return circles;
}

cv::Mat Camera::drawGridAndTokens(const cv::Mat &src, const cv::Rect &gridRect, const std::vector<cv::Vec3f> &circles)
{
    cv::Mat out = src.clone();
    if (gridRect.area() > 0)
        cv::rectangle(out, gridRect, cv::Scalar(0,255,0), 2);

    for (auto &c : circles)
        cv::circle(out, cv::Point(c[0], c[1]), c[2], cv::Scalar(255,0,255), 1);

    return out;
}

// ======================================================
// ======= CLASSIFICATION COULEUR ADAPTATIVE ============
// ======================================================
void Camera::classifyTokens(const cv::Mat &src, cv::Mat &dst, const std::vector<cv::Vec3f> &circles)
{
    if (circles.empty()) return;
    for (auto &c : circles) {
        cv::Vec3b meanColor = getCircleMeanColor(src, c);
        cv::Mat sample(1,1,CV_8UC3, meanColor);
        cv::Mat hsvC; cv::cvtColor(sample, hsvC, cv::COLOR_BGR2HSV);
        cv::Vec3b h = hsvC.at<cv::Vec3b>(0,0);
        int H=h[0], S=h[1], V=h[2];
        std::string color="EMPTY";

        auto inRange = [&](const ColorRange &r){
            return (H>=r.Hmin && H<=r.Hmax &&
                    S>=r.Smin && S<=r.Smax &&
                    V>=r.Vmin && V<=r.Vmax);
        };

        if (inRange(redRange)) color="RED";
        else if (inRange(yellowRange)) color="YELLOW";

        cv::Scalar draw = (color=="RED")?cv::Scalar(0,0,255):
                              (color=="YELLOW")?cv::Scalar(0,255,255):
                              cv::Scalar(80,80,80);
        cv::circle(dst, cv::Point(c[0],c[1]), c[2]-6, draw, -1);
    }
}

cv::Vec3b Camera::getCircleMeanColor(const cv::Mat &image, cv::Vec3f circle)
{
    cv::Mat mask = cv::Mat::zeros(image.size(), CV_8UC1);
    cv::circle(mask, cv::Point(circle[0], circle[1]), circle[2]-5, cv::Scalar(255), -1);
    cv::Scalar mean = cv::mean(image, mask);
    return cv::Vec3b(mean[0], mean[1], mean[2]);
}

// ======================================================
// ============== CALIBRATION COULEURS ==================
// ======================================================
void Camera::calibrateColors(const cv::Mat &frame, const std::vector<cv::Vec3f> &circles)
{
    std::vector<cv::Vec3b> hsvSamples;
    for (auto &c : circles) {
        cv::Vec3b meanColor = getCircleMeanColor(frame, c);
        cv::Mat s(1,1,CV_8UC3, meanColor);
        cv::Mat hsv; cv::cvtColor(s, hsv, cv::COLOR_BGR2HSV);
        hsvSamples.push_back(hsv.at<cv::Vec3b>(0,0));
    }

    auto computeRange = [&](const std::vector<cv::Vec3b>&samples, int Hmin,int Hmax){
        ColorRange r; r.Hmin=0;r.Hmax=180;r.Smin=0;r.Smax=255;r.Vmin=0;r.Vmax=255;
        if(samples.empty()) return r;
        std::vector<int> H,S,V;
        for(auto &p:samples){H.push_back(p[0]);S.push_back(p[1]);V.push_back(p[2]);}
        auto minmaxH = std::minmax_element(H.begin(),H.end());
        auto minmaxS = std::minmax_element(S.begin(),S.end());
        auto minmaxV = std::minmax_element(V.begin(),V.end());
        r.Hmin=*minmaxH.first; r.Hmax=*minmaxH.second;
        r.Smin=*minmaxS.first; r.Smax=*minmaxS.second;
        r.Vmin=*minmaxV.first; r.Vmax=*minmaxV.second;
        return r;
    };

    // Rough presets
    redRange = {0,15,60,255,40,255};
    yellowRange = {20,45,60,255,40,255};
    emptyRange = {80,130,0,120,0,255};
}

void Camera::loadCalibration()
{
    std::ifstream f("color_calib.yml");
    if(!f.good()) return;
    cv::FileStorage fs("color_calib.yml", cv::FileStorage::READ);
    fs["redRange"] >> *(cv::Vec<int,6>*)&redRange;
    fs["yellowRange"] >> *(cv::Vec<int,6>*)&yellowRange;
    fs["emptyRange"] >> *(cv::Vec<int,6>*)&emptyRange;
    calibrated=true;
}

void Camera::saveCalibration()
{
    cv::FileStorage fs("color_calib.yml", cv::FileStorage::WRITE);
    fs << "redRange" << cv::Vec<int,6>(redRange.Hmin,redRange.Hmax,redRange.Smin,redRange.Smax,redRange.Vmin,redRange.Vmax);
    fs << "yellowRange" << cv::Vec<int,6>(yellowRange.Hmin,yellowRange.Hmax,yellowRange.Smin,yellowRange.Smax,yellowRange.Vmin,yellowRange.Vmax);
    fs << "emptyRange" << cv::Vec<int,6>(emptyRange.Hmin,emptyRange.Hmax,emptyRange.Smin,emptyRange.Smax,emptyRange.Vmin,emptyRange.Vmax);
}
