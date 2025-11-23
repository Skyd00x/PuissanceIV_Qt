#pragma once

#include <QMainWindow>
#include <QStackedWidget>
#include <QApplication>

#include "Robot.hpp"
#include "CameraAi.hpp"
#include "StateMachine.hpp"
#include "IntroScreen.hpp"
#include "CheckDevicesScreen.hpp"
#include "MainMenu.hpp"
#include "CalibrationScreen.hpp"
#include "ExplanationScreen.hpp"

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

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QStackedWidget *stack = nullptr;

    Robot      *robot     = nullptr;
    CameraAI   *cameraAI  = nullptr;
    StateMachine stateMachine;

    IntroScreen        *introScreen       = nullptr;
    CheckDevicesScreen *checkScreen       = nullptr;
    MainMenu           *mainMenu          = nullptr;
    CalibrationScreen  *calibrationScreen = nullptr;
    ExplanationScreen  *explanationScreen = nullptr;

    bool debugMode = false;
};
