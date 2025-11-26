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

    explanationScreen = new ExplanationScreen(this);
    gameScreen        = new GameScreen(this);

    // === AJOUT AU STACK ===
    stack->addWidget(introScreen);
    stack->addWidget(checkScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(calibrationScreen);
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
        showMenu();
    });

    connect(gameScreen, &GameScreen::countdownFinished,
            gameLogic, &GameLogic::startGame);

    connect(gameLogic, &GameLogic::turnPlayer,
            gameScreen, &GameScreen::setTurnPlayer);

    connect(gameLogic, &GameLogic::turnRobot,
            gameScreen, &GameScreen::setTurnRobot);

    connect(gameLogic, &GameLogic::difficultyText,
            gameScreen, &GameScreen::setDifficultyText);

    connect(gameLogic, &GameLogic::sendFrameToScreen,
            gameScreen, &GameScreen::updateCameraFrame);

    connect(gameLogic, &GameLogic::endOfGame,
            gameScreen, &GameScreen::showEndOfGame);

    // === CONNEXIONS DU MENU ===
    connect(mainMenu, &MainMenu::startGame,
            [&](StateMachine::Difficulty diff){
                stateMachine.setDifficulty(diff);
                showGame();
            });

    connect(mainMenu, &MainMenu::startCalibration,
            this, &MainWindow::showCalibration);

    connect(mainMenu, &MainMenu::openExplanation,
            this, &MainWindow::showExplanation);

    connect(mainMenu, &MainMenu::quitGame,
            this, &MainWindow::close);

    // === CONNEXIONS CALIBRATION ===
    connect(calibrationScreen, &CalibrationScreen::backToMenuRequested, this, [this]() {
        qDebug() << "[MainWindow] Signal backToMenuRequested reçu";
        // Ne pas faire le reset maintenant, il sera fait au prochain affichage
        showMenu();
        qDebug() << "[MainWindow] Affichage du menu principal effectué";

        // Reset après avoir changé d'écran (en asynchrone)
        QTimer::singleShot(100, [this]() {
            qDebug() << "[MainWindow] Début du reset de la calibration";
            calibrationScreen->resetCalibration();
            qDebug() << "[MainWindow] Reset de la calibration terminé";
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
