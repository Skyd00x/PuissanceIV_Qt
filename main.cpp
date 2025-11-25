#include "MainWindow.hpp"

// === DEBUG FLAGS ===
#define DEBUG_VISIONAI     0
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

#else
    // === APPLICATION COMPLÈTE ===

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

    // CHOIX DE LA DIFFICULTÉ POUR LE MODE DEBUG
    w.stateMachine.setDifficulty(StateMachine::Difficulty::Hard);
    // Options possibles :
    // StateMachine::Easy
    // StateMachine::Medium
    // StateMachine::Hard
    // StateMachine::Impossible

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
