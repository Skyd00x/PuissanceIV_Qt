#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QMovie>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QGraphicsOpacityEffect>

#include "calibrationLogic.hpp"
#include "Robot.hpp"

struct CalibrationStep {
    QString text;
    QString imagePath;
    bool showNext;
    bool showBack;
    bool showGripper;
    bool showRotation;
    bool showTest;
    bool showRestart;
    bool showMenu;
};

class CalibrationScreen : public QWidget {
    Q_OBJECT

public:
    explicit CalibrationScreen(Robot* robot, QWidget* parent = nullptr);

    void fadeIn();
    void fadeOut();

signals:
    void backToMenuRequested();
    void calibrationStarted();
    void calibrationFinished();
    void connectionFailed();

    void stepChanged(int currentStep);
    void progressUpdated(int value);

public slots:
    // === Boutons ===
    void onStartClicked();
    void onNextClicked();
    void onBackClicked();
    void onTestClicked();
    void onRestartClicked();

    // === Logique robot ===
    void attemptConnection();
    void onConnectionFinished(bool success);
    void onRobotReady(); // ✅ quand homing terminé
    void onLogicProgress(int value);
    void onFadeAnimationFinished();

private:
    void styleButton(QPushButton* button, const QString &c1 = "#4F8ED8", const QString &c2 = "#1B3B5F");
    void applyRoundedImageEffect(QLabel *label, const QString &imagePath);
    void showIntroLayout();
    void showCalibrationLayout();
    void showEndLayout();

    // === Données ===
    Robot* robot;
    CalibrationLogic* logic;
    std::vector<CalibrationStep> steps;
    int currentStep = -1;
    bool pendingUpdate;

    // === Éléments communs ===
    QLabel* titleLabel;
    QProgressBar* progressBar;
    QPropertyAnimation* fadeAnimation;
    QTimer* connectionTimer;
    QStackedLayout* stackedLayout;

    // === Layouts ===
    QWidget* introWidget;
    QWidget* calibrationWidget;
    QWidget* endWidget;

    // === Intro ===
    QLabel* introLabel;
    QLabel* loadingLabel;
    QMovie* loadingMovie;
    QPushButton* startButton;
    QPushButton* retryButton;

    // === Calibration ===
    QLabel* label;
    QLabel* imageLabel;
    QPushButton* nextButton;
    QPushButton* backButton;
    QPushButton* toggleGripperButton;
    QPushButton* rotateLeftButton;
    QPushButton* rotateRightButton;

    // === Fin ===
    QLabel* endLabel;
    QPushButton* testButton;
    QPushButton* restartButton;
    QPushButton* menuButton;
};
