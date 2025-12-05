#include "CalibrationScreen.hpp"
#include <QPalette>
#include <QFont>
#include <QDebug>

// === CONSTRUCTEUR ===
CalibrationScreen::CalibrationScreen(Robot *robot, QWidget *parent)
    : QWidget(parent), robot(robot)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // === WIDGET PRINCIPAL (contient tout sauf confirmation) ===
    mainWidget = new QWidget(this);

    // === TITRE ===
    titleLabel = new QLabel("Calibration du robot", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #1B3B5F; margin-top: 10px;");

    // === BARRE DE PROGRESSION ===
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 8);  // 8 étapes au total (0-7 + fin)
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
    {
        QVBoxLayout* introLayout = new QVBoxLayout(introWidget);
        introLayout->setAlignment(Qt::AlignCenter);
        introLayout->setSpacing(40);
        introLayout->setContentsMargins(100, 50, 100, 50);

        introLabel = new QLabel("Connexion au robot en cours...", this);
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
    }

    // =====================
    // === CALIBRATION ===
    // =====================
    calibrationWidget = new QWidget(this);
    {
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
        instructionsView->setMinimumHeight(120);
        instructionsView->setMaximumHeight(180);

        toggleGripperButton = new QPushButton("Ouvrir la pince");  // État initial : pince fermée
        rotateLeftButton   = new QPushButton("↺ Tourner gauche");
        rotateRightButton  = new QPushButton("↻ Tourner droite");

        // Boutons de déplacement automatique vers les zones
        goToLeftReservoirButton  = new QPushButton("→ Réservoir gauche");
        goToRightReservoirButton = new QPushButton("→ Réservoir droit");
        goToGridButton           = new QPushButton("→ Grille");

        // Boutons de déplacement fin
        moveXPlusButton  = new QPushButton("X+");
        moveXMinusButton = new QPushButton("X-");
        moveYPlusButton  = new QPushButton("Y+");
        moveYMinusButton = new QPushButton("Y-");
        moveZPlusButton  = new QPushButton("Z+");
        moveZMinusButton = new QPushButton("Z-");

        // Style compact pour les boutons de déplacement
        QString compactStyle = QString(
            "QPushButton {"
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4F8ED8, stop:1 #1B3B5F);"
            "   color: white;"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "   border: none;"
            "   border-radius: 15px;"
            "   padding: 5px;"
            "   min-width: 35px;"
            "   max-width: 35px;"
            "   min-height: 30px;"
            "   max-height: 30px;"
            "}"
            "QPushButton:hover {"
            "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66B0FF, stop:1 #347AD1);"
            "}"
            "QPushButton:pressed {"
            "   background-color: #1B3B5F;"
            "}"
        );

        QList<QPushButton*> moveButtons = {
            moveXPlusButton, moveXMinusButton,
            moveYPlusButton, moveYMinusButton,
            moveZPlusButton, moveZMinusButton
        };
        for (auto *btn : moveButtons) {
            btn->setStyleSheet(compactStyle);
            btn->setCursor(Qt::PointingHandCursor);
        }

        backButton = new QPushButton("Retour");
        nextButton = new QPushButton("Suivant");

        QHBoxLayout* gripLayout = new QHBoxLayout();
        gripLayout->setAlignment(Qt::AlignCenter);
        gripLayout->setSpacing(10);
        gripLayout->addWidget(toggleGripperButton);
        gripLayout->addWidget(rotateLeftButton);
        gripLayout->addWidget(rotateRightButton);

        // Layout pour les boutons de déplacement automatique par zone
        QHBoxLayout* zoneLayout = new QHBoxLayout();
        zoneLayout->setAlignment(Qt::AlignCenter);
        zoneLayout->setSpacing(10);
        zoneLayout->addWidget(goToLeftReservoirButton);
        zoneLayout->addWidget(goToRightReservoirButton);
        zoneLayout->addWidget(goToGridButton);

        // Layout pour les boutons de déplacement fin
        QHBoxLayout* moveLayout = new QHBoxLayout();
        moveLayout->setAlignment(Qt::AlignCenter);
        moveLayout->setSpacing(5);
        moveLayout->addWidget(moveXMinusButton);
        moveLayout->addWidget(moveXPlusButton);
        moveLayout->addWidget(moveYMinusButton);
        moveLayout->addWidget(moveYPlusButton);
        moveLayout->addWidget(moveZMinusButton);
        moveLayout->addWidget(moveZPlusButton);

        QHBoxLayout* navLayout = new QHBoxLayout();
        navLayout->setAlignment(Qt::AlignCenter);
        navLayout->setSpacing(20);
        navLayout->addWidget(backButton);
        navLayout->addWidget(nextButton);

        formLayout->addWidget(instructionsView, 0, Qt::AlignCenter);
        formLayout->addSpacing(10);
        formLayout->addLayout(gripLayout);
        formLayout->addSpacing(5);
        formLayout->addLayout(zoneLayout);
        formLayout->addSpacing(5);
        formLayout->addLayout(moveLayout);
        formLayout->addSpacing(20);
        formLayout->addLayout(navLayout);

        contentLayout->addWidget(imageLabel, 3);
        contentLayout->addWidget(formContainer, 7);
        stackedLayout->addWidget(calibrationWidget);
    }

    // =====================
    // === FIN ===
    // =====================
    endWidget = new QWidget(this);
    {
        QVBoxLayout* endLayout = new QVBoxLayout(endWidget);
        endLayout->setAlignment(Qt::AlignCenter);
        endLayout->setSpacing(40);
        endLayout->setContentsMargins(100, 80, 100, 80);

        endLabel = new QLabel(
            "Calibration terminée !<br>"
            "Vous pouvez maintenant recommencer la calibration ou retourner au menu principal.",
            this
            );
        endLabel->setAlignment(Qt::AlignCenter);
        endLabel->setWordWrap(true);
        endLabel->setStyleSheet("font-size: 26px; color: #1B3B5F; font-weight: bold;");
        endLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        endLabel->setMinimumWidth(900);

        restartButton = new QPushButton("Recommencer");
        menuButton = new QPushButton("Retour au menu principal");

        endLayout->addWidget(endLabel, 0, Qt::AlignCenter);
        endLayout->addSpacing(30);
        endLayout->addWidget(restartButton, 0, Qt::AlignCenter);
        endLayout->addWidget(menuButton, 0, Qt::AlignCenter);
        stackedLayout->addWidget(endWidget);
    }

    // === BOUTON QUITTER ===
    quitButton = new QPushButton("← Quitter", mainWidget);
    quitButton->setFixedSize(160, 55);
    quitButton->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 27px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
        );
    quitButton->setCursor(Qt::PointingHandCursor);
    connect(quitButton, &QPushButton::clicked, this, &CalibrationScreen::showConfirmationScreen);

    auto *shadowQuit = new QGraphicsDropShadowEffect;
    shadowQuit->setBlurRadius(20);
    shadowQuit->setOffset(3, 3);
    shadowQuit->setColor(QColor(0, 0, 0, 80));
    quitButton->setGraphicsEffect(shadowQuit);

    // === LAYOUT GLOBAL DU MAIN WIDGET ===
    {
        QVBoxLayout* mainLayout = new QVBoxLayout(mainWidget);
        mainLayout->setContentsMargins(0, 0, 0, 0);

        // Layout du header avec bouton quitter
        QHBoxLayout* headerLayout = new QHBoxLayout;
        headerLayout->setContentsMargins(20, 20, 20, 0);
        headerLayout->addWidget(quitButton, 0, Qt::AlignLeft | Qt::AlignTop);
        headerLayout->addStretch();

        mainLayout->addLayout(headerLayout);
        mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
        mainLayout->addSpacing(10);
        mainLayout->addWidget(progressBar, 0, Qt::AlignHCenter);
        mainLayout->addSpacing(20);
        mainLayout->addLayout(stackedLayout);
    }

    // Styliser tous les boutons (sauf les boutons de déplacement déjà stylés)
    {
        QList<QPushButton*> allButtons = {
            startButton, retryButton,
            nextButton, backButton,
            toggleGripperButton, rotateLeftButton, rotateRightButton,
            goToLeftReservoirButton, goToRightReservoirButton, goToGridButton,
            restartButton, menuButton
        };
        for (auto *b : allButtons) {
            styleButton(b);
            b->setFixedHeight(55);
        }
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

        // Boutons de déplacement visibles pour toutes les étapes de calibration
        bool showMovement = step.showGripper || step.showRotation;
        moveXPlusButton->setVisible(showMovement);
        moveXMinusButton->setVisible(showMovement);
        moveYPlusButton->setVisible(showMovement);
        moveYMinusButton->setVisible(showMovement);
        moveZPlusButton->setVisible(showMovement);
        moveZMinusButton->setVisible(showMovement);

        // Boutons de zone visibles pendant la calibration
        goToLeftReservoirButton->setVisible(showMovement);
        goToRightReservoirButton->setVisible(showMovement);
        goToGridButton->setVisible(showMovement);

        restartButton->setVisible(step.showRestart);
        menuButton->setVisible(step.showMenu);
        currentStep = index;

        emit stepChanged(currentStep);
    });
    connect(logic, &CalibrationLogic::calibrationFinished, this, [this]() {
        showEndLayout();
        emit calibrationFinished();
    });
    connect(logic, &CalibrationLogic::calibrationTestFinished, this, [this]() {
        loadingMovie->stop();
        loadingLabel->hide();
        progressBar->setRange(0, 8);
        progressBar->setValue(8);

        endLabel->setText(
            "Calibration terminée.<br>"
            "Vous pouvez maintenant recommencer la calibration ou revenir au menu principal."
            );

        restartButton->show();
        menuButton->show();
    });

    // Connexion pour mettre à jour le texte du bouton selon l'état de la pince
    connect(logic, &CalibrationLogic::gripperStateChanged, this, [this](bool isOpen) {
        if (isOpen) {
            toggleGripperButton->setText("Fermer la pince");
        } else {
            toggleGripperButton->setText("Ouvrir la pince");
        }
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
    connect(rotateLeftButton,  &QPushButton::clicked, logic, &CalibrationLogic::rotateLeft);
    connect(rotateRightButton, &QPushButton::clicked, logic, &CalibrationLogic::rotateRight);

    // Connexion des boutons de déplacement fin en mode "joystick" (mouvement continu tant que maintenu)
    // 1.5mm pour X (précision), 3mm pour Y/Z (rapidité) toutes les 50ms
    connect(moveXPlusButton,  &QPushButton::pressed, this, [this]() { logic->startContinuousMove('x', +1.5f); });
    connect(moveXPlusButton,  &QPushButton::released, logic, &CalibrationLogic::stopContinuousMove);

    connect(moveXMinusButton, &QPushButton::pressed, this, [this]() { logic->startContinuousMove('x', -1.5f); });
    connect(moveXMinusButton, &QPushButton::released, logic, &CalibrationLogic::stopContinuousMove);

    connect(moveYPlusButton,  &QPushButton::pressed, this, [this]() { logic->startContinuousMove('y', +3.0f); });
    connect(moveYPlusButton,  &QPushButton::released, logic, &CalibrationLogic::stopContinuousMove);

    connect(moveYMinusButton, &QPushButton::pressed, this, [this]() { logic->startContinuousMove('y', -3.0f); });
    connect(moveYMinusButton, &QPushButton::released, logic, &CalibrationLogic::stopContinuousMove);

    connect(moveZPlusButton,  &QPushButton::pressed, this, [this]() { logic->startContinuousMove('z', +3.0f); });
    connect(moveZPlusButton,  &QPushButton::released, logic, &CalibrationLogic::stopContinuousMove);

    connect(moveZMinusButton, &QPushButton::pressed, this, [this]() { logic->startContinuousMove('z', -3.0f); });
    connect(moveZMinusButton, &QPushButton::released, logic, &CalibrationLogic::stopContinuousMove);

    // Connexion des boutons de déplacement automatique par zone
    connect(goToLeftReservoirButton,  &QPushButton::clicked, logic, &CalibrationLogic::goToLeftReservoirArea);
    connect(goToRightReservoirButton, &QPushButton::clicked, logic, &CalibrationLogic::goToRightReservoirArea);
    connect(goToGridButton,           &QPushButton::clicked, logic, &CalibrationLogic::goToGridArea);

    connect(restartButton, &QPushButton::clicked, this, &CalibrationScreen::onRestartClicked);

    connect(menuButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "[CalibrationScreen] Bouton menu cliqué - nettoyage avant de quitter";

        // Déconnexion propre avant de retourner au menu
        logic->disconnectToRobot();
        currentStep = 0;
        isConnecting = false;

        qDebug() << "[CalibrationScreen] Émission du signal backToMenuRequested";
        emit backToMenuRequested();
    });

    connect(retryButton, &QPushButton::clicked, this, &CalibrationScreen::attemptConnection);

    // === CRÉATION DU WIDGET DE CONFIRMATION ===
    createConfirmWidget();

    // === STACK GLOBAL (mainWidget + confirmWidget) ===
    mainStack = new QStackedWidget(this);
    mainStack->addWidget(mainWidget);
    mainStack->addWidget(confirmWidget);
    mainStack->setCurrentWidget(mainWidget);

    QVBoxLayout* globalLayout = new QVBoxLayout(this);
    globalLayout->setContentsMargins(0, 0, 0, 0);
    globalLayout->addWidget(mainStack);
    setLayout(globalLayout);

    // === DÉMARRAGE ===
    prepareIntroUI("Connexion au robot en cours...");
    showIntroLayout(true); // auto-connexion immédiate si visible, sinon à showEvent
}

// === showEvent : relance la connexion si l’intro est visible au (re)affichage ===
void CalibrationScreen::showEvent(QShowEvent* event) {
    QWidget::showEvent(event);
    if (stackedLayout->currentWidget() == introWidget) {
        QTimer::singleShot(0, this, &CalibrationScreen::attemptConnection);
    }
}

// === CONNEXION AU ROBOT ===
void CalibrationScreen::attemptConnection() {
    if (isConnecting) {
        qDebug() << "[CalibrationScreen] tentative ignorée: connexion déjà en cours";
        return;
    }
    isConnecting = true;

    introLabel->setText("Connexion au robot en cours...");
    loadingLabel->show();
    loadingMovie->start();
    retryButton->hide();
    startButton->hide();

    logic->connectToRobot();
}

void CalibrationScreen::onConnectionFinished(bool success) {
    isConnecting = false;

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

    // ⚠️ NE PAS appeler homeRobot() si l'écran n'est pas visible
    // (évite le conflit avec GameLogic ou CalibrationTestScreen)
    if (!this->isVisible()) {
        qDebug() << "[CalibrationScreen] Connexion réussie mais écran non visible, pas de Home()";
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

    introLabel->setText("Le robot est maintenant prêt.<br>"
                        "Les manipulations à effectuer seront affichées tout au long de la calibration.<br>"
                        "<br><em>La durée estimée est de 5 minutes.</em><br>");

    // Réappliquer complètement le style du bouton pour s'assurer qu'il s'affiche correctement
    styleButton(startButton);

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

void CalibrationScreen::onRestartClicked() {
    logic->resetCalibration();
    currentStep = 0;

    // Plus de remise en position initiale à la sortie (évite les conflits)
    // Le robot sera remis en position initiale au prochain lancement de calibration/partie
    prepareIntroUI("<b>Retour au menu...</b>");
    showIntroLayout(false);
    startButton->hide();
    progressBar->hide();

    // Émettre directement le signal de retour au menu
    emit backToMenuRequested();
}

// === OUTILS ===
void CalibrationScreen::fadeOut() {
    fadeAnimation->setStartValue(1.0);
    fadeAnimation->setEndValue(0.0);
    fadeAnimation->start();
}

void CalibrationScreen::fadeIn() {
    fadeAnimation->setStartValue(0.0);
    fadeAnimation->setEndValue(1.0);
    fadeAnimation->start();
}

void CalibrationScreen::onFadeAnimationFinished() {}

void CalibrationScreen::onLogicProgress(int value) {
    progressBar->setValue(value);
    emit progressUpdated(value);
}

void CalibrationScreen::prepareIntroUI(const QString& message) {
    stackedLayout->setCurrentWidget(introWidget);
    introLabel->setText(message);
    startButton->hide();
    retryButton->hide();
    progressBar->setRange(0, 8);
    progressBar->setValue(0);

    loadingMovie->stop();
    loadingLabel->hide();
}

void CalibrationScreen::showIntroLayout(bool autoConnect) {
    stackedLayout->setCurrentWidget(introWidget);

    if (autoConnect && isVisible()) {
        QTimer::singleShot(0, this, &CalibrationScreen::attemptConnection);
    }
}

void CalibrationScreen::showCalibrationLayout() {
    stackedLayout->setCurrentWidget(calibrationWidget);
}

void CalibrationScreen::showEndLayout() {
    stackedLayout->setCurrentWidget(endWidget);
}

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
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPainterPath path;
    path.addRoundedRect(original.rect(), 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, original);
    painter.end();

    QPixmap scaled = rounded.scaled(400, 400, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    label->setPixmap(scaled);
    label->setStyleSheet("background: transparent; border: none;");
}

// === CRÉATION DU WIDGET DE CONFIRMATION ===
void CalibrationScreen::createConfirmWidget() {
    confirmWidget = new QWidget(this);
    QVBoxLayout *outerLayout = new QVBoxLayout(confirmWidget);
    outerLayout->setContentsMargins(80, 80, 80, 80);
    outerLayout->setSpacing(0);

    QVBoxLayout *centerLayout = new QVBoxLayout;
    centerLayout->setSpacing(50);

    QLabel *title = new QLabel("Confirmation");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    QLabel *confirmLabel = new QLabel("Voulez-vous vraiment quitter la calibration ?<br>Les modifications en cours seront perdues.");
    confirmLabel->setAlignment(Qt::AlignCenter);
    confirmLabel->setStyleSheet("font-size: 30px; color: #333; padding: 40px 30px;");
    confirmLabel->setWordWrap(true);
    confirmLabel->setMinimumHeight(150);

    QPushButton *yesButton = new QPushButton("Oui");
    QPushButton *noButton = new QPushButton("Non");

    QList<QPushButton*> buttons = {yesButton, noButton};
    QList<QString> colors = {"#2ECC71", "#E74C3C"};

    for (int i = 0; i < buttons.size(); ++i) {
        buttons[i]->setFixedSize(200, 70);
        buttons[i]->setStyleSheet(QString(
                                      "QPushButton { background-color: %1; color: white; font-size: 24px; font-weight: bold; border-radius: 35px; }"
                                      "QPushButton:hover { background-color: #444; }"
                                      "QPushButton:pressed { background-color: #111; }"
                                      ).arg(colors[i]));
        buttons[i]->setCursor(Qt::PointingHandCursor);
    }

    connect(noButton, &QPushButton::clicked, this, &CalibrationScreen::returnToCalibration);
    connect(yesButton, &QPushButton::clicked, this, &CalibrationScreen::onQuitButtonClicked);

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(yesButton);
    buttonLayout->addSpacing(40);
    buttonLayout->addWidget(noButton);
    buttonLayout->addStretch();

    centerLayout->addWidget(title, 0, Qt::AlignCenter);
    centerLayout->addWidget(confirmLabel, 0, Qt::AlignCenter);
    centerLayout->addLayout(buttonLayout);

    outerLayout->addStretch();
    outerLayout->addLayout(centerLayout);
    outerLayout->addStretch();
}

// === NAVIGATION VERS CONFIRMATION ===
void CalibrationScreen::showConfirmationScreen() {
    mainStack->setCurrentWidget(confirmWidget);
}

void CalibrationScreen::returnToCalibration() {
    mainStack->setCurrentWidget(mainWidget);
}

void CalibrationScreen::onQuitButtonClicked() {
    qDebug() << "[CalibrationScreen] Bouton Oui cliqué - nettoyage avant de quitter";

    // Remettre sur le widget principal
    mainStack->setCurrentWidget(mainWidget);

    // IMPORTANT: Faire le reset/déconnexion AVANT d'émettre le signal
    // pour éviter les problèmes de threads et de double déconnexion
    qDebug() << "[CalibrationScreen] Déconnexion et reset de la calibration...";
    logic->disconnectToRobot();
    currentStep = 0;
    progressBar->setRange(0, 8);
    progressBar->setValue(0);
    progressBar->hide();
    loadingMovie->stop();
    loadingLabel->hide();
    isConnecting = false;

    // Émettre le signal pour retourner au menu
    qDebug() << "[CalibrationScreen] Émission du signal backToMenuRequested";
    emit backToMenuRequested();
    qDebug() << "[CalibrationScreen] Signal backToMenuRequested émis";
}

// === RESET COMPLET DE LA CALIBRATION (méthode publique) ===
void CalibrationScreen::resetCalibration() {
    // Reset logique
    logic->resetCalibration();
    currentStep = 0;

    // Reset UI
    progressBar->setRange(0, 8);
    progressBar->setValue(0);
    progressBar->hide();

    loadingMovie->stop();
    loadingLabel->hide();

    // Déconnexion propre
    logic->disconnectToRobot();

    // Autoriser la prochaine tentative
    isConnecting = false;

    // Revenir à l'intro
    prepareIntroUI("Connexion au robot en cours...");
    showIntroLayout(false);

    // Retourner au widget principal (seulement si on reste sur calibration)
    mainStack->setCurrentWidget(mainWidget);
}
