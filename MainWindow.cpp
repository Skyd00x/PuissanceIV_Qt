#include "MainWindow.hpp"
#include <QCloseEvent>
#include <QDebug>
#include <QTimer>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Puissance IV Robotisé");
    setMinimumSize(1280, 720);

    // === STACK PRINCIPAL ===
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // === OBJETS MOTEUR ===
    robot     = new Robot(this);
    cameraAI  = new CameraAI();  // Pas de parent pour permettre moveToThread()

    // === ÉCRANS UI ===
    introScreen       = new IntroScreen(this);
    checkScreen       = new CheckDevicesScreen(this);
    mainMenu          = new MainMenu(this);

    // ❗ Correction : on passe robot, pas "this"
    calibrationScreen = new CalibrationScreen(robot, this);

    // Écran de test de calibration (nécessite robot et CalibrationLogic)
    calibrationTestScreen = new CalibrationTestScreen(robot, calibrationScreen->getCalibrationLogic(), this);

    explanationScreen = new ExplanationScreen(this);
    gameScreen        = new GameScreen(this);

    // === AJOUT AU STACK ===
    stack->addWidget(introScreen);
    stack->addWidget(checkScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(calibrationScreen);
    stack->addWidget(calibrationTestScreen);
    stack->addWidget(explanationScreen);
    stack->addWidget(gameScreen);

    // === CRÉATION GAMELOGIC ===
    gameLogic = new GameLogic(
        cameraAI,
        robot,
        calibrationScreen->getCalibrationLogic(), // getter ajouté
        &stateMachine,
        this
        );

    // === CONNEXIONS GAME SCREEN ↔ GAME LOGIC ===
    connect(gameScreen, &GameScreen::quitRequested, this, [this]() {
        gameLogic->stopGame();
        gameScreen->resetGame();  // Reset complet de l'écran de jeu

        // Réinitialiser l'état de connexion du robot pour refaire Home() à la prochaine partie
        gameLogic->resetRobotConnection();

        showMenu();
    });

    connect(gameScreen, &GameScreen::prepareGame,
            gameLogic, &GameLogic::prepareGame);

    connect(gameScreen, &GameScreen::countdownFinished,
            gameLogic, &GameLogic::startGame);

    connect(gameLogic, &GameLogic::turnPlayer,
            gameScreen, &GameScreen::setTurnPlayer);

    connect(gameLogic, &GameLogic::turnRobot,
            gameScreen, &GameScreen::setTurnRobot);

    connect(gameLogic, &GameLogic::robotStatus,
            gameScreen, &GameScreen::setRobotStatus);

    connect(gameLogic, &GameLogic::difficultyText,
            gameScreen, &GameScreen::setDifficultyText);

    connect(gameLogic, &GameLogic::sendFrameToScreen,
            gameScreen, &GameScreen::updateCameraFrame);

    connect(gameLogic, &GameLogic::endOfGame,
            gameScreen, &GameScreen::showEndOfGame);

    connect(cameraAI, &CameraAI::gridIncomplete, this, [this](int detectedCount) {
        // Ne pas afficher l'overlay de grille incomplète si on attend le remplissage des réservoirs
        if (!gameScreen->isReservoirOverlayVisible()) {
            gameScreen->showGridIncompleteWarning(detectedCount);
        }
    }, Qt::QueuedConnection);

    connect(cameraAI, &CameraAI::gridComplete, this, [this]() {
        gameScreen->resetAllOverlays();
    }, Qt::QueuedConnection);

    connect(gameLogic, &GameLogic::robotInitializing,
            gameScreen, &GameScreen::showRobotInitializing);

    connect(gameLogic, &GameLogic::robotInitialized,
            gameScreen, &GameScreen::hideRobotInitializing);

    connect(gameLogic, &GameLogic::robotInitialized,
            gameScreen, &GameScreen::startCountdownWhenReady);

    connect(gameLogic, &GameLogic::cheatDetected,
            gameScreen, &GameScreen::showCheatDetected);

    connect(gameLogic, &GameLogic::reservoirEmpty,
            gameScreen, &GameScreen::showReservoirEmpty);

    connect(gameScreen, &GameScreen::reservoirsRefilled,
            gameLogic, &GameLogic::onReservoirsRefilled);

    connect(gameLogic, &GameLogic::gameResult,
            gameScreen, &GameScreen::showGameResult);

    connect(gameLogic, &GameLogic::connectionFailed,
            gameScreen, &GameScreen::showConnectionError);

    // === CONNEXIONS DU MENU ===
    connect(mainMenu, &MainMenu::startGame,
            [&](StateMachine::Difficulty diff, StateMachine::PlayerColor color){
                stateMachine.setDifficulty(diff);
                stateMachine.setPlayerColor(color);
                showGame();
            });

    connect(mainMenu, &MainMenu::startCalibration,
            this, &MainWindow::showCalibration);

    connect(mainMenu, &MainMenu::startCalibrationTest,
            this, &MainWindow::showCalibrationTest);

    connect(mainMenu, &MainMenu::openExplanation,
            this, &MainWindow::showExplanation);

    connect(mainMenu, &MainMenu::quitGame,
            this, &MainWindow::close);

    // === CONNEXIONS CALIBRATION ===
    connect(calibrationScreen, &CalibrationScreen::backToMenuRequested, this, [this]() {
        qDebug() << "[MainWindow] Signal backToMenuRequested reçu";

        // IMPORTANT : Le robot a déjà été déconnecté dans CalibrationScreen::onQuitButtonClicked()
        // On évite donc la double déconnexion qui pourrait causer un crash

        // Informer GameLogic que le robot a été déconnecté
        gameLogic->resetRobotConnection();
        qDebug() << "[MainWindow] État de connexion réinitialisé dans GameLogic";

        showMenu();
        qDebug() << "[MainWindow] Affichage du menu principal effectué";

        // Reset UI après avoir changé d'écran (en asynchrone)
        QTimer::singleShot(100, [this]() {
            qDebug() << "[MainWindow] Début du reset de la calibration";
            calibrationScreen->resetCalibration();
            qDebug() << "[MainWindow] Reset de la calibration terminé";
        });
    });

    // === CONNEXIONS CALIBRATION TEST ===
    connect(calibrationTestScreen, &CalibrationTestScreen::backToMenuRequested, this, [this]() {
        qDebug() << "[MainWindow] Retour au menu depuis le test de calibration";

        // Informer GameLogic que le robot peut avoir été déconnecté
        gameLogic->resetRobotConnection();

        showMenu();

        // Reset de l'écran de test après transition
        QTimer::singleShot(100, [this]() {
            calibrationTestScreen->resetScreen();
        });
    });

    // === CONNEXIONS EXPLANATION ===
    connect(explanationScreen, &ExplanationScreen::backToMenu,
            this, &MainWindow::showMenu);

    // === CONNEXIONS INTRO → CHECK → MENU ===
    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
        showCheck();
        checkScreen->startChecking();
    });

    connect(checkScreen, &CheckDevicesScreen::readyToContinue, this, [this]() {
        showMenu();
    });

    // === DÉMARRAGE PAR L'INTRO ===
    // Géré dans main.cpp pour respecter les modes debug
}

MainWindow::~MainWindow()
{
    // Nettoyage de CameraAI qui n'a pas de parent
    if (cameraAI) {
        cameraAI->stop();
        delete cameraAI;
        cameraAI = nullptr;
    }
}

void MainWindow::setDebugMode(bool enabled)
{
    debugMode = enabled;
}

// =====================================================
//        MÉTHODES D'AFFICHAGE DES ÉCRANS
// =====================================================

void MainWindow::showIntro()
{
    stack->setCurrentWidget(introScreen);
    introScreen->start();
}

void MainWindow::showCheck()
{
    stack->setCurrentWidget(checkScreen);
}

void MainWindow::showMenu()
{
    mainMenu->resetToMainMenu();  // Reset le menu au principal
    stack->setCurrentWidget(mainMenu);
}

void MainWindow::showCalibration()
{
    stack->setCurrentWidget(calibrationScreen);
}

void MainWindow::showCalibrationTest()
{
    calibrationTestScreen->resetScreen();
    stack->setCurrentWidget(calibrationTestScreen);
}

void MainWindow::showExplanation()
{
    stack->setCurrentWidget(explanationScreen);
}

void MainWindow::showGame()
{
    stack->setCurrentWidget(gameScreen);
    gameScreen->startGame();
}

// =====================================================
//      FERMETURE DE LA FENÊTRE
// =====================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (gameLogic)
        gameLogic->stopGame();
    if (cameraAI)
        cameraAI->stop();

    event->accept();
}
