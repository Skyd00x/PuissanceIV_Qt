#pragma once
#include <QMainWindow>
#include <QStackedWidget>

#include "MainMenu.hpp"
#include "GameUI.hpp"
#include "StateMachine.hpp"
#include "Robot.hpp"
#include "IntroScreen.hpp"
#include "Camera.hpp"
#include "CheckDevicesScreen.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QStackedWidget *stack;
    IntroScreen *introScreen;
    CheckDevicesScreen *checkScreen;
    MainMenu *mainMenu;
    GameUI *gameUI;
    Robot *robot;
    Camera *camera;
    StateMachine stateMachine;
};
