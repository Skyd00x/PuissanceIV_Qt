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
    void prepareGame();        // avant le countdown - connexion robot et home
    void startGame();          // après countdownFinished() - démarrage caméra et jeu
    void stopGame();           // bouton quitter
    void onGridUpdated(const QVector<QVector<int>>& g);
    void onReservoirsRefilled(); // Appelé quand l'utilisateur a rempli les réservoirs
    void resetRobotConnection(); // Réinitialise l'état de connexion du robot

signals:
    // Vers GameScreen
    void turnPlayer();
    void turnRobot();
    void robotStatus(QString status);  // État détaillé du robot
    void difficultyText(QString);
    void sendFrameToScreen(QImage img);
    void endOfGame(QString winnerText, int totalSeconds);
    void robotInitializing();          // Robot en cours de mise en position
    void robotInitialized();           // Robot prêt
    void cheatDetected(QString reason); // Triche détectée
    void reservoirEmpty();             // Réservoirs vides
    void gameResult(QString winner, QString difficulty, int totalSeconds);  // Fin de partie (victoire/égalité)
    void connectionFailed();           // Échec de connexion au robot (avec possibilité de réessayer)

private:
    enum Turn { PlayerTurn, RobotTurn, WaitingForRobotDetection };
    Turn currentTurn = PlayerTurn;
    int lastRobotColumn = -1;  // Dernière colonne jouée par le robot

    // Pointeurs externes (non possédés)
    CameraAI* camera;
    Robot* robot;
    CalibrationLogic* calib;
    StateMachine* sm;

    QVector<QVector<int>> grid;           // grille actuelle
    QVector<QVector<int>> prevGrid;       // grille précédente
    QVector<QVector<int>> candidateGrid;  // grille candidate à valider
    int gridConfirmCount = 0;             // compteur de détections identiques (besoin de 5)
    static constexpr int GRID_CONFIRM_THRESHOLD = 5;  // nombre de détections nécessaires

    // Anti-cheat : compteurs pour détecter les triches sur plusieurs images
    int cheatMultiplePiecesCount = 0;     // Compteur pour plusieurs pions ajoutés
    int cheatWrongColorCount = 0;         // Compteur pour mauvaise couleur
    int cheatDuringRobotCount = 0;        // Compteur pour pions ajoutés pendant tour robot
    static constexpr int CHEAT_CONFIRM_THRESHOLD = 30;  // 30 images consécutives pour confirmer triche

    bool gridReady = false;               // caméra OK
    bool gameRunning = false;
    bool robotConnected = false;          // robot connecté et prêt
    int elapsedSeconds = 0;               // récupéré depuis GameScreen

    // Thread pour SimpleAI
    QThread* negamaxThreadObj = nullptr;
    std::atomic<bool> negamaxRunning = false;

    // Thread pour la préparation du robot
    QThread* preparationThreadObj = nullptr;
    std::atomic<bool> preparationRunning = false;

    // Compteurs de pions dans les réservoirs (initialisés à plein)
    int leftReservoirPieces = 4;          // Réservoir gauche : 4 pions
    int rightReservoirPieces = 4;         // Réservoir droit : 4 pions
    int currentLeftIndex = 0;             // Prochain index dans le réservoir gauche (0-3)
    int currentRightIndex = 0;            // Prochain index dans le réservoir droit (0-3)

    // Couleurs des joueurs (1=rouge, 2=jaune)
    int playerColor = 1;                  // Couleur choisie par le joueur
    int robotColor = 2;                   // Couleur du robot (inverse du joueur)

private:
    bool detectPlayerMove(const QVector<QVector<int>>& oldG,
                          const QVector<QVector<int>>& newG,
                          int& playedColumn);

    bool detectRobotPlacement(const QVector<QVector<int>>& oldG,
                              const QVector<QVector<int>>& newG,
                              int robotColumn);

    void launchRobotTurn();
    void runNegamax(int depth);

    bool checkWin(int color);          // Vérifier si une couleur a gagné (4 alignés)
    bool isBoardFull();

    // Gestion des réservoirs de pions
    CalibPoint getNextReservoirPosition();

    // Comparaison de grilles
    bool areGridsEqual(const QVector<QVector<int>>& g1, const QVector<QVector<int>>& g2);
};
