#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QProgressBar>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QStackedLayout>
#include <QStackedWidget>
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

#include "CalibrationLogic.hpp"
#include "Robot.hpp"

class CalibrationScreen : public QWidget {
    Q_OBJECT

public:
    explicit CalibrationScreen(Robot* robot, QWidget* parent = nullptr);

    // 👉 AJOUT : permet au MainWindow de récupérer la logique interne
    CalibrationLogic* getCalibrationLogic() const { return logic; }

    void fadeIn();
    void fadeOut();
    void resetCalibration();  // Reset complet de la calibration

signals:
    void backToMenuRequested();
    void calibrationStarted();
    void calibrationFinished();
    void connectionFailed();

    void stepChanged(int currentStep);
    void progressUpdated(int value);

public slots:
    void onStartClicked();
    void onNextClicked();
    void onBackClicked();
    void onTestClicked();
    void onRestartClicked();

    void attemptConnection();
    void onConnectionFinished(bool success);
    void onRobotReady();
    void onLogicProgress(int value);
    void onFadeAnimationFinished();

    void onQuitButtonClicked();
    void showConfirmationScreen();
    void returnToCalibration();

protected:
    void showEvent(QShowEvent* event) override;

private:
    void prepareIntroUI(const QString& message = QStringLiteral("Connexion au robot en cours..."));
    void showIntroLayout(bool autoConnect);
    void showCalibrationLayout();
    void showEndLayout();
    void createConfirmWidget();

    void styleButton(QPushButton* button, const QString &c1 = "#4F8ED8", const QString &c2 = "#1B3B5F");
    void applyRoundedImageEffect(QLabel *label, const QString &imagePath);

    // === Données ===
    Robot* robot;
    CalibrationLogic* logic;
    int currentStep = -1;

    bool isConnecting = false;

    QLabel* titleLabel;
    QProgressBar* progressBar;
    QPropertyAnimation* fadeAnimation;

    QStackedWidget* mainStack;      // Stack global (mainWidget + confirmWidget)
    QWidget* mainWidget;            // Widget principal contenant tout sauf confirmation
    QWidget* confirmWidget;         // Widget de confirmation

    QStackedLayout* stackedLayout;  // Stack interne (intro, calibration, end)

    QWidget* introWidget;
    QWidget* calibrationWidget;
    QWidget* endWidget;

    QPushButton* quitButton;        // Bouton quitter en haut à gauche

    QLabel* introLabel;
    QLabel* loadingLabel;
    QMovie* loadingMovie;
    QPushButton* startButton;
    QPushButton* retryButton;

    QTextBrowser* instructionsView;
    QLabel* imageLabel;
    QPushButton* nextButton;
    QPushButton* backButton;
    QPushButton* toggleGripperButton;
    QPushButton* rotateLeftButton;
    QPushButton* rotateRightButton;

    QLabel* endLabel;
    QPushButton* testButton;
    QPushButton* restartButton;
    QPushButton* menuButton;
};
