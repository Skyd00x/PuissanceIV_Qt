#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QImage>
#include <opencv2/opencv.hpp>

class Camera : public QObject
{
    Q_OBJECT

public:
    explicit Camera(QObject *parent = nullptr);
    ~Camera();

    void start();
    void stop();

    cv::Mat getFrame();

signals:
    void frameReady(const QImage &frame);

private slots:
    void captureLoop();

private:
    QThread workerThread;
    QMutex frameMutex;

    cv::VideoCapture cap;
    cv::Mat frame;
    bool running = false;

    QImage matToQImage(const cv::Mat &mat);
};
