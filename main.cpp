#include "MainWindow.hpp"
#include "Robot.hpp"
#include "CalibrationLogic.hpp"
#include <QMetaType>
#include <QTimer>
#include <QDebug>
#include <thread>
#include <chrono>

// === DEBUG FLAGS ===
#define DEBUG_VISIONAI     0
#define DEBUG_INTRO        0
#define DEBUG_CHECK        0
#define DEBUG_MENU         1
#define DEBUG_GAME         0
#define DEBUG_CALIBRATION  0
#define DEBUG_ROBOT_TEST   0

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
    w.showFullScreen();
    return app.exec();

#elif DEBUG_CHECK
    // === MODE DEBUG CHECK ===
    MainWindow w;
    w.setDebugMode(true);
    w.showCheck();
    w.showFullScreen();
    return app.exec();

#elif DEBUG_MENU
    // === MODE DEBUG MENU ===
    MainWindow w;
    w.setDebugMode(true);
    w.showMenu();
    w.showFullScreen();
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

    w.showGame();
    w.showFullScreen();
    return app.exec();

#elif DEBUG_CALIBRATION
    // === MODE DEBUG CALIBRATION ===
    MainWindow w;
    w.setDebugMode(true);
    w.showCalibration();
    w.showFullScreen();
    return app.exec();

#elif DEBUG_ROBOT_TEST
    // === MODE DEBUG TEST ROBOT ===
    qDebug() << "[DEBUG_ROBOT_TEST] Démarrage du test des mouvements du robot";

    Robot robot(nullptr);
    CalibrationLogic calibLogic(&robot, nullptr);

    // Connexion au robot
    qDebug() << "[DEBUG_ROBOT_TEST] Connexion au robot...";
    if (!calibLogic.connectToRobot()) {
        qDebug() << "[DEBUG_ROBOT_TEST] ERREUR: Impossible de se connecter au robot!";
        return 1;
    }
    qDebug() << "[DEBUG_ROBOT_TEST] Robot connecté avec succès";

    // Remise à la position d'origine
    qDebug() << "[DEBUG_ROBOT_TEST] Remise à la position d'origine...";
    robot.Home();
    std::this_thread::sleep_for(std::chrono::seconds(3));

    // Test des mouvements : épuise réservoir gauche (4 pions) puis réservoir droit (4 pions)
    for (int i = 0; i < 8; i++) {
        qDebug() << QString("[DEBUG_ROBOT_TEST] === Test %1/8 ===").arg(i+1);

        // Déterminer la position de réservoir
        CalibPoint pickPoint;
        if (i < 4) {
            // Réservoir gauche (Left_1 à Left_4)
            pickPoint = static_cast<CalibPoint>((int)CalibPoint::Left_1 + i);
            qDebug() << QString("[DEBUG_ROBOT_TEST] Prendre pion à la position Left_%1").arg(i + 1);
        } else {
            // Réservoir droit (Right_1 à Right_4)
            pickPoint = static_cast<CalibPoint>((int)CalibPoint::Right_1 + (i - 4));
            qDebug() << QString("[DEBUG_ROBOT_TEST] Prendre pion à la position Right_%1").arg((i - 4) + 1);
        }

        // 1. Prendre le pion (utilise la nouvelle fonction de haut niveau)
        calibLogic.pickPiece(pickPoint);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        // 2. Déposer dans une colonne (on fait le tour des colonnes 0-6, puis revient à 0)
        int column = i % 7;
        qDebug() << QString("[DEBUG_ROBOT_TEST] Déposer pion dans colonne %1").arg(column + 1);
        calibLogic.dropPiece(column);
        std::this_thread::sleep_for(std::chrono::milliseconds(300));

        qDebug() << QString("[DEBUG_ROBOT_TEST] Test %1/8 terminé").arg(i+1);
    }

    // Retour à la position d'origine
    qDebug() << "[DEBUG_ROBOT_TEST] Retour à la position d'origine...";
    robot.Home();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // Déconnexion
    qDebug() << "[DEBUG_ROBOT_TEST] Déconnexion du robot";
    calibLogic.disconnectToRobot();

    qDebug() << "[DEBUG_ROBOT_TEST] Test terminé avec succès!";
    return 0;

#else
    // === APPLICATION NORMALE ===
    MainWindow w;
    w.showIntro();
    w.showFullScreen();  // Démarrage en plein écran
    return app.exec();
#endif
}
