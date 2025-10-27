#include "CalibrationScreen.hpp"

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
    progressBar->hide();

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

    introLabel = new QLabel("Connexion au robot en cours", this);
    introLabel->setAlignment(Qt::AlignCenter);
    introLabel->setWordWrap(true);
    introLabel->setStyleSheet("font-size: 26px; color: #1B3B5F; font-weight: bold;");
    introLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    introLabel->setMinimumWidth(900);

    loadingLabel = new QLabel(this);
    loadingMovie = new QMovie("./Ressources/image/Gifs/simple_loading.gif");
    loadingLabel->setMovie(loadingMovie);
    loadingLabel->setFixedSize(85, 85);
    loadingLabel->setScaledContents(true);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->hide();

    startButton = new QPushButton("Commencer la calibration");
    retryButton = new QPushButton("Réessayer la connexion");
    startButton->hide();
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
    contentLayout->setSpacing(50);
    contentLayout->setContentsMargins(40, 20, 40, 20);
    contentLayout->setAlignment(Qt::AlignVCenter);

    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setFixedSize(500, 500);
    applyRoundedImageEffect(imageLabel, "./Ressources/image/Calibration/welcome_calibration.png");

    auto *shadow = new QGraphicsDropShadowEffect(imageLabel);
    shadow->setBlurRadius(30);
    shadow->setOffset(8, 8);
    shadow->setColor(QColor(0, 0, 0, 100));
    imageLabel->setGraphicsEffect(shadow);

    QWidget* formContainer = new QWidget(this);
    formContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout* formLayout = new QVBoxLayout(formContainer);
    formLayout->setAlignment(Qt::AlignVCenter);
    formLayout->setContentsMargins(0, 0, 30, 0);
    formLayout->setSpacing(20);

    instructionsView = new QTextBrowser(this);
    instructionsView->setOpenLinks(false);
    instructionsView->setOpenExternalLinks(false);
    instructionsView->setReadOnly(true);
    instructionsView->setFrameShape(QFrame::NoFrame);
    instructionsView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    instructionsView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    instructionsView->setAlignment(Qt::AlignCenter);
    instructionsView->setStyleSheet(
        "QTextBrowser {"
        "  background: transparent;"
        "  color: #1B3B5F;"
        "  font-size: 24px;"
        "  font-weight: bold;"
        "  padding: 10px;"
        "  text-align: justify;"
        "}"
        );
    instructionsView->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    instructionsView->setMinimumWidth(750);
    instructionsView->setMaximumHeight(100);

    toggleGripperButton = new QPushButton("Ouvrir / Fermer la pince");
    rotateLeftButton = new QPushButton("↺ Tourner gauche");
    rotateRightButton = new QPushButton("↻ Tourner droite");
    backButton = new QPushButton("Retour");
    nextButton = new QPushButton("Suivant");

    QHBoxLayout* gripLayout = new QHBoxLayout();
    gripLayout->setAlignment(Qt::AlignCenter);
    gripLayout->setSpacing(10);
    gripLayout->addWidget(toggleGripperButton);
    gripLayout->addWidget(rotateLeftButton);
    gripLayout->addWidget(rotateRightButton);

    QHBoxLayout* navLayout = new QHBoxLayout();
    navLayout->setAlignment(Qt::AlignCenter);
    navLayout->setSpacing(20);
    navLayout->addWidget(backButton);
    navLayout->addWidget(nextButton);

    formLayout->addWidget(instructionsView, 0, Qt::AlignCenter);
    formLayout->addSpacing(10);
    formLayout->addLayout(gripLayout);
    formLayout->addSpacing(20);
    formLayout->addLayout(navLayout);

    contentLayout->addWidget(imageLabel, 3);
    contentLayout->addWidget(formContainer, 7);
    stackedLayout->addWidget(calibrationWidget);

    // =====================
    // === FIN (modifiée uniquement dans la structure)
    // =====================
    endWidget = new QWidget(this);
    QVBoxLayout* endLayout = new QVBoxLayout(endWidget);
    endLayout->setAlignment(Qt::AlignCenter);
    endLayout->setSpacing(40);
    endLayout->setContentsMargins(100, 80, 100, 80);

    endLabel = new QLabel(
        "Calibration terminée.<br>"
        "Vous pouvez maintenant tester les positions, recommencer la calibration ou revenir au menu principal.",
        this
        );
    endLabel->setAlignment(Qt::AlignCenter);
    endLabel->setWordWrap(true);
    endLabel->setStyleSheet("font-size: 26px; color: #1B3B5F; font-weight: bold;");
    endLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    endLabel->setMinimumWidth(900);  // même largeur que l’intro


    testButton = new QPushButton("Tester toutes les positions");
    restartButton = new QPushButton("Recommencer");
    menuButton = new QPushButton("Retour au menu principal");

    endLayout->addWidget(endLabel, 0, Qt::AlignCenter);
    endLayout->addSpacing(30);
    endLayout->addWidget(testButton, 0, Qt::AlignCenter);
    endLayout->addWidget(restartButton, 0, Qt::AlignCenter);
    endLayout->addWidget(menuButton, 0, Qt::AlignCenter);
    stackedLayout->addWidget(endWidget);

    // === LAYOUT GLOBAL ===
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(progressBar, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(stackedLayout);

    QList<QPushButton*> allButtons = {
        startButton, retryButton,
        nextButton, backButton,
        toggleGripperButton, rotateLeftButton, rotateRightButton,
        testButton, restartButton, menuButton
    };
    for (auto *b : allButtons) {
        styleButton(b);
        b->setFixedHeight(55);
    }

    // === LOGIQUE ===
    logic = new CalibrationLogic(robot, this);
    connect(logic, &CalibrationLogic::progressChanged, this, &CalibrationScreen::onLogicProgress);
    connect(logic, &CalibrationLogic::connectionFinished, this, &CalibrationScreen::onConnectionFinished);
    connect(logic, &CalibrationLogic::robotReady, this, &CalibrationScreen::onRobotReady);
    connect(logic, &CalibrationLogic::stepChanged, this, [this](const CalibrationStep& step, int index) {
        instructionsView->clear();
        instructionsView->setHtml(step.text);
        instructionsView->document()->adjustSize();
        instructionsView->updateGeometry();
        instructionsView->document()->setTextWidth(instructionsView->viewport()->width());

        applyRoundedImageEffect(imageLabel, step.imagePath);
        nextButton->setVisible(step.showNext);
        backButton->setVisible(step.showBack);
        toggleGripperButton->setVisible(step.showGripper);
        rotateLeftButton->setVisible(step.showRotation);
        rotateRightButton->setVisible(step.showRotation);
        testButton->setVisible(step.showTest);
        restartButton->setVisible(step.showRestart);
        menuButton->setVisible(step.showMenu);
        currentStep = index;
    });
    connect(logic, &CalibrationLogic::calibrationFinished, this, [this]() {
        showEndLayout();
    });
    connect(logic, &CalibrationLogic::calibrationTestFinished, this, [this]() {
        // 🔹 Cacher le GIF
        loadingMovie->stop();
        loadingLabel->hide();
        progressBar->setRange(0, 7);
        progressBar->setValue(7);

        // 🔹 Remettre le texte d’origine
        endLabel->setText(
            "Calibration terminée.<br>"
            "Vous pouvez maintenant tester les positions, recommencer la calibration ou revenir au menu principal."
            );

        // 🔹 Réafficher les boutons
        testButton->show();
        restartButton->show();
        menuButton->show();
    });

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
    connect(menuButton, &QPushButton::clicked, this, [this]() {
        // 🔹 Réinitialiser la logique
        logic->resetCalibration();
        currentStep = 0;

        // 🔹 Réinitialiser l'affichage
        progressBar->setRange(0, 7);
        progressBar->setValue(0);
        progressBar->hide();

        loadingMovie->stop();
        loadingLabel->hide();

        introLabel->setText("Connexion au robot en cours...");
        startButton->hide();
        retryButton->hide();

        // 🔹 Revenir à l’écran d’intro propre
        showIntroLayout();

        // 🔹 Relancer automatiquement la tentative de connexion
        connectionTimer->start(300);  // ✅ relance du timer initial (appelera attemptConnection)

        // 🔹 Signaler au reste du programme qu’on retourne au menu
        emit backToMenuRequested();
    });
    connect(retryButton, &QPushButton::clicked, this, &CalibrationScreen::attemptConnection);

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

    introLabel->setText("Remise en position initiale du robot");
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
}

void CalibrationScreen::onBackClicked() {
    logic->previousStep();
}

void CalibrationScreen::onTestClicked() {
    // 🔹 Cacher les boutons pendant le test
    testButton->hide();
    restartButton->hide();
    menuButton->hide();
    progressBar->setRange(0, 100); // pourcentage de progression du test
    progressBar->setValue(0);

    endLabel->setText("<b>Test des positions calibrées en cours...</b>");
    loadingLabel->show();
    loadingMovie->start();

    QTimer::singleShot(100, [this]() {
        logic->testCalibration();
    });
}

void CalibrationScreen::onRestartClicked() {
    // 🔹 Réinitialiser la logique
    logic->resetCalibration();
    currentStep = 0;

    // 🔹 Afficher le message de reset et le GIF immédiatement
    introLabel->setText("<b>Remise en position initiale du robot...</b>");
    showIntroLayout();
    startButton->hide();
    progressBar->hide();

    loadingLabel->show();
    loadingMovie->start();

    // 🔹 Lancer la remise à zéro du robot après un petit délai
    QTimer::singleShot(200, [this]() {
        logic->homeRobot();
    });
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
                              "QPushButton {"
                              "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
                              "   color: white;"
                              "   font-size: 18px;"
                              "   font-weight: bold;"
                              "   border: none;"
                              "   border-radius: 30px;"
                              "   padding: 10px 20px;"
                              "   min-width: 220px;"
                              "   min-height: 55px;"
                              "   text-align: center;"
                              "}"
                              "QPushButton:hover {"
                              "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66B0FF, stop:1 #347AD1);"
                              "}"
                              "QPushButton:pressed {"
                              "   background-color: %2;"
                              "}"
                              ).arg(c1, c2));

    button->setFont(QFont("Segoe UI", 12, QFont::Bold));
    button->setCursor(Qt::PointingHandCursor);

    // Ombre portée pour effet 3D doux
    auto *shadow = new QGraphicsDropShadowEffect(button);
    shadow->setBlurRadius(20);
    shadow->setOffset(2, 3);
    shadow->setColor(QColor(0, 0, 0, 120));
    button->setGraphicsEffect(shadow);
}

void CalibrationScreen::applyRoundedImageEffect(QLabel *label, const QString &imagePath) {
    QPixmap original(imagePath);
    if (original.isNull()) return;

    // Création d’une pixmap avec coins arrondis
    QPixmap rounded(original.size());
    rounded.fill(Qt::transparent);

    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(original.rect(), 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, original);
    painter.end();

    // Mise à l’échelle
    QPixmap scaled = rounded.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    label->setPixmap(scaled);
    label->setStyleSheet("background: transparent; border: none;");
}

