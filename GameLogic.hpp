#pragma once

#include <QObject>
#include <QVector>
#include <QThread>
#include <QMutex>
#include <atomic>

#include "CameraAi.hpp"
#include "Robot.hpp"
#include "StateMachine.hpp"
#include "CalibrationLogic.hpp"

// Forward declaration de GameScreen (view)
class GameScreen;

class GameLogic : public QObject
{
    Q_OBJECT

public:
    explicit GameLogic(CameraAI* cam,
                       Robot* robot,
                       CalibrationLogic* calibration,
                       StateMachine* sm,
                       QObject* parent = nullptr);

    ~GameLogic();

public slots:
    void startGame();          // après countdownFinished()
    void stopGame();           // bouton quitter
    void onGridUpdated(const QVector<QVector<int>>& g);

signals:
    // Vers GameScreen
    void turnPlayer();
    void turnRobot();
    void difficultyText(QString);
    void sendFrameToScreen(QImage img);
    void endOfGame(QString winnerText, int totalSeconds);

private:
    enum Turn { PlayerTurn, RobotTurn };
    Turn currentTurn = PlayerTurn;

    // Pointeurs externes (non possédés)
    CameraAI* camera;
    Robot* robot;
    CalibrationLogic* calib;
    StateMachine* sm;

    QVector<QVector<int>> grid;           // grille actuelle
    QVector<QVector<int>> prevGrid;       // grille précédente

    bool gridReady = false;               // caméra OK
    bool gameRunning = false;
    int elapsedSeconds = 0;               // récupéré depuis GameScreen

    // Thread pour SimpleAI
    QThread* negamaxThreadObj = nullptr;
    std::atomic<bool> negamaxRunning = false;

private:
    bool detectPlayerMove(const QVector<QVector<int>>& oldG,
                          const QVector<QVector<int>>& newG,
                          int& playedColumn);

    bool detectRobotPlacement(const QVector<QVector<int>>& oldG,
                              const QVector<QVector<int>>& newG,
                              int robotColumn);

    void launchRobotTurn();
    void runNegamax(int depth);

    bool isWinningMove(int color);
    bool isBoardFull();
};
