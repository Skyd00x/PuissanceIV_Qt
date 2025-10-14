#include "MainWindow.hpp"
#include <QApplication>
#include <QDebug>

// === DEBUG FLAGS ===
// Active un seul à la fois pour ouvrir directement une fenêtre précise
#define DEBUG_INTRO     0
#define DEBUG_CHECK     0
#define DEBUG_MENU      1
#define DEBUG_GAME      0   // <-- exemple : démarre directement sur le jeu

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    MainWindow w;

#if DEBUG_INTRO
    // === Mode debug ===
    w.setDebugMode(true);
    qDebug() << "[DEBUG] Lancement direct sur IntroScreen";
    w.showIntro();
#elif DEBUG_CHECK
    // === Mode debug ===
    w.setDebugMode(true);
    qDebug() << "[DEBUG] Lancement direct sur CheckDevicesScreen";
    w.showCheck();
#elif DEBUG_MENU
    // === Mode debug ===
    w.setDebugMode(true);
    qDebug() << "[DEBUG] Lancement direct sur MainMenu";
    w.showMenu();
#elif DEBUG_GAME
    // === Mode debug ===
    w.setDebugMode(true);
    qDebug() << "[DEBUG] Lancement direct sur GameUI";
    w.showGame();
#else
    qDebug() << "[DEBUG] Mode normal";
    w.showIntro();
#endif

    w.show();
    return app.exec();
}
