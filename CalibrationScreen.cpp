#include "CalibrationScreen.hpp"
#include <thread>
#include <chrono>
#include <algorithm>

// === CONSTRUCTEUR ===
CalibrationScreen::CalibrationScreen(Robot *robot, QWidget *parent)
    : QWidget(parent), robot(robot), pendingUpdate(false)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // === TITRE ===
    titleLabel = new QLabel("Calibration du robot", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #1B3B5F; margin-top: 10px;");

    // === BARRE DE PROGRESSION ===
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 7);
    progressBar->setValue(0);
    progressBar->setFixedSize(900, 25);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%p%");
    progressBar->setStyleSheet(
        "QProgressBar { background-color: #E0E0E0; border-radius: 12px; color: #1B3B5F; "
        "font-size: 18px; font-weight: bold; text-align: center; }"
        "QProgressBar::chunk { background-color: #4F8ED8; border-radius: 12px; }"
        );
    progressBar->hide(); // cachée tant que la calibration n’est pas démarrée

    // === STACK PRINCIPAL ===
    stackedLayout = new QStackedLayout();

    // =====================
    // === INTRO ===
    // =====================
    introWidget = new QWidget(this);
    QVBoxLayout* introLayout = new QVBoxLayout(introWidget);
    introLayout->setAlignment(Qt::AlignCenter);
    introLayout->setSpacing(40);
    introLayout->setContentsMargins(100, 50, 100, 50);

    introLabel = new QLabel(
        "Connexion au robot en cours...", this);
    introLabel->setAlignment(Qt::AlignCenter);
    introLabel->setWordWrap(true);
    introLabel->setStyleSheet("font-size: 26px; color: #1B3B5F; font-weight: bold;");
    introLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    introLabel->setMinimumWidth(900);

    // GIF de chargement
    loadingLabel = new QLabel(this);
    loadingMovie = new QMovie("./Ressources/image/Gifs/simple_loading.gif");
    loadingLabel->setMovie(loadingMovie);
    loadingLabel->setFixedSize(85, 85);
    loadingLabel->setScaledContents(true);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->hide();

    // Boutons
    startButton = new QPushButton("Commencer la calibration");
    styleButton(startButton);
    startButton->setFixedSize(300, 60);
    startButton->hide();

    retryButton = new QPushButton("Réessayer la connexion");
    styleButton(retryButton);
    retryButton->hide();

    introLayout->addWidget(introLabel, 0, Qt::AlignCenter);
    introLayout->addWidget(loadingLabel, 0, Qt::AlignCenter);
    introLayout->addWidget(startButton, 0, Qt::AlignCenter);
    introLayout->addWidget(retryButton, 0, Qt::AlignCenter);
    stackedLayout->addWidget(introWidget);

    // =====================
    // === CALIBRATION ===
    // =====================
    calibrationWidget = new QWidget(this);
    QHBoxLayout* contentLayout = new QHBoxLayout(calibrationWidget);
    contentLayout->setSpacing(30);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setFixedSize(500, 500);
    applyRoundedImageEffect(imageLabel, "./Ressources/image/Calibration/welcome_calibration.png");

    QWidget* formContainer = new QWidget(this);
    QVBoxLayout* formLayout = new QVBoxLayout(formContainer);
    formLayout->setAlignment(Qt::AlignTop);
    label = new QLabel("Instructions de calibration...", this);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold;");

    toggleGripperButton = new QPushButton("Ouvrir / Fermer la pince");
    rotateLeftButton = new QPushButton("↺ Tourner gauche");
    rotateRightButton = new QPushButton("↻ Tourner droite");
    backButton = new QPushButton("Retour");
    nextButton = new QPushButton("Suivant");

    QList<QPushButton*> calibButtons = {
        toggleGripperButton, rotateLeftButton, rotateRightButton, backButton, nextButton
    };
    for (auto *b : calibButtons) {
        styleButton(b);
        b->setFixedSize(200, 50);
    }

    QHBoxLayout* gripLayout = new QHBoxLayout();
    gripLayout->addStretch();
    gripLayout->addWidget(toggleGripperButton);
    gripLayout->addSpacing(10);
    gripLayout->addWidget(rotateLeftButton);
    gripLayout->addSpacing(10);
    gripLayout->addWidget(rotateRightButton);
    gripLayout->addStretch();

    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->addStretch();
    navLayout->addWidget(backButton);
    navLayout->addSpacing(20);
    navLayout->addWidget(nextButton);
    navLayout->addStretch();

    formLayout->addWidget(label, 0, Qt::AlignCenter);
    formLayout->addSpacing(20);
    formLayout->addLayout(gripLayout);
    formLayout->addSpacing(30);
    formLayout->addLayout(navLayout);

    contentLayout->addWidget(imageLabel, 3);
    contentLayout->addWidget(formContainer, 7);
    stackedLayout->addWidget(calibrationWidget);

    // === FIN ===
    endWidget = new QWidget(this);
    QVBoxLayout* endLayout = new QVBoxLayout(endWidget);
    endLayout->setAlignment(Qt::AlignCenter);
    endLayout->setSpacing(25);

    endLabel = new QLabel("Calibration terminée !<br>Vous pouvez maintenant tester les positions, recommencer ou revenir au menu principal.", this);
    endLabel->setAlignment(Qt::AlignCenter);
    endLabel->setWordWrap(true);
    endLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold;");

    testButton = new QPushButton("Tester toutes les positions");
    restartButton = new QPushButton("Recommencer");
    menuButton = new QPushButton("Retour au menu principal");

    QList<QPushButton*> endButtons = { testButton, restartButton, menuButton };
    for (auto *b : endButtons) {
        styleButton(b);
        b->setFixedSize(250, 55);
    }

    endLayout->addWidget(endLabel);
    endLayout->addWidget(testButton);
    endLayout->addWidget(restartButton);
    endLayout->addWidget(menuButton);
    stackedLayout->addWidget(endWidget);

    // === LAYOUT GLOBAL ===
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(progressBar, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(stackedLayout);

    // === LOGIQUE ===
    logic = new CalibrationLogic(robot, this);
    connect(logic, &CalibrationLogic::progressChanged, this, &CalibrationScreen::onLogicProgress);
    connect(logic, &CalibrationLogic::connectionFinished, this, &CalibrationScreen::onConnectionFinished);
    connect(logic, &CalibrationLogic::robotReady, this, &CalibrationScreen::onRobotReady);

    // === ANIMATION ===
    fadeAnimation = new QPropertyAnimation(this, "windowOpacity");
    fadeAnimation->setDuration(300);
    connect(fadeAnimation, &QPropertyAnimation::finished, this, &CalibrationScreen::onFadeAnimationFinished);

    // === BOUTONS ===
    connect(startButton, &QPushButton::clicked, this, &CalibrationScreen::onStartClicked);
    connect(nextButton, &QPushButton::clicked, this, &CalibrationScreen::onNextClicked);
    connect(backButton, &QPushButton::clicked, this, &CalibrationScreen::onBackClicked);
    connect(toggleGripperButton, &QPushButton::clicked, logic, &CalibrationLogic::toggleGripper);
    connect(rotateLeftButton, &QPushButton::clicked, logic, &CalibrationLogic::rotateLeft);
    connect(rotateRightButton, &QPushButton::clicked, logic, &CalibrationLogic::rotateRight);
    connect(testButton, &QPushButton::clicked, this, &CalibrationScreen::onTestClicked);
    connect(restartButton, &QPushButton::clicked, this, &CalibrationScreen::onRestartClicked);
    connect(menuButton, &QPushButton::clicked, this, [this]() { emit backToMenuRequested(); });
    connect(retryButton, &QPushButton::clicked, this, &CalibrationScreen::attemptConnection);

    // === CONNEXION INITIALE ===
    connectionTimer = new QTimer(this);
    connectionTimer->setSingleShot(true);
    connect(connectionTimer, &QTimer::timeout, this, &CalibrationScreen::attemptConnection);
    connectionTimer->start(300);
}

// === CONNEXION AU ROBOT ===
void CalibrationScreen::attemptConnection() {
    introLabel->setText("Connexion au robot en cours...");
    loadingLabel->show();
    loadingMovie->start();
    retryButton->hide();

    logic->connectToRobot();
}

void CalibrationScreen::onConnectionFinished(bool success) {
    if (!success) {
        loadingMovie->stop();
        loadingLabel->hide();
        introLabel->setText("<b>Impossible de se connecter au robot.<br>"
                            "Vérifiez la connexion USB et cliquez sur 'Réessayer'.</b>");
        retryButton->show();
        startButton->hide();
        emit connectionFailed();
        return;
    }

    introLabel->setText("Remise en position initiale du robot...");
    startButton->hide();
    retryButton->hide();
    loadingLabel->show();
    loadingMovie->start();

    logic->homeRobot();
}

void CalibrationScreen::onRobotReady() {
    loadingMovie->stop();
    loadingLabel->hide();

    introLabel->setText("Le robot est maintenant prêt.<br> Les manipulations à effectuer seront affichées tout au long de la calibration.<br>"
                        "<br><em>La durée estimée est de 5 minutes.</em><br>");
    startButton->show();
    progressBar->hide();
}

// === CALIBRATION ===
void CalibrationScreen::onStartClicked() {
    emit calibrationStarted();
    startButton->hide();
    progressBar->show();

    showCalibrationLayout();
    currentStep = 0;
    progressBar->setValue(0);
    logic->startCalibration();
}

void CalibrationScreen::onNextClicked() {
    logic->recordStep(currentStep);
    currentStep++;
    progressBar->setValue(currentStep);
    if (currentStep >= 7)
        showEndLayout();
}

void CalibrationScreen::onBackClicked() {
    if (currentStep > 0) {
        currentStep--;
        progressBar->setValue(currentStep);
    }
}

void CalibrationScreen::onTestClicked() { logic->testCalibration(); }

void CalibrationScreen::onRestartClicked() {
    logic->resetCalibration();
    currentStep = 0;

    showIntroLayout();
    progressBar->hide();
    startButton->hide();

    introLabel->setText("Reconnexion au robot en cours...");
    loadingLabel->show();
    loadingMovie->start();
    attemptConnection();
}

// === OUTILS ===
void CalibrationScreen::fadeOut() { fadeAnimation->setStartValue(1.0); fadeAnimation->setEndValue(0.0); fadeAnimation->start(); }
void CalibrationScreen::fadeIn() { fadeAnimation->setStartValue(0.0); fadeAnimation->setEndValue(1.0); fadeAnimation->start(); }
void CalibrationScreen::onFadeAnimationFinished() {}
void CalibrationScreen::onLogicProgress(int value) { progressBar->setValue(value); }

void CalibrationScreen::showIntroLayout() { stackedLayout->setCurrentWidget(introWidget); }
void CalibrationScreen::showCalibrationLayout() { stackedLayout->setCurrentWidget(calibrationWidget); }
void CalibrationScreen::showEndLayout() { stackedLayout->setCurrentWidget(endWidget); }

void CalibrationScreen::styleButton(QPushButton *button, const QString &c1, const QString &c2) {
    button->setStyleSheet(QString(
                              "QPushButton { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2); color: white;"
                              "font-size: 16px; font-weight: bold; border: 2px solid #1B3B5F; border-radius: 30px; padding: 8px 16px; }"
                              "QPushButton:hover { background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66B0FF, stop:1 #347AD1); }"
                              "QPushButton:pressed { background-color: %2; }"
                              ).arg(c1, c2));
    button->setFont(QFont("Segoe UI", 12, QFont::Bold));
    button->setCursor(Qt::PointingHandCursor);
    auto *shadow = new QGraphicsDropShadowEffect(button);
    shadow->setBlurRadius(20);
    shadow->setOffset(2, 3);
    shadow->setColor(QColor(0, 0, 0, 120));
    button->setGraphicsEffect(shadow);
}

void CalibrationScreen::applyRoundedImageEffect(QLabel *label, const QString &imagePath) {
    QPixmap original(imagePath);
    if (original.isNull()) return;
    QPixmap rounded(original.size());
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(original.rect(), 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, original);
    label->setPixmap(rounded.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}
