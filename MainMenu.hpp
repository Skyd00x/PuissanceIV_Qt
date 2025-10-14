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

private:
    // ⚙️ Définir l'enum AVANT toute fonction qui l'utilise
    enum class ConfirmationType { None, StartGame, Calibration, Quit };
    StateMachine::Difficulty selectedDifficulty = StateMachine::Difficulty::Easy;
    ConfirmationType currentConfirm = ConfirmationType::None;

private slots:
    void showDifficultyMenu();
    void showMainMenu();
    void onDifficultySelected();
    void showConfirmationMenu(const QString &text, ConfirmationType type);

private:
    QStackedWidget *stack;

    // Écrans
    QWidget *mainMenuWidget;
    QWidget *difficultyWidget;
    QWidget *confirmWidget;

    // Bouton aide
    QPushButton *helpButton;

    // Menu principal
    QLabel *titleLabel;
    QPushButton *launchButton;
    QPushButton *calibrationButton;
    QPushButton *quitButton;

    // Difficultés
    QPushButton *diffEasy;
    QPushButton *diffNormal;
    QPushButton *diffHard;
    QPushButton *diffImpossible;
    QPushButton *backButton;

    // Confirmation
    QLabel *confirmLabel;

    // Méthodes internes
    void createMainMenu();
    void createDifficultyMenu();
    void createConfirmationMenu();
    void animateTransition(QWidget *from, QWidget *to, bool forward);

};
