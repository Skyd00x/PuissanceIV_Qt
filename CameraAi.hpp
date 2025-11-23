#pragma once

#include <QObject>
#include <QImage>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <opencv2/opencv.hpp>
#undef slots
#include <torch/script.h>
#include <torch/torch.h>
#define slots Q_SLOTS

struct Detection {
    float x1, y1, x2, y2;
    float conf;
    int   cls; // 0=r, 1=y, 2=e
};

class CameraAI : public QObject
{
    Q_OBJECT

public:
    explicit CameraAI(QObject* parent = nullptr);
    ~CameraAI();

    static bool isAvailable();
    void start(int camIndex = 0);
    void stop();
    int getGrille(QVector<QVector<int>>& out) const;

signals:
    void frameReady(const QImage& img);
    void gridUpdated(const QVector<QVector<int>>& g);

private slots:
    void processLoop();

private:
    std::vector<Detection> inferTorch(const cv::Mat& frame);
    void updateGrid(const std::vector<Detection>& dets);
    QImage matToQImage(const cv::Mat& mat);

    QThread        workerThread;
    cv::VideoCapture cap;
    bool           running = false;
    std::shared_ptr<torch::jit::Module> model;

    static constexpr int rows_ = 6;
    static constexpr int cols_ = 7;
    mutable QMutex          gridMutex_;
    QVector<QVector<int>>   grid_;
    bool gridComplete_ = false;
};
