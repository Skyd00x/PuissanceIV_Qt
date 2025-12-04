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
    // --- Type réutilisable par d'autres classes ---
    using Grid = QVector<QVector<int>>;

    explicit CameraAI(QObject* parent = nullptr);
    ~CameraAI();

    static bool isAvailable();
    void loadModel();  // Charge le modèle TorchScript
    void start(int camIndex = 0);
    void stop();

    // Retourne la grille détectée
    int getGrille(Grid& out) const;

signals:
    void frameReady(const QImage& img);
    void gridUpdated(const Grid& g);
    void gridIncomplete(int detectedCount);  // Émis quand la grille n'est pas complète (pas 42 pions)
    void gridComplete();  // Émis quand la grille devient complète

private slots:
    void processLoop();

private:
    void initializeCamera(int camIndex);  // Initialise et démarre dans le workerThread
    cv::Mat extractBlueGrid(const cv::Mat& frame);  // Extrait la grille bleue de l'image
    std::vector<Detection> inferTorch(const cv::Mat& frame);
    void updateGrid(const std::vector<Detection>& dets);
    QImage matToQImage(const cv::Mat& mat);

    QThread            workerThread;
    cv::VideoCapture   cap;
    bool               running = false;
    std::shared_ptr<torch::jit::Module> model;

    static constexpr int rows_ = 6;
    static constexpr int cols_ = 7;

    mutable QMutex     gridMutex_;
    Grid               grid_;          // ← Grille utilisant le type Grid
    bool               gridComplete_ = false;
    int                incompleteCount_ = 0;  // Compteur de détections incomplètes consécutives
};
