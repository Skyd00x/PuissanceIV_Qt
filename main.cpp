#include "MainWindow.hpp"
#include "CameraAi.hpp"
#include <QApplication>
#include <QDebug>

// === DEBUG FLAGS ===
#define DEBUG_VISION       0   // Mode debug de la vision classique
#define DEBUG_VISIONAI     1   // Mode debug IA (ONNX)
#define DEBUG_INTRO        0
#define DEBUG_CHECK        0
#define DEBUG_MENU         0
#define DEBUG_GAME         0
#define DEBUG_CALIBRATION  0

int main(int argc, char *argv[])
{
#if DEBUG_VISIONAI
    QApplication app(argc, argv);
    QLabel label;
    label.show();

    CameraAI ai;
    ai.start(0);

    QObject::connect(&ai, &CameraAI::frameReady, &label, [&](const QImage& img){
        label.setPixmap(QPixmap::fromImage(img).scaled(1280, 720, Qt::KeepAspectRatio));
    }, Qt::QueuedConnection);

    return app.exec();
#elif DEBUG_VISION
    // === MODE DEBUG VISION CLASSIQUE ===
    Camera cam;
    cam.debugVision();
    return 0;

#else
    // === MODE APPLICATION COMPLÈTE ===
    QApplication app(argc, argv);
    MainWindow w;

#if DEBUG_INTRO
    w.setDebugMode(true);
    w.showIntro();
#elif DEBUG_CHECK
    w.setDebugMode(true);
    w.showCheck();
#elif DEBUG_MENU
    w.setDebugMode(true);
    w.showMenu();
#elif DEBUG_GAME
    w.setDebugMode(true);
    w.showGame();
#elif DEBUG_CALIBRATION
    w.setDebugMode(true);
    w.showCalibration();
#else
    w.showIntro();
#endif

    w.show();
    return app.exec();
#endif
}
