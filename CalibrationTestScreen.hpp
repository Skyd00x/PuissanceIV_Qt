#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QMovie>
#include <atomic>

#include "CalibrationLogic.hpp"
#include "Robot.hpp"

class CalibrationTestScreen : public QWidget {
    Q_OBJECT

public:
    explicit CalibrationTestScreen(Robot* robot, CalibrationLogic* calibLogic, QWidget* parent = nullptr);

    void resetScreen();  // Réinitialiser l'écran pour un nouveau test

signals:
    void backToMenuRequested();

public slots:
    void onStartTestClicked();
    void onStopTestClicked();
    void onBackToMenuClicked();

private:
    void styleButton(QPushButton* button, const QString &c1 = "#4F8ED8", const QString &c2 = "#1B3B5F");
    void runTest();  // Exécute le test dans un thread

    Robot* robot;
    CalibrationLogic* calib;

    std::atomic<bool> testRunning;
    std::atomic<bool> shouldStop;

    QLabel* titleLabel;
    QLabel* instructionsLabel;
    QLabel* statusLabel;
    QMovie* loadingMovie;
    QLabel* loadingLabel;
    QProgressBar* progressBar;

    QPushButton* startButton;
    QPushButton* stopButton;
    QPushButton* backButton;
    QPushButton* emergencyStopButton;
};
