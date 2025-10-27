#pragma once
#include <QObject>
#include <QThread>
#include <QImage>
#include <QProcess>
#include <opencv2/opencv.hpp>

class CameraAI : public QObject {
    Q_OBJECT
public:
    explicit CameraAI(QObject* parent = nullptr);
    ~CameraAI();

    void start(int camIndex = 0);
    void stop();

    int grid[6][7]; // 0 = vide, 1 = rouge, 2 = jaune

signals:
    void frameReady(const QImage& img);

private:
    void processLoop();
    QImage matToQImage(const cv::Mat& mat);
    void loadGridFromJson(const QString& jsonPath);

    QThread workerThread;
    cv::VideoCapture cap;
    bool running = false;
};
