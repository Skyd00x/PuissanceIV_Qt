// CalibrationScreen.hpp
#pragma once
#include <QWidget>
#include <QLabel>
#include <QMovie>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QProgressBar>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QGraphicsDropShadowEffect>
#include <QPalette>
#include <QPixmap>
#include <QApplication>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QPropertyAnimation>
#include <vector>
#include "Robot.hpp"

// === STRUCTURE D'ÉTAPE DE CALIBRATION ===
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

// === CLASSE CALIBRATIONSCREEN ===
class CalibrationScreen : public QWidget {
    Q_OBJECT
public:
    explicit CalibrationScreen(Robot *robot, QWidget *parent = nullptr);

signals:
    void backToMenu();

private slots:
    void attemptConnection();
    void onConnectionFinished(bool success);
    void onStartClicked();
    void onNextClicked();
    void onBackClicked();
    void onToggleGripperClicked();
    void onRotateLeftClicked();
    void onRotateRightClicked();
    void onTestClicked();
    void onRestartClicked();
    void onFadeAnimationFinished();

private:
    void recordCalibrationStep();
    void resetCalibration();
    void applyCalibration();
    void saveCalibration(const QString &path = "./Ressources/calibration.json");
    void loadCalibration(const QString &path = "./Ressources/calibration.json");
    void styleButton(QPushButton *button, const QString &c1="#4F8ED8", const QString &c2="#1B3B5F");
    void updateStepUI();
    void fadeOut();
    void fadeIn();

    // === ATTRIBUTS ===
    Robot *robot;
    QTimer *connectionTimer;
    QLabel *titleLabel;
    QLabel *label;
    QLabel *imageLabel;
    QLabel *loadingLabel;
    QMovie *loadingMovie;
    QProgressBar *progressBar;
    QPushButton *startButton;
    QPushButton *nextButton;
    QPushButton *backButton;
    QPushButton *toggleGripperButton;
    QPushButton *rotateLeftButton;
    QPushButton *rotateRightButton;
    QPushButton *testButton;
    QPushButton *restartButton;
    QPushButton *retryButton;
    QPushButton *menuButton;
    int currentStep = 0;
    std::vector<CalibrationStep> steps;
    QPropertyAnimation *fadeAnimation;
    QWidget *imageContainer;
    QWidget *formContainer;
};
