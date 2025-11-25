#include "MainWindow.hpp"
#include <QCloseEvent>
#include <QDebug>

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
    cameraAI  = new CameraAI(this);

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
    connect(gameScreen, &GameScreen::quitRequested,
            gameLogic, &GameLogic::stopGame);

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

    connect(mainMenu, &MainMenu::quitGame,
            this, &MainWindow::close);

    // === DÉMARRAGE PAR L'INTRO ===
    showIntro();
}

MainWindow::~MainWindow() {}

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
}

void MainWindow::showCheck()
{
    stack->setCurrentWidget(checkScreen);
}

void MainWindow::showMenu()
{
    stack->setCurrentWidget(mainMenu);
}

void MainWindow::showCalibration()
{
    stack->setCurrentWidget(calibrationScreen);
}

void MainWindow::showGame()
{
    stack->setCurrentWidget(gameScreen);
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
