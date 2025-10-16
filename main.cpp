#include "MainWindow.hpp"
#include <QApplication>
#include <QDebug>

// === DEBUG FLAGS ===
#define DEBUG_VISION       0
#define DEBUG_INTRO        0
#define DEBUG_CHECK        0
#define DEBUG_MENU         0
#define DEBUG_GAME         0
#define DEBUG_CALIBRATION  1

int main(int argc, char *argv[])
{
#if DEBUG_VISION
    Camera cam;
    cam.debugVision();
    return 0;
#else
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
    w.showCalibration();   // <--- nouvelle méthode
#else
    w.showIntro();
#endif

    w.show();
    return app.exec();
#endif
}
