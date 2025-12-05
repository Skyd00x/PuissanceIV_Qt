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

    void resetToMainMenu();  // Retourne au menu principal

signals:
    void startGame(StateMachine::Difficulty difficulty, StateMachine::PlayerColor color);
    void startCalibration();
    void startCalibrationTest();
    void openExplanation();
    void quitGame();

private:
    // ⚙️ Définir l'enum AVANT toute fonction qui l'utilise
    enum class ConfirmationType { None, StartGame, Calibration, Quit };
    StateMachine::Difficulty selectedDifficulty = StateMachine::Difficulty::Easy;
    StateMachine::PlayerColor selectedColor = StateMachine::PlayerColor::Red;
    ConfirmationType currentConfirm = ConfirmationType::None;

private slots:
    void showDifficultyMenu();
    void showColorMenu();
    void showMainMenu();
    void onDifficultySelected();
    void onColorSelected();
    void showConfirmationMenu(const QString &text, ConfirmationType type);

private:
    QStackedWidget *stack;

    // Écrans
    QWidget *mainMenuWidget;
    QWidget *difficultyWidget;
    QWidget *colorWidget;
    QWidget *confirmWidget;

    // Bouton aide
    QPushButton *helpButton;

    // Menu principal
    QLabel *titleLabel;
    QPushButton *launchButton;
    QPushButton *calibrationButton;
    QPushButton *testCalibrationButton;
    QPushButton *quitButton;

    // Difficultés
    QPushButton *diffEasy;
    QPushButton *diffNormal;
    QPushButton *diffHard;
    QPushButton *backButtonDiff;

    // Choix de couleur
    QPushButton *colorRed;
    QPushButton *colorYellow;
    QPushButton *backButtonColor;

    // Confirmation
    QLabel *confirmLabel;

    // Méthodes internes
    void createMainMenu();
    void createDifficultyMenu();
    void createColorMenu();
    void createConfirmationMenu();
    void animateTransition(QWidget *from, QWidget *to, bool forward);

};
