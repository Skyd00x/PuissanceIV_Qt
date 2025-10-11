#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include "MainMenu.hpp"
#include "GameUI.hpp"
#include "StateMachine.hpp"
#include "Robot.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    QStackedWidget *stack;
    MainMenu *mainMenu;
    GameUI *gameUI;
    Robot *robot;
    StateMachine stateMachine;
};
