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
#include "CalibrationScreen.hpp"
#include "ExplanationScreen.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    void setDebugMode(bool enabled);

    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    // Pour debug rapide
    void showIntro();
    void showCheck();
    void showMenu();
    void showGame();
    void showCalibration();

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    bool debugMode = false;

    QStackedWidget *stack;

    IntroScreen *introScreen;
    CheckDevicesScreen *checkScreen;
    MainMenu *mainMenu;
    GameUI *gameUI;
    CalibrationScreen *calibrationScreen;
    ExplanationScreen *explanationScreen;

    Robot *robot;
    Camera *camera;
    StateMachine stateMachine;
};
