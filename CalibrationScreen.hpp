#pragma once
#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QTimer>
#include <QApplication>
#include "Robot.hpp"

class CalibrationScreen : public QWidget
{
    Q_OBJECT
public:
    explicit CalibrationScreen(Robot *robot, QWidget *parent = nullptr);

signals:
    void backToMenu();

private slots:
    // === Étapes principales ===
    void attemptConnection();
    void onConnectionFinished(bool success);
    void onStartClicked();
    void onNextClicked();
    void onBackClicked();              // ✅ ajouté
    void onToggleGripperClicked();
    void onRotateLeftClicked();
    void onRotateRightClicked();
    void onTestClicked();
    void onRestartClicked();
    void updateStep(int step, const QString &message);

private:
    Robot *robot;

    QLabel *titleLabel;
    QLabel *label;
    QProgressBar *progressBar;

    QPushButton *startButton;
    QPushButton *nextButton;
    QPushButton *backButton;           // ✅ ajouté
    QPushButton *toggleGripperButton;
    QPushButton *rotateLeftButton;
    QPushButton *rotateRightButton;
    QPushButton *testButton;
    QPushButton *restartButton;
    QPushButton *retryButton;

    QTimer *connectionTimer;

    void styleButton(QPushButton *button, const QString &color1 = "#4F8ED8", const QString &color2 = "#2C5FA3");
};
