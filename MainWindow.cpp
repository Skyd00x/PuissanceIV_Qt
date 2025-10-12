#include "MainWindow.hpp"
#include <QDebug>
#include <QCloseEvent>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1280, 720);
    setWindowTitle("Puissance IV");
    showMaximized();

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    robot  = new Robot();
    camera = new Camera();

    introScreen = new IntroScreen();
    checkScreen = new CheckDevicesScreen();
    mainMenu    = new MainMenu();
    gameUI      = new GameUI(robot);

    stack->addWidget(introScreen);
    stack->addWidget(checkScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(gameUI);
    stack->setCurrentWidget(introScreen);

    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
        stack->setCurrentWidget(checkScreen);
        checkScreen->startChecking();
    });

    connect(checkScreen, &CheckDevicesScreen::readyToContinue, this, [this]() {
        qDebug() << "Connexion au robot et démarrage du flux vidéo...";

        if (robot && !robot->connect()) {
            qWarning() << "Échec de connexion au Dobot.";
        }

        if (camera) {
            camera->start();
        }

        stack->setCurrentWidget(mainMenu);
    });

    connect(mainMenu, &MainMenu::playClicked, this, [this]() {
        stack->setCurrentWidget(gameUI);
        stateMachine.ChangeState(StateMachine::State::Game);
    });

    connect(gameUI, &GameUI::backClicked, this, [this]() {
        stack->setCurrentWidget(mainMenu);
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });

    connect(mainMenu, &MainMenu::difficultyChanged,
            this, [this](StateMachine::Difficulty diff, float p1, float p2, float p3) {
                stateMachine.setDifficulty(diff, p1, p2, p3);
            });

    connect(camera, &Camera::frameReady, gameUI, &GameUI::updateCameraFrame);

    introScreen->start();
}

MainWindow::~MainWindow()
{
    if (camera) {
        camera->stop();
        delete camera;
    }

    if (robot) {
        delete robot;
    }

    delete gameUI;
    delete mainMenu;
    delete checkScreen;
    delete introScreen;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "Fermeture de l’application - arrêt des périphériques";
    if (camera) camera->stop();
    QMainWindow::closeEvent(event);
}
