#include "MainWindow.hpp"
#include <QMetaType>

// === DEBUG FLAGS ===
#define DEBUG_VISIONAI     0
#define DEBUG_INTRO        0
#define DEBUG_CHECK        0
#define DEBUG_MENU         1
#define DEBUG_GAME         0
#define DEBUG_CALIBRATION  0

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    qRegisterMetaType<QImage>("QImage");

#if DEBUG_VISIONAI
    // === MODE DEBUG VISION AI ===
    QLabel label;
    label.show();

    CameraAI ai;
    ai.start(0);

    QObject::connect(&ai, &CameraAI::frameReady, &label, [&](const QImage& img){
        label.setPixmap(QPixmap::fromImage(img).scaled(1280, 720, Qt::KeepAspectRatio));
    }, Qt::QueuedConnection);

    // Impression périodique de la grille (toutes les 1s)
    QTimer* gridPrinter = new QTimer(&app);
    gridPrinter->setInterval(1000);

    bool hasGrid = false;
    QObject::connect(gridPrinter, &QTimer::timeout, [&](){
        QVector<QVector<int>> grid;
        const int r = ai.getGrille(grid);
        if (r == 0) {
            hasGrid = true;
            QString out = "\n=== GRID ===\n";
            for (const auto& row : grid) {
                QString line;
                for (int c = 0; c < row.size(); ++c) {
                    line += QString::number(row[c]);
                    if (c < row.size() - 1) line += ' ';
                }
                out += line + '\n';
            }
            qDebug().noquote() << out;
        }
        else if (!hasGrid) {
            qDebug() << "[GRID] Incomplète — en attente d'une détection complète (6x7).";
        }
    });
    gridPrinter->start();

    return app.exec();

#elif DEBUG_INTRO
    // === MODE DEBUG INTRO ===
    MainWindow w;
    w.setDebugMode(true);
    w.showIntro();
    w.show();
    return app.exec();

#elif DEBUG_CHECK
    // === MODE DEBUG CHECK ===
    MainWindow w;
    w.setDebugMode(true);
    w.showCheck();
    w.show();
    return app.exec();

#elif DEBUG_MENU
    // === MODE DEBUG MENU ===
    MainWindow w;
    w.setDebugMode(true);
    w.showMenu();
    w.show();
    return app.exec();

#elif DEBUG_GAME
    // === MODE DEBUG GAME ===
    MainWindow w;
    w.setDebugMode(true);

    // CHOIX DE LA DIFFICULTÉ POUR LE MODE DEBUG
    w.stateMachine.setDifficulty(StateMachine::Difficulty::Hard);
    // Options possibles :
    // StateMachine::Easy
    // StateMachine::Medium
    // StateMachine::Hard
    // StateMachine::Impossible

    w.showGame();
    w.show();
    return app.exec();

#elif DEBUG_CALIBRATION
    // === MODE DEBUG CALIBRATION ===
    MainWindow w;
    w.setDebugMode(true);
    w.showCalibration();
    w.show();
    return app.exec();

#else
    // === APPLICATION NORMALE ===
    MainWindow w;
    w.showIntro();
    w.show();
    return app.exec();
#endif
}
