#pragma once

#include <QObject>
#include <QImage>
#include <QThread>
#include <opencv2/opencv.hpp>
#include <memory>
#include <vector>
#undef slots
#include <torch/script.h>
#include <torch/torch.h>
#define slots Q_SLOTS

// Structure de détection (coordonnées, confiance, classe)
struct Detection {
    float x1, y1, x2, y2, conf;
    int cls;
};

class CameraAI : public QObject {
    Q_OBJECT

public:
    explicit CameraAI(QObject* parent = nullptr);
    ~CameraAI();

    void start(int camIndex);
    void stop();

signals:
    void frameReady(const QImage& frame);

private:
    void processLoop();
    std::vector<Detection> inferTorch(const cv::Mat& frame);
    QImage matToQImage(const cv::Mat& mat);

private:
    cv::VideoCapture cap;
    QThread workerThread;
    bool running;

    // === Modèle TorchScript (pointeur partagé pour sécurité) ===
    std::shared_ptr<torch::jit::Module> model;

    // Exemple de structure interne (grille, si utilisée ailleurs)
    int grid[6][7];
};
