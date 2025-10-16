#include "MainWindow.hpp"
#include <QDebug>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1280, 720);
    setWindowTitle("Puissance IV");
    showMaximized();

    // === CONTENEUR PRINCIPAL ===
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // === OBJETS MOTEUR ===
    robot  = new Robot();
    camera = new Camera();

    // === ÉCRANS ===
    introScreen       = new IntroScreen();
    checkScreen       = new CheckDevicesScreen();
    mainMenu          = new MainMenu();
    gameUI            = new GameUI(robot);
    calibrationScreen = new CalibrationScreen(robot);
    explanationScreen = new ExplanationScreen();

    // === AJOUT DANS LE STACK ===
    stack->addWidget(introScreen);
    stack->addWidget(checkScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(gameUI);
    stack->addWidget(calibrationScreen);
    stack->addWidget(explanationScreen);

    // === DÉMARRAGE ===
    stack->setCurrentWidget(introScreen);

    // === TRANSITIONS ENTRE SCÈNES ===
    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
        if (debugMode) return; // Désactive le comportement automatique
        stack->setCurrentWidget(checkScreen);
        checkScreen->startChecking();
    });

    connect(checkScreen, &CheckDevicesScreen::readyToContinue, this, [this]() {
        if (debugMode) return;
        qDebug() << "Connexion au robot et démarrage du flux vidéo...";

        if (robot && !robot->connect()) {
            qWarning() << "Échec de connexion au Dobot.";
        }

        if (camera) {
            camera->start();
        }

        stack->setCurrentWidget(mainMenu);
    });

    // === MENU PRINCIPAL ===
    connect(mainMenu, &MainMenu::startGame, this,
            [this](StateMachine::Difficulty diff) {
                stateMachine.setDifficulty(diff);
                stack->setCurrentWidget(gameUI);
                stateMachine.ChangeState(StateMachine::State::Game);
            });

    connect(mainMenu, &MainMenu::startCalibration, this, [this]() {
        stack->setCurrentWidget(calibrationScreen);
        stateMachine.ChangeState(StateMachine::State::Calibration);
    });

    connect(mainMenu, &MainMenu::openExplanation, this, [this]() {
        stack->setCurrentWidget(explanationScreen);
        stateMachine.ChangeState(StateMachine::State::Explanation);
    });

    connect(mainMenu, &MainMenu::quitGame, this, [this]() {
        close();
    });

    // === RETOUR DEPUIS LE JEU ===
    connect(gameUI, &GameUI::backClicked, this, [this]() {
        stack->setCurrentWidget(mainMenu);
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });

    // === RETOURS FUTURS ===
    connect(calibrationScreen, &CalibrationScreen::backToMenu, this, [this]() {
        stack->setCurrentWidget(mainMenu);
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });

    connect(explanationScreen, &ExplanationScreen::backToMenu, this, [this]() {
        stack->setCurrentWidget(mainMenu);
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });

    // === CAMERA ===
    connect(camera, &Camera::frameReady, gameUI, &GameUI::updateCameraFrame);

    // === LANCEMENT INTRO ===
    introScreen->start();
}

MainWindow::~MainWindow()
{
    if (camera) {
        camera->stop();
        delete camera;
    }

    if (robot) delete robot;

    delete gameUI;
    delete mainMenu;
    delete checkScreen;
    delete introScreen;
    delete calibrationScreen;
    delete explanationScreen;
}

void MainWindow::setDebugMode(bool enabled)
{
    debugMode = enabled;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "Fermeture de l’application - arrêt des périphériques";
    if (camera) camera->stop();
    QMainWindow::closeEvent(event);
}

void MainWindow::showIntro()
{
    stack->setCurrentWidget(introScreen);
    introScreen->start();
}

void MainWindow::showCheck()
{
    stack->setCurrentWidget(checkScreen);
    checkScreen->startChecking();
}

void MainWindow::showMenu()
{
    stack->setCurrentWidget(mainMenu);
}

void MainWindow::showGame()
{
    stack->setCurrentWidget(gameUI);
    stateMachine.ChangeState(StateMachine::State::Game);
}

void MainWindow::showCalibration()
{
    stack->setCurrentWidget(calibrationScreen);
    stateMachine.ChangeState(StateMachine::State::Calibration);
}
