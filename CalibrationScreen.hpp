#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QTextBrowser>
#include <QMovie>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QTimer>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QColor>
#include <QShowEvent>
#include <QFrame>

#include "CalibrationLogic.hpp"   // adapte la casse au nom réel de ton fichier
#include "Robot.hpp"

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
    void onRobotReady();
    void onLogicProgress(int value);
    void onFadeAnimationFinished();

protected:
    void showEvent(QShowEvent* event) override;

private:
    // Prépare l’UI d’intro (sans auto-connexion)
    void prepareIntroUI(const QString& message = QStringLiteral("Connexion au robot en cours..."));
    // Affiche l’intro ; si autoConnect == true, lance (ou planifie) la connexion
    void showIntroLayout(bool autoConnect);
    void showCalibrationLayout();
    void showEndLayout();

    void styleButton(QPushButton* button, const QString &c1 = "#4F8ED8", const QString &c2 = "#1B3B5F");
    void applyRoundedImageEffect(QLabel *label, const QString &imagePath);

    // === Données ===
    Robot* robot;
    CalibrationLogic* logic;
    int currentStep = -1;

    // État de tentative de connexion en cours (évite les doubles appels)
    bool isConnecting = false;

    // === Éléments communs ===
    QLabel* titleLabel;
    QProgressBar* progressBar;
    QPropertyAnimation* fadeAnimation;
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
    QTextBrowser* instructionsView;
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
