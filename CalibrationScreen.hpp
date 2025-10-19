#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
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
    // === Signaux pour la navigation ===
    void backToMenuRequested();         // Pour revenir au menu principal
    void calibrationStarted();          // Quand la calibration débute
    void calibrationFinished();         // Quand elle est terminée
    void connectionFailed();            // Si la connexion au robot échoue

    // === Signaux d’état interne ===
    void stepChanged(int currentStep);  // Émis à chaque changement d’étape
    void progressUpdated(int value);    // Émis quand la barre progresse

public slots:
    // === Actions des boutons ===
    void onStartClicked();
    void onNextClicked();
    void onBackClicked();
    void onToggleGripperClicked();
    void onRotateLeftClicked();
    void onRotateRightClicked();
    void onTestClicked();
    void onRestartClicked();

    // === Logique interne ===
    void attemptConnection();
    void onConnectionFinished(bool success);
    void onLogicMessage(const QString& msg);
    void onLogicProgress(int value);
    void onFadeAnimationFinished();

private:
    void styleButton(QPushButton* button, const QString &c1 = "#4F8ED8", const QString &c2 = "#1B3B5F");
    void applyRoundedImageEffect(QLabel *label, const QString &imagePath);
    void updateStepUI();

    // === Données ===
    Robot* robot;
    CalibrationLogic* logic;
    std::vector<CalibrationStep> steps;
    int currentStep = -1;
    bool pendingUpdate;

    // === Éléments UI ===
    QLabel* titleLabel;
    QLabel* label;
    QLabel* imageLabel;
    QLabel* loadingLabel;
    QMovie* loadingMovie;
    QProgressBar* progressBar;

    QWidget* imageContainer;
    QWidget* formContainer;

    QPushButton* startButton;
    QPushButton* nextButton;
    QPushButton* backButton;
    QPushButton* toggleGripperButton;
    QPushButton* rotateLeftButton;
    QPushButton* rotateRightButton;
    QPushButton* testButton;
    QPushButton* restartButton;
    QPushButton* retryButton;
    QPushButton* menuButton;

    // === Animation & Timer ===
    QPropertyAnimation* fadeAnimation;
    QTimer* connectionTimer;
};

