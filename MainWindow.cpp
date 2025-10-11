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

    robot = new Robot();
    camera = new Camera();   // ✅ on crée la caméra ici

    introScreen = new IntroScreen();
    mainMenu = new MainMenu();
    gameUI   = new GameUI(robot);

    stack->addWidget(introScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(gameUI);
    stack->setCurrentWidget(introScreen);

    // ✅ Connexion caméra → GameUI
    connect(camera, &Camera::frameReady, gameUI, &GameUI::updateCameraFrame);
    camera->start();  // démarre la capture dès le lancement

    // transitions habituelles
    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
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

    introScreen->start();
}

MainWindow::~MainWindow()
{
    if (camera) {
        camera->stop();     // ✅ arrête la boucle proprement
        delete camera;
    }
    delete gameUI;
    delete mainMenu;
    delete introScreen;
    delete robot;
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "🛑 Fermeture de l’application - arrêt de la caméra";
    if (camera) camera->stop();   // ✅ ferme proprement la webcam
    QMainWindow::closeEvent(event);
}
