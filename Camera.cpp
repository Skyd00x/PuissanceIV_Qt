#include "Camera.hpp"
#include <QDebug>

Camera::Camera(QObject *parent)
    : QObject(parent)
{
    moveToThread(&workerThread);

    connect(&workerThread, &QThread::started, this, &Camera::captureLoop);
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
    cap.open(0); // Choisis la webcam 0 par défaut

    if (!cap.isOpened()) {
        qWarning() << "Erreur : impossible d'ouvrir la caméra.";
        running = false;
        return;
    }

    while (running) {
        cv::Mat temp;
        cap >> temp;
        if (temp.empty()) continue;

        {
            QMutexLocker locker(&frameMutex);
            frame = temp.clone();
        }

        // Conversion vers QImage pour affichage immédiat
        QImage img = matToQImage(temp);
        emit frameReady(img);

        QThread::msleep(30); // ~33 FPS
    }

    cap.release();
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
