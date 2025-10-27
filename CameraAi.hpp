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

signals:
    void frameReady(const QImage& img);

private:
    void processLoop();
    QImage matToQImage(const cv::Mat& mat);

    QThread workerThread;
    cv::VideoCapture cap;
    bool running = false;
};
