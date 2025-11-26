#include "GameLogic.hpp"
#include "Negamax.hpp"      // → version SimpleAI que nous venons de créer
using namespace SimpleAI;

// =============================================================
//   CONSTRUCTEUR
// =============================================================
GameLogic::GameLogic(CameraAI* cam,
                     Robot* r,
                     CalibrationLogic* calibration,
                     StateMachine* stm,
                     QObject* parent)
    : QObject(parent),
    camera(cam),
    robot(r),
    calib(calibration),
    sm(stm)
{
    grid.resize(6);
    prevGrid.resize(6);
    for (int r = 0; r < 6; r++) {
        grid[r].resize(7);
        prevGrid[r].resize(7);
    }

    // Frame vers view
    connect(camera, &CameraAI::frameReady,
            this, &GameLogic::sendFrameToScreen,
            Qt::QueuedConnection);

    // Grille mise à jour
    connect(camera, &CameraAI::gridUpdated,
            this, &GameLogic::onGridUpdated,
            Qt::QueuedConnection);
}

GameLogic::~GameLogic()
{
    stopGame();
}

// =============================================================
//   START GAME — appelé après countdownFinished()
// =============================================================
void GameLogic::startGame()
{
    gameRunning = true;

    // Démarrage caméra
    camera->start(0);

    // Texte de difficulté
    QString diffString;
    switch (sm->getDifficulty()) {
    case StateMachine::Easy: diffString = "Facile"; break;
    case StateMachine::Medium: diffString = "Normal"; break;
    case StateMachine::Hard: diffString = "Difficile"; break;
    case StateMachine::Impossible: diffString = "Impossible"; break;
    }
    emit difficultyText(diffString);

    // Le joueur commence
    currentTurn = PlayerTurn;
    emit turnPlayer();
}

// =============================================================
//   STOP GAME — quitter partie
// =============================================================
void GameLogic::stopGame()
{
    gameRunning = false;
    negamaxRunning = false;

    camera->stop();

    if (negamaxThreadObj) {
        negamaxThreadObj->quit();
        negamaxThreadObj->wait();
        delete negamaxThreadObj;
        negamaxThreadObj = nullptr;
    }
}

// =============================================================
//   RÉCEPTION D'UNE GRILLE CAMERA
// =============================================================
void GameLogic::onGridUpdated(const QVector<QVector<int>>& g)
{
    if (!gameRunning)
        return;

    prevGrid = grid;
    grid = g;
    gridReady = true;

    if (currentTurn == PlayerTurn)
    {
        int playedCol = -1;
        if (detectPlayerMove(prevGrid, grid, playedCol))
        {
            currentTurn = RobotTurn;
            emit turnRobot();
            launchRobotTurn();
        }
    }
}

// =============================================================
//   DÉTECTION NOUVEAU PION ROUGE DU JOUEUR
// =============================================================
bool GameLogic::detectPlayerMove(const QVector<QVector<int>>& oldG,
                                 const QVector<QVector<int>>& newG,
                                 int& playedColumn)
{
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            if (oldG[r][c] == 0 && newG[r][c] == 1) {
                playedColumn = c;
                return true;
            }
        }
    }
    return false;
}

// =============================================================
//   DÉTECTION PLACEMENT ROBOT (JAUNE)
// =============================================================
bool GameLogic::detectRobotPlacement(const QVector<QVector<int>>& oldG,
                                     const QVector<QVector<int>>& newG,
                                     int robotColumn)
{
    for (int r = 0; r < 6; r++) {
        if (oldG[r][robotColumn] == 0 && newG[r][robotColumn] == 2)
            return true;
    }
    return false;
}

// =============================================================
//   LANCEMENT TOUR DU ROBOT
// =============================================================
void GameLogic::launchRobotTurn()
{
    if (!gridReady)
        return;

    int depth = 5;
    switch (sm->getDifficulty()) {
    case StateMachine::Easy: depth = 3; break;
    case StateMachine::Medium: depth = 5; break;
    case StateMachine::Hard: depth = 7; break;
    case StateMachine::Impossible: depth = 10; break;
    }

    runNegamax(depth);
}

// =============================================================
//   THREAD : IA SimpleAI
// =============================================================
void GameLogic::runNegamax(int depth)
{
    if (negamaxRunning)
        return;
    negamaxRunning = true;

    QVector<QVector<int>> current = grid;

    negamaxThreadObj = QThread::create([this, current, depth]() {

        // IA : déterminer la meilleure colonne pour le robot (joueur = 2)
        int bestMove = SimpleAI::getBestMove(current, depth);

        // ---------------------------------------------------
        // Action robot : prendre un pion, le placer, revenir
        // ---------------------------------------------------
        robot->openGripper();

        Pose pick = calib->getPosePick();
        robot->goToSecurized(pick);
        robot->closeGripper();

        Pose drop = calib->getPoseForColumn(bestMove);
        robot->goToSecurized(drop);
        robot->openGripper();

        robot->Home();

        // Fin du tour robot
        negamaxRunning = false;
        currentTurn = PlayerTurn;
        emit turnPlayer();
    });

    negamaxThreadObj->start();
}

// =============================================================
//   FIN DE PARTIE (simplifiée)
// =============================================================
bool GameLogic::isWinningMove(int color)
{
    return false; // non utilisé actuellement
}

bool GameLogic::isBoardFull()
{
    for (int c = 0; c < 7; c++)
        if (grid[0][c] == 0)
            return false;
    return true;
}
