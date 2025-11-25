#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(QWidget *parent = nullptr);

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

private:
    void startCountdown();

private:
    QPushButton *quitButton;

    QLabel *titleLabel;          // Partie en mode X
    QLabel *turnLabel;           // Au tour du joueur/robot
    QLabel *cameraLabel;         // Affichage de la caméra

    QLabel *timerLabel;          // Chronomètre
    QTimer chronometer;
    int elapsedSeconds = 0;

    QLabel *countdownLabel;      // Compte à rebours (3,2,1)
    QTimer countdownTimer;
    int countdownValue = 3;
};
