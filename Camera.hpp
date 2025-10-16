#pragma once
#include <QCoreApplication>
#include <QObject>
#include <QThread>
#include <QImage>
#include <QMutex>
#include <opencv2/opencv.hpp>
#include <vector>
#include <string>

/**
 * @brief Classe Camera — Capture, détection et analyse de la grille du jeu.
 *
 * Fonctions :
 *  - Capture vidéo multithreadée
 *  - Correction lumineuse et colorimétrique
 *  - Détection structurelle (grille + trous)
 *  - Classification couleur adaptative
 *  - Mode debug mosaïque (6 vues)
 */
class Camera : public QObject
{
    Q_OBJECT

public:
    explicit Camera(QObject *parent = nullptr);
    ~Camera();

    static bool isAvailable();
    void start();
    void stop();
    cv::Mat getFrame();

    /** Mode debug complet avec affichage mosaïque **/
    void debugVision();

signals:
    void frameReady(const QImage &frame);

private:
    // === Capture ===
    QThread workerThread;
    cv::VideoCapture cap;
    cv::Mat frame;
    QMutex frameMutex;
    bool running = false;

    void captureLoop();
    QImage matToQImage(const cv::Mat &mat);

    // === Prétraitement lumière / couleur ===
    cv::Mat preprocessLighting(const cv::Mat &input);

    // === Détection structurelle ===
    std::vector<cv::Vec3f> detectGridAndCircles(const cv::Mat &thresh, cv::Rect &gridRect);
    cv::Mat drawGridAndTokens(const cv::Mat &src, const cv::Rect &gridRect, const std::vector<cv::Vec3f> &circles);

    // === Classification couleur ===
    void classifyTokens(const cv::Mat &src, cv::Mat &dst, const std::vector<cv::Vec3f> &circles);
    cv::Vec3b getCircleMeanColor(const cv::Mat &image, cv::Vec3f circle);

    // === Calibration couleur ===
    void calibrateColors(const cv::Mat &frame, const std::vector<cv::Vec3f> &circles);
    void loadCalibration();
    void saveCalibration();

    // === Données de calibration ===
    struct ColorRange { int Hmin, Hmax, Smin, Smax, Vmin, Vmax; };
    ColorRange redRange, yellowRange, emptyRange;
    bool calibrated = false;
};
