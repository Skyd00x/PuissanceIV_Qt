#include "MainWindow.hpp"
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    resize(1280, 720);
    setWindowTitle("Puissance IV");
    showMaximized();

    // === Création du conteneur de vues ===
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // === Initialisation du robot ===
    robot = new Robot();

    // === Création des écrans ===
    introScreen = new IntroScreen();
    mainMenu = new MainMenu();
    gameUI   = new GameUI(robot);

    // === Ajout dans la pile ===
    stack->addWidget(introScreen); // index 0
    stack->addWidget(mainMenu);    // index 1
    stack->addWidget(gameUI);      // index 2

    stack->setCurrentWidget(introScreen);

    // === Transition automatique après l’intro ===
    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
        qDebug() << "🎬 Fin de l’intro → affichage du menu principal";
        stack->setCurrentWidget(mainMenu);
    });

    // === Connexions existantes ===
    connect(mainMenu, &MainMenu::playClicked, this, [this]() {
        qDebug() << "➡️ Passage à la fenêtre de jeu";
        stack->setCurrentWidget(gameUI);
        stateMachine.ChangeState(StateMachine::State::Game);
    });

    connect(gameUI, &GameUI::backClicked, this, [this]() {
        qDebug() << "⬅️ Retour au menu principal";
        stack->setCurrentWidget(mainMenu);
        stateMachine.ChangeState(StateMachine::State::MainMenu);
    });

    connect(mainMenu, &MainMenu::difficultyChanged,
            this, [this](StateMachine::Difficulty diff, float p1, float p2, float p3) {
                qDebug() << "🎚️ Difficulté sélectionnée:" << (int)diff << p1 << p2 << p3;
                stateMachine.setDifficulty(diff, p1, p2, p3);
            });

    // === Lancement de l’intro ===
    introScreen->start();
}

MainWindow::~MainWindow()
{
    delete gameUI;
    delete mainMenu;
    delete introScreen;
    delete robot;
}
