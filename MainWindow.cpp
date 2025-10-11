#include "MainWindow.hpp"
#include <QApplication>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // === Configuration de la fenêtre principale ===
    resize(1280, 720);
    setWindowTitle("Puissance IV - Projet Polytech Tours");

    // === Création du conteneur de vues ===
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // === Initialisation du robot ===
    robot = new Robot();

    // === Création des écrans ===
    mainMenu = new MainMenu();
    gameUI   = new GameUI(robot);

    stack->addWidget(mainMenu); // index 0
    stack->addWidget(gameUI);   // index 1

    stack->setCurrentWidget(mainMenu);

    // === Connexions entre les vues ===
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
}

MainWindow::~MainWindow()
{
    delete gameUI;
    delete mainMenu;
    delete robot;
}
