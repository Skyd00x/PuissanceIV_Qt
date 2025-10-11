#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include "MainMenu.hpp"
#include "GameUI.hpp"
#include "StateMachine.hpp"
#include "Robot.hpp"
#include "IntroScreen.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QStackedWidget *stack;
    IntroScreen *introScreen;
    MainMenu *mainMenu;
    GameUI *gameUI;
    Robot *robot;
    StateMachine stateMachine;
};
