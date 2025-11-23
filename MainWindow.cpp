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
    qDebug() << "[MainWindow] Création Robot / CameraAI";
    robot     = new Robot();          // comme dans ton code original
    cameraAI  = new CameraAI(this);   // QObject parenté à la fenêtre (OK)

    // === ÉCRANS ===
    introScreen       = new IntroScreen();
    checkScreen       = new CheckDevicesScreen();
    mainMenu          = new MainMenu();
    calibrationScreen = new CalibrationScreen(robot);
    explanationScreen = new ExplanationScreen();

    // === AJOUT DANS LE STACK ===
    stack->addWidget(introScreen);
    stack->addWidget(checkScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(calibrationScreen);
    stack->addWidget(explanationScreen);

    // === ÉCRAN DE DÉPART ===
    stack->setCurrentWidget(introScreen);
    introScreen->start();

    // =========================================================
    //                 TRANSITIONS ENTRE ÉCRANS
    // =========================================================

    // --- Intro -> Check ---
    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
        if (debugMode) {
            qDebug() << "[MainWindow] introFinished en mode debug, on ne passe pas aux checks.";
            return;
        }
        qDebug() << "[MainWindow] introFinished, passage à l'écran de vérification.";
        showCheck();
    });

    // --- Check -> Menu principal ---
    connect(checkScreen, &CheckDevicesScreen::readyToContinue, this, [this]() {
        qDebug() << "[MainWindow] readyToContinue reçu depuis CheckDevicesScreen";
        showMenu();
    });

    // --- Menu -> Calibration ---
    connect(mainMenu, &MainMenu::startCalibration, this, [this]() {
        qDebug() << "[MainWindow] startCalibration depuis MainMenu.";
        stack->setCurrentWidget(calibrationScreen);
        stateMachine.ChangeState(StateMachine::State::Calibration);
    });

    // --- Menu -> Explication ---
    connect(mainMenu, &MainMenu::openExplanation, this, [this]() {
        qDebug() << "[MainWindow] openExplanation depuis MainMenu.";
        stack->setCurrentWidget(explanationScreen);
        stateMachine.ChangeState(StateMachine::State::Explanation);
    });

    // --- Menu -> Quitter ---
    connect(mainMenu, &MainMenu::quitGame, this, [this]() {
        qDebug() << "[MainWindow] quitGame depuis MainMenu.";
        close();
    });

    // --- Calibration -> Menu ---
    connect(calibrationScreen, &CalibrationScreen::backToMenuRequested, this, [this]() {
        qDebug() << "[MainWindow] Retour menu depuis Calibration.";
        showMenu();
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });

    // --- Explication -> Menu ---
    connect(explanationScreen, &ExplanationScreen::backToMenu, this, [this]() {
        qDebug() << "[MainWindow] Retour menu depuis Explication.";
        showMenu();
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });
}

MainWindow::~MainWindow()
{
    qDebug() << "[MainWindow] Destruction, arrêt des périphériques.";

    if (cameraAI) {
        cameraAI->stop();    // arrête le thread + cap
        // pas de delete ici, cameraAI a un parent (this)
    }

    if (robot) {
        // si tu veux, tu peux ajouter robot->disconnect(); ici
        delete robot;
        robot = nullptr;
    }

    // Les écrans sont détruits par le QStackedWidget (parentage Qt)
}

void MainWindow::setDebugMode(bool enabled)
{
    debugMode = enabled;
    qDebug() << "[MainWindow] Debug mode =" << enabled;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "[MainWindow] closeEvent, arrêt CameraAI.";
    if (cameraAI) cameraAI->stop();
    QMainWindow::closeEvent(event);
}

void MainWindow::showIntro()
{
    qDebug() << "[MainWindow] showIntro()";
    stack->setCurrentWidget(introScreen);
    introScreen->start();
}

void MainWindow::showCheck()
{
    qDebug() << "[MainWindow] showCheck()";
    stack->setCurrentWidget(checkScreen);
    checkScreen->startChecking();
}

void MainWindow::showMenu()
{
    qDebug() << "[MainWindow] showMenu()";
    stack->setCurrentWidget(mainMenu);
}

void MainWindow::showCalibration()
{
    qDebug() << "[MainWindow] showCalibration()";
    stack->setCurrentWidget(calibrationScreen);
    stateMachine.ChangeState(StateMachine::State::Calibration);
}
