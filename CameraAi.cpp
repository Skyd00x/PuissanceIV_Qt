#include "CameraAI.hpp"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>

CameraAI::CameraAI(QObject* parent)
    : QObject(parent)
{
    connect(&workerThread, &QThread::started, this, &CameraAI::processLoop);
    moveToThread(&workerThread);
}

CameraAI::~CameraAI() {
    stop();
}

void CameraAI::start(int camIndex) {
    if (!cap.open(camIndex, cv::CAP_DSHOW)) {
        qWarning() << "[AI] Impossible d'ouvrir la webcam";
        return;
    }
    running = true;
    workerThread.start();
    qDebug() << "[AI] Capture démarrée (Python)";
}

void CameraAI::stop() {
    running = false;
    if (workerThread.isRunning()) {
        workerThread.quit();
        workerThread.wait();
    }
    if (cap.isOpened()) cap.release();
    qDebug() << "[AI] Capture arrêtée (Python)";
}

void CameraAI::processLoop() {
    cv::Mat frame;
    while (running) {
        cap >> frame;
        if (frame.empty()) continue;

        // Sauvegarde frame temporaire
        QString inputPath  = QCoreApplication::applicationDirPath() + "/input.jpg";
        QString outputPath = QCoreApplication::applicationDirPath() + "/output.jpg";
        cv::imwrite(inputPath.toStdString(), frame);

        // Appel Python
        QProcess process;
        QString pythonExe = "python"; // ou chemin complet vers ton python.exe
        QString scriptPath = QCoreApplication::applicationDirPath() + "/Model/detect_loader.py";
        qDebug() << "[AI] Execution:" << pythonExe << scriptPath << inputPath << outputPath;
        process.start(pythonExe, QStringList() << scriptPath << inputPath << outputPath);
        process.waitForFinished(-1);
        qDebug() << "[AI] Python output:" << process.readAllStandardOutput();
        qDebug() << "[AI] Python error:" << process.readAllStandardError();


        if (!QFile::exists(outputPath)) {
            qWarning() << "[AI] Aucune sortie Python trouvée";
            continue;
        }

        QImage annotated(outputPath);
        if (!annotated.isNull()) {
            emit frameReady(annotated);
        }

        QThread::msleep(100); // environ 10 FPS
    }
}

QImage CameraAI::matToQImage(const cv::Mat& mat) {
    if (mat.empty()) return {};
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}
