#ifndef CAMERAAI_HPP
#define CAMERAAI_HPP

#include <QObject>
#include <QImage>
#include <QThread>
#include <QVector>
#include <QMutex>
#include <QMutexLocker>
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

    void start(int camIndex = 0);
    void stop();

    // Renvoie 0 si OK et remplit 'out' (6x7, 0=vide, 1=rouge, 2=jaune).
    // Renvoie -1 si la grille n'est pas complète/fiable.
    int getGrille(QVector<QVector<int>>& out) const;

signals:
    void frameReady(const QImage& img);
    void gridUpdated(const QVector<QVector<int>>& grid); // optionnel si tu veux être notifié

private slots:
    void processLoop();

private:
    std::vector<Detection> inferTorch(const cv::Mat& frame);
    void updateGrid(const std::vector<Detection>& dets);
    QImage matToQImage(const cv::Mat& mat);

private:
    QThread        workerThread;
    cv::VideoCapture cap;
    bool           running = false;
    std::shared_ptr<torch::jit::Module> model;

    // --- Etat de la grille ---
    const int rows_ = 6;
    const int cols_ = 7;
    mutable QMutex gridMutex_;
    QVector<QVector<int>> grid_;   // 0=vide,1=rouge,2=jaune
    bool gridComplete_ = false;
};

#endif // CAMERAAI_HPP
