#include "CameraAI.hpp"
#include <QDebug>
#include <QDir>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>

CameraAI::CameraAI(QObject* parent)
    : QObject(parent)
{
    connect(&workerThread, &QThread::started, this, &CameraAI::processLoop);
    moveToThread(&workerThread);

    // Initialisation de la grille à 0
    memset(grid, 0, sizeof(grid));
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

        QString basePath = QCoreApplication::applicationDirPath();
        QString inputPath  = basePath + "/input.jpg";
        QString outputPath = basePath + "/output.jpg";
        QString jsonPath   = basePath + "/grid.json";
        cv::imwrite(inputPath.toStdString(), frame);

        // Appel du script Python
        QProcess process;
        QString pythonExe = "python"; // ou chemin complet vers ton python.exe
        QString scriptPath = basePath + "/Model/detect_loader.py";

        qDebug() << "[AI] Execution:" << pythonExe << scriptPath;
        process.start(pythonExe, QStringList() << scriptPath << inputPath << outputPath << jsonPath);
        process.waitForFinished(-1);

        qDebug() << "[AI] Python output:" << process.readAllStandardOutput();
        qDebug() << "[AI] Python error:" << process.readAllStandardError();

        if (QFile::exists(outputPath)) {
            QImage annotated(outputPath);
            if (!annotated.isNull()) {
                emit frameReady(annotated);
            }
        }

        if (QFile::exists(jsonPath)) {
            loadGridFromJson(jsonPath);
        }

        QThread::msleep(100);
    }
}

void CameraAI::loadGridFromJson(const QString& jsonPath) {
    QFile file(jsonPath);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "[AI] Impossible d'ouvrir grid.json";
        return;
    }

    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) return;

    QJsonObject root = doc.object();
    QJsonArray rows = root["grid"].toArray();

    if (rows.size() != 6) return;

    for (int i = 0; i < 6; ++i) {
        QJsonArray cols = rows[i].toArray();
        if (cols.size() != 7) continue;
        for (int j = 0; j < 7; ++j) {
            grid[i][j] = cols[j].toInt(0);
        }
    }

    qDebug() << "[AI] Grille mise à jour :";
    for (int i = 0; i < 6; ++i) {
        QString line;
        for (int j = 0; j < 7; ++j)
            line += QString::number(grid[i][j]) + " ";
        qDebug().noquote() << " " << line;
    }
}

QImage CameraAI::matToQImage(const cv::Mat& mat) {
    if (mat.empty()) return {};
    cv::Mat rgb;
    cv::cvtColor(mat, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, rgb.step, QImage::Format_RGB888).copy();
}
