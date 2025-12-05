#include "CameraAI.hpp"

#include <QCoreApplication>
#include <QDebug>
#include <QDir>
#include <filesystem>
#include <algorithm>
#include <cmath>

#undef slots
#include <torch/script.h>
#define slots Q_SLOTS

CameraAI::CameraAI(QObject* parent)
    : QObject(parent),
    running(false),
    grid_(rows_, QVector<int>(cols_, 0)),
    gridComplete_(false)
{
    // Déplace cet objet dans le workerThread
    // IMPORTANT: ne fonctionne que si parent == nullptr
    moveToThread(&workerThread);
}

CameraAI::~CameraAI() {
    stop();
}

void CameraAI::loadModel()
{
    QString modelPath = QCoreApplication::applicationDirPath() + "/Model/model.torchscript";
    qDebug() << "[AI] Chargement du modèle :" << modelPath;

    std::filesystem::path fsPath = modelPath.toStdWString();
    if (!std::filesystem::exists(fsPath)) {
        qWarning() << "[AI] ⚠️ Modèle introuvable:" << modelPath;
        return;
    }

    try {
        model = std::make_shared<torch::jit::Module>(torch::jit::load(fsPath.string(), torch::kCPU));
        model->eval();
        qDebug() << "[AI] ✅ Modèle YOLOv8 TorchScript chargé (CPU)";
    }
    catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Erreur de chargement:" << e.what();
    }
}

bool CameraAI::isAvailable()
{
    cv::VideoCapture testCap(0, cv::CAP_DSHOW);
    bool ok = testCap.isOpened();
    if (ok) testCap.release();
    return ok;
}

void CameraAI::start(int camIndex)
{
    running = true;

    if (!workerThread.isRunning()) {
        // Connecte une seule fois au démarrage du thread
        connect(&workerThread, &QThread::started, this, [this, camIndex]() {
            initializeCamera(camIndex);
        }, Qt::DirectConnection);

        workerThread.start();
    } else {
        // Thread déjà démarré, lance l'initialisation directement
        QMetaObject::invokeMethod(this, [this, camIndex]() {
            initializeCamera(camIndex);
        }, Qt::QueuedConnection);
    }
}

void CameraAI::initializeCamera(int camIndex)
{
    // Cette méthode s'exécute dans le workerThread

    // Charge le modèle si pas encore fait
    if (!model) {
        qDebug() << "[AI] Chargement du modèle dans le workerThread...";
        loadModel();
    }

    // Ouvre la caméra
    if (!cap.open(camIndex, cv::CAP_DSHOW)) {
        qWarning() << "[AI] ❌ Impossible d'ouvrir la caméra (index:" << camIndex << ")";
        running = false;
        return;
    }
    qDebug() << "[AI] 🚀 Capture démarrée";

    // Démarre la boucle de traitement
    processLoop();
}

void CameraAI::stop()
{
    running = false;

    if (workerThread.isRunning()) {
        workerThread.quit();
        workerThread.wait();
    }

    if (cap.isOpened())
        cap.release();

    // Réinitialiser le compteur de détections incomplètes
    incompleteCount_ = 0;

    qDebug() << "[AI] 🛑 Capture arrêtée";
}

int CameraAI::getGrille(Grid& out) const
{
    QMutexLocker lock(&gridMutex_);
    if (!gridComplete_) return -1;
    out = grid_;
    return 0;
}

void CameraAI::processLoop()
{
    try {
        cv::Mat frame;

        while (running) {
            cap >> frame;
            if (frame.empty())
                continue;

            // Effectuer l'inférence directement sur la frame complète (sans cropping)
            auto dets = inferTorch(frame);
            updateGrid(dets);

            // Afficher la frame complète avec les détections
            emit frameReady(matToQImage(frame));

            QThread::msleep(30);
        }
    }
    catch (const std::exception& e) {
        qWarning() << "[AI] ❌ Exception:" << e.what();
    }
}

cv::Mat CameraAI::extractBlueGrid(const cv::Mat& frame)
{
    if (frame.empty())
        return frame;

    // 1. Conversion en HSV pour détecter la couleur bleue
    cv::Mat hsv;
    cv::cvtColor(frame, hsv, cv::COLOR_BGR2HSV);

    // 2. Définir les seuils pour la couleur bleue (teinte H ~100-130)
    // Ces valeurs peuvent nécessiter un ajustement selon l'éclairage
    cv::Scalar lowerBlue(90, 50, 50);     // H, S, V minimums
    cv::Scalar upperBlue(130, 255, 255);  // H, S, V maximums

    // 3. Créer un masque binaire pour la couleur bleue
    cv::Mat mask;
    cv::inRange(hsv, lowerBlue, upperBlue, mask);

    // 4. Opérations morphologiques pour nettoyer le masque
    cv::Mat kernel = cv::getStructuringElement(cv::MORPH_RECT, cv::Size(5, 5));
    cv::morphologyEx(mask, mask, cv::MORPH_CLOSE, kernel);  // Fermer les trous
    cv::morphologyEx(mask, mask, cv::MORPH_OPEN, kernel);   // Enlever le bruit

    // 5. Trouver les contours
    std::vector<std::vector<cv::Point>> contours;
    cv::findContours(mask, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    if (contours.empty())
        return frame;  // Pas de grille détectée, retourner l'image originale

    // 6. Trouver le plus grand contour (qui devrait être la grille)
    double maxArea = 0;
    int maxIdx = -1;
    for (size_t i = 0; i < contours.size(); ++i) {
        double area = cv::contourArea(contours[i]);
        if (area > maxArea) {
            maxArea = area;
            maxIdx = (int)i;
        }
    }

    if (maxIdx == -1 || maxArea < 10000)  // Seuil minimum de taille pour éviter les faux positifs
        return frame;

    // 7. Obtenir le rectangle englobant
    cv::Rect boundingBox = cv::boundingRect(contours[maxIdx]);

    // 8. Ajouter une petite marge (5%) pour s'assurer de capturer toute la grille
    int margin = std::min(boundingBox.width, boundingBox.height) * 0.05;
    boundingBox.x = std::max(0, boundingBox.x - margin);
    boundingBox.y = std::max(0, boundingBox.y - margin);
    boundingBox.width = std::min(frame.cols - boundingBox.x, boundingBox.width + 2 * margin);
    boundingBox.height = std::min(frame.rows - boundingBox.y, boundingBox.height + 2 * margin);

    // 9. Extraire la région d'intérêt (crop)
    cv::Mat gridROI = frame(boundingBox).clone();

    // 10. Augmenter fortement la saturation pour renforcer les couleurs
    cv::Mat hsvROI;
    cv::cvtColor(gridROI, hsvROI, cv::COLOR_BGR2HSV);

    // Séparer les canaux H, S, V
    std::vector<cv::Mat> hsvChannels;
    cv::split(hsvROI, hsvChannels);

    // Augmenter la saturation (S) de 100% (doublement) pour rendre les couleurs très vives
    hsvChannels[1] = hsvChannels[1] * 2.0;
    cv::threshold(hsvChannels[1], hsvChannels[1], 255, 255, cv::THRESH_TRUNC);

    // Recombiner les canaux
    cv::merge(hsvChannels, hsvROI);

    // Reconvertir en BGR
    cv::Mat result;
    cv::cvtColor(hsvROI, result, cv::COLOR_HSV2BGR);

    return result;
}

std::vector<Detection> CameraAI::inferTorch(const cv::Mat& frameBGR)
{
    std::vector<Detection> results;
    if (!model || frameBGR.empty())
        return results;

    const int imgsz = 640;
    const cv::Scalar padColor(114, 114, 114);

    int h0 = frameBGR.rows, w0 = frameBGR.cols;

    float gain = std::min((float)imgsz / h0, (float)imgsz / w0);
    int newW = int(std::round(w0 * gain));
    int newH = int(std::round(h0 * gain));
    int padW = imgsz - newW;
    int padH = imgsz - newH;
    int dw = padW / 2;
    int dh = padH / 2;

    cv::Mat resized, lb(imgsz, imgsz, CV_8UC3, padColor);
    cv::resize(frameBGR, resized, {newW, newH});
    resized.copyTo(lb(cv::Rect(dw, dh, newW, newH)));

    cv::cvtColor(lb, lb, cv::COLOR_BGR2RGB);
    lb.convertTo(lb, CV_32F, 1.0 / 255.0);

    at::Tensor input = torch::from_blob(lb.data, {1, imgsz, imgsz, 3}, at::kFloat)
                           .permute({0, 3, 1, 2}).contiguous();

    torch::NoGradGuard noGrad;
    at::Tensor out;
    try {
        out = model->forward({input}).toTensor();
    }
    catch (const c10::Error& e) {
        qWarning() << "[AI] ❌ Erreur inférence:" << e.what();
        return results;
    }

    out = out.squeeze(0).permute({1, 0});
    int64_t nc = out.size(1) - 4;

    at::Tensor boxes_xywh = out.slice(1, 0, 4);
    at::Tensor cls_scores = out.slice(1, 4, 4 + nc);

    at::Tensor conf, labels;
    std::tie(conf, labels) = cls_scores.max(1);

    const float confTh = 0.1f;  // Filtrer les prédictions < 90% de confiance
    at::Tensor keep = conf > confTh;
    boxes_xywh = boxes_xywh.index({keep});
    conf = conf.index({keep});
    labels = labels.index({keep});

    auto x = boxes_xywh.select(1, 0);
    auto y = boxes_xywh.select(1, 1);
    auto w = boxes_xywh.select(1, 2);
    auto h = boxes_xywh.select(1, 3);

    at::Tensor xyxy = at::stack({x - w * 0.5f, y - h * 0.5f, x + w * 0.5f, y + h * 0.5f}, 1).to(torch::kCPU);

    conf = conf.to(torch::kCPU);
    labels = labels.to(torch::kCPU);

    const float iouTh = 0.5f;
    std::vector<cv::Rect> finalBoxes;
    std::vector<float>    finalScores;
    std::vector<int>      finalClasses;

    for (int c = 0; c < nc; ++c) {
        std::vector<cv::Rect> boxesC;
        std::vector<float> scoresC;

        for (int i = 0; i < xyxy.size(0); ++i) {
            if (labels[i].item<int>() != c)
                continue;

            float x1 = xyxy[i][0].item<float>();
            float y1 = xyxy[i][1].item<float>();
            float x2 = xyxy[i][2].item<float>();
            float y2 = xyxy[i][3].item<float>();

            boxesC.emplace_back(cv::Point((int)x1, (int)y1), cv::Point((int)x2, (int)y2));
            scoresC.push_back(conf[i].item<float>());
        }

        if (boxesC.empty())
            continue;

        std::vector<int> idxs;
        cv::dnn::NMSBoxes(boxesC, scoresC, confTh, iouTh, idxs);

        for (int id : idxs) {
            finalBoxes.push_back(boxesC[id]);
            finalScores.push_back(scoresC[id]);
            finalClasses.push_back(c);
        }
    }

    for (size_t i = 0; i < finalBoxes.size(); ++i) {
        float x1 = (finalBoxes[i].x - dw) / gain;
        float y1 = (finalBoxes[i].y - dh) / gain;
        float x2 = (finalBoxes[i].x + finalBoxes[i].width  - dw) / gain;
        float y2 = (finalBoxes[i].y + finalBoxes[i].height - dh) / gain;

        x1 = std::clamp(x1, 0.f, (float)w0);
        y1 = std::clamp(y1, 0.f, (float)h0);
        x2 = std::clamp(x2, 0.f, (float)w0);
        y2 = std::clamp(y2, 0.f, (float)h0);

        finalBoxes[i] = cv::Rect(cv::Point((int)x1, (int)y1), cv::Point((int)x2, (int)y2));
    }

    static const std::vector<std::string> names = {"r", "y", "e"};

    for (size_t i = 0; i < finalBoxes.size(); ++i) {
        int cls = finalClasses[i];
        float sc = finalScores[i];

        results.push_back({
            (float)finalBoxes[i].x,
            (float)finalBoxes[i].y,
            (float)(finalBoxes[i].x + finalBoxes[i].width),
            (float)(finalBoxes[i].y + finalBoxes[i].height),
            sc, cls
        });

        cv::Scalar color =
            (cls == 0 ? cv::Scalar(0, 0, 255) :
                 cls == 1 ? cv::Scalar(0, 255, 255) :
                 cv::Scalar(255, 255, 255));

        cv::rectangle((cv::Mat&)frameBGR, finalBoxes[i], color, 2);

        char txt[64];
        std::snprintf(txt, sizeof(txt), "%s (%.2f)", names[cls].c_str(), sc);

        int base;
        auto sz = cv::getTextSize(txt, cv::FONT_HERSHEY_SIMPLEX, 0.6, 2, &base);

        cv::rectangle((cv::Mat&)frameBGR,
                      cv::Rect(finalBoxes[i].x,
                               std::max(0, finalBoxes[i].y - sz.height - 6),
                               sz.width + 6, sz.height + 6),
                      color, cv::FILLED);

        cv::putText((cv::Mat&)frameBGR, txt,
                    {finalBoxes[i].x + 3, finalBoxes[i].y - 3},
                    cv::FONT_HERSHEY_SIMPLEX, 0.6,
                    cv::Scalar(0, 0, 0), 2);
    }

    return results;
}

void CameraAI::updateGrid(const std::vector<Detection>& dets)
{
    if ((int)dets.size() != rows_ * cols_) {
        // Grille incomplète - démarrer le timer si pas déjà démarré
        if (!incompleteTimerStarted_) {
            incompleteTimerStarted_ = true;
            incompleteStartTime_ = std::chrono::steady_clock::now();
            incompleteCount_++;
        } else {
            // Timer déjà démarré, vérifier si 5 secondes se sont écoulées
            auto now = std::chrono::steady_clock::now();
            auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(now - incompleteStartTime_).count();

            if (elapsed >= 5) {
                // 5 secondes écoulées, émettre le signal
                emit gridIncomplete((int)dets.size());
            }
            incompleteCount_++;
        }
        return;
    }

    // La grille est complète - réinitialiser le timer et émettre le signal si elle était incomplète avant
    if (incompleteTimerStarted_) {
        incompleteTimerStarted_ = false;
        incompleteCount_ = 0;
        emit gridComplete();
    }

    struct Cell { float cx, cy; int val; };
    std::vector<Cell> cells;
    cells.reserve(dets.size());

    auto mapVal = [](int cls) {
        if (cls == 2) return 0;
        if (cls == 0) return 1;
        return 2;
    };

    for (const auto& d : dets) {
        float cx = 0.5f * (d.x1 + d.x2);
        float cy = 0.5f * (d.y1 + d.y2);
        cells.push_back({cx, cy, mapVal(d.cls)});
    }

    std::sort(cells.begin(), cells.end(),
              [](auto& a, auto& b) { return a.cy < b.cy; });

    Grid newGrid(rows_, QVector<int>(cols_, 0));
    bool ok = true;

    for (int r = 0; r < rows_; ++r) {
        int start = r * cols_;
        int end = start + cols_;

        if (end > (int)cells.size()) { ok = false; break; }

        std::sort(cells.begin() + start, cells.begin() + end,
                  [](auto& a, auto& b) { return a.cx < b.cx; });

        for (int c = 0; c < cols_; ++c)
            newGrid[r][c] = cells[start + c].val;
    }

    // Validation : un pion ne peut pas flotter dans l'air
    // Il doit avoir un support en dessous (ou être sur la ligne du bas)
    for (int r = 0; r < rows_ - 1; ++r) {  // Pas besoin de vérifier la dernière ligne
        for (int c = 0; c < cols_; ++c) {
            // Si c'est un pion (rouge=1 ou jaune=2)
            if (newGrid[r][c] != 0) {
                // Vérifier qu'il y a un support en dessous (ligne r+1)
                if (newGrid[r + 1][c] == 0) {
                    // Pas de support → ignorer ce pion (défaillance du modèle)
                    newGrid[r][c] = 0;
                }
            }
        }
    }

    QMutexLocker lock(&gridMutex_);
    grid_ = std::move(newGrid);
    gridComplete_ = ok;

    if (ok)
        emit gridUpdated(grid_);
}

QImage CameraAI::matToQImage(const cv::Mat& mat)
{
    if (mat.empty())
        return QImage();

    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);

    return QImage(rgb.data, rgb.cols, rgb.rows,
                  rgb.step, QImage::Format_RGB888).copy();
}
