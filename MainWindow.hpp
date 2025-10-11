#pragma once
#include <QMainWindow>
#include <QStackedWidget>
#include "MainMenu.hpp"
#include "GameUI.hpp"
#include "StateMachine.hpp"
#include "Robot.hpp"
#include "IntroScreen.hpp"
#include "Camera.hpp"   // ✅ important

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;  // ✅ ajout

private:
    QStackedWidget *stack;
    IntroScreen *introScreen;
    MainMenu *mainMenu;
    GameUI *gameUI;
    Robot *robot;
    Camera *camera;       // ✅ référence caméra
    StateMachine stateMachine;
};
