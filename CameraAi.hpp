#ifndef CAMERAAI_HPP
#define CAMERAAI_HPP

#include <QObject>
#include <QImage>
#include <QThread>
#include <opencv2/opencv.hpp>
#undef slots
#include <torch/script.h>
#include <torch/torch.h>
#define slots Q_SLOTS

struct Detection {
    float x1, y1, x2, y2;
    float conf;
    int   cls;
};

class CameraAI : public QObject
{
    Q_OBJECT

public:
    explicit CameraAI(QObject* parent = nullptr);
    ~CameraAI();

    void start(int camIndex = 0);
    void stop();

signals:
    void frameReady(const QImage& img);

private slots:
    void processLoop();

private:
    std::vector<Detection> inferTorch(const cv::Mat& frame);
    QImage matToQImage(const cv::Mat& mat);

private:
    QThread        workerThread;
    cv::VideoCapture cap;
    bool           running = false;
    std::shared_ptr<torch::jit::Module> model;
};

#endif // CAMERAAI_HPP
