#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QStackedWidget>

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(QWidget *parent = nullptr);

    void startGame();  // Lance le countdown et prépare le jeu

    // Slots pour GameLogic
public slots:
    void updateCameraFrame(const QImage &img);
    void setTurnPlayer();
    void setTurnRobot();
    void setDifficultyText(const QString &txt);
    void showEndOfGame(const QString &winnerText, int totalSeconds);

signals:
    void quitRequested();          // L'utilisateur veut quitter la partie
    void countdownFinished();      // Fin du compte à rebours → GameLogic démarre

private slots:
    void updateCountdown();
    void updateChronometer();
    void onQuitButtonClicked();
    void showConfirmationScreen();
    void returnToGame();

private:
    void startCountdown();
    void createGameWidget();
    void createConfirmWidget();

private:
    QStackedWidget *stack;

    // Widgets
    QWidget *gameWidget;
    QWidget *confirmWidget;

    // Éléments du jeu
    QPushButton *quitButton;

    QLabel *titleLabel;          // Partie en mode X
    QLabel *turnLabel;           // Au tour du joueur/robot
    QLabel *cameraLabel;         // Affichage de la caméra

    QLabel *timerLabel;          // Chronomètre
    QTimer chronometer;
    int elapsedSeconds = 0;

    QLabel *countdownTextLabel;  // Texte "Lancement de la partie dans"
    QLabel *countdownLabel;      // Compte à rebours (3,2,1)
    QTimer countdownTimer;
    int countdownValue = 3;
};
