#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QApplication>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>

#include "Robot.hpp"
#include "CameraAi.hpp"
#include "StateMachine.hpp"

#include "IntroScreen.hpp"
#include "CheckDevicesScreen.hpp"
#include "MainMenu.hpp"
#include "CalibrationScreen.hpp"
#include "CalibrationTestScreen.hpp"
#include "ExplanationScreen.hpp"

#include "GameScreen.hpp"
#include "GameLogic.hpp"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void setDebugMode(bool enabled);
    void showIntro();
    void showCheck();
    void showMenu();
    void showCalibration();
    void showCalibrationTest();
    void showExplanation();
    void showGame();

protected:
    void closeEvent(QCloseEvent *event) override;

public:
    StateMachine stateMachine; // ← doit être public pour debug

private:
    void animateTransition(QWidget *from, QWidget *to, bool forward);

    QStackedWidget *stack = nullptr;

    Robot      *robot     = nullptr;
    CameraAI   *cameraAI  = nullptr;

    IntroScreen        *introScreen       = nullptr;
    CheckDevicesScreen *checkScreen       = nullptr;
    MainMenu           *mainMenu          = nullptr;
    CalibrationScreen  *calibrationScreen = nullptr;
    CalibrationTestScreen *calibrationTestScreen = nullptr;
    ExplanationScreen  *explanationScreen = nullptr;

    GameScreen         *gameScreen        = nullptr;
    GameLogic          *gameLogic         = nullptr;

    bool debugMode = false;
};
