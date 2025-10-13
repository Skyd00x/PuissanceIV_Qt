#pragma once
#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QStackedWidget>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QParallelAnimationGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QMessageBox>
#include "StateMachine.hpp"

class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QWidget *parent = nullptr);

signals:
    void startGame(StateMachine::Difficulty difficulty);
    void startCalibration();
    void openExplanation();
    void quitGame();

private slots:
    void showDifficultyMenu();
    void showMainMenu();
    void onDifficultySelected();
    void onCalibration();
    void onQuit();

private:
    QStackedWidget *stack; // pour basculer entre les deux vues

    // Vue principale
    QWidget *mainMenuWidget;
    QLabel *titleLabel;
    QPushButton *launchButton;
    QPushButton *calibrationButton;
    QPushButton *quitButton;
    QPushButton *helpButton;

    // Vue difficultés
    QWidget *difficultyWidget;
    QPushButton *backButton;
    QPushButton *diffEasy;
    QPushButton *diffNormal;
    QPushButton *diffHard;
    QPushButton *diffImpossible;

    void createMainMenu();
    void createDifficultyMenu();
    void animateTransition(QWidget *from, QWidget *to);
};
