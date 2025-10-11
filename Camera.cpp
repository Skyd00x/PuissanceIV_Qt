#include "Camera.hpp"
#include <QDebug>

Camera::Camera(QObject *parent)
    : QObject(parent)
{
    // On démarre le thread de travail
    moveToThread(&workerThread);

    // Quand le thread démarre, on appelle captureLoop() dans CE thread
    connect(&workerThread, &QThread::started, this, [this]() {
        qDebug() << "🎥 Capture thread démarré.";
        captureLoop();
    });

    // Quand le thread se termine, on libère la caméra
    connect(&workerThread, &QThread::finished, this, [this]() {
        if (cap.isOpened()) {
            cap.release();
            qDebug() << "📷 Caméra libérée.";
        }
    });
}

Camera::~Camera()
{
    stop();
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

    if (cap.isOpened()) {
        cap.release();
    }
}

cv::Mat Camera::getFrame()
{
    QMutexLocker locker(&frameMutex);
    return frame.clone();
}

void Camera::captureLoop()
{
    cap.open(0, cv::CAP_DSHOW); // Sélectionne la webcam 0

    if (!cap.isOpened()) {
        qWarning() << "❌ Erreur : impossible d'ouvrir la caméra.";
        running = false;
        return;
    }

    qDebug() << "✅ Caméra ouverte avec succès.";

    while (running) {
        cv::Mat temp;
        cap >> temp;

        if (temp.empty())
            continue;

        {
            QMutexLocker locker(&frameMutex);
            frame = temp.clone();
        }

        QImage img = matToQImage(temp);
        emit frameReady(img);

        QThread::msleep(30); // ~30 FPS
        QCoreApplication::processEvents(); // ✅ permet à Qt de respirer
    }

    cap.release();
    qDebug() << "🛑 Capture stoppée.";
}

QImage Camera::matToQImage(const cv::Mat &mat)
{
    if (mat.empty())
        return QImage();

    if (mat.type() == CV_8UC3) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_BGR888).copy();
    }
    else if (mat.type() == CV_8UC1) {
        return QImage(mat.data, mat.cols, mat.rows, mat.step, QImage::Format_Grayscale8).copy();
    }

    return QImage();
}
