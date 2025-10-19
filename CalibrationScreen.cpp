// CalibrationScreen.cpp
#include "CalibrationScreen.hpp"
#include <thread>
#include <chrono>
#include <algorithm>

// === CONSTRUCTEUR ===
CalibrationScreen::CalibrationScreen(Robot *robot, QWidget *parent)
    : QWidget(parent), robot(robot) {
    // Configuration de base
    setAutoFillBackground(true);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
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
        "QProgressBar { background-color: #E0E0E0; border-radius: 12px; color: #1B3B5F; font-size: 18px; font-weight: bold; text-align: center; }"
        "QProgressBar::chunk { background-color: #4F8ED8; border-radius: 12px; }"
        );

    // === INDICATEUR DE CHARGEMENT ===
    loadingLabel = new QLabel(this);
    loadingMovie = new QMovie("./Ressources/image/Gifs/simple_loading.gif");
    loadingLabel->setMovie(loadingMovie);
    loadingLabel->setFixedSize(85, 85);
    loadingLabel->setScaledContents(true);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->setStyleSheet("background: transparent;");
    loadingLabel->hide();

    // === TEXTE CONSIGNE ===
    label = new QLabel("Connexion au robot en cours...", this);
    label->setAlignment(Qt::AlignCenter);
    label->setWordWrap(true);
    label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    label->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold;");
    label->setMinimumHeight(160);

    // === BOUTONS ===
    startButton = new QPushButton("Commencer la calibration");
    nextButton = new QPushButton("Suivant");
    backButton = new QPushButton("Retour");
    toggleGripperButton = new QPushButton("Ouvrir / Fermer la pince");
    rotateLeftButton = new QPushButton("↺ Tourner gauche");
    rotateRightButton = new QPushButton("↻ Tourner droite");
    testButton = new QPushButton("Tester toutes les positions");
    restartButton = new QPushButton("Recommencer");
    retryButton = new QPushButton("Réessayer la connexion");
    menuButton = new QPushButton("Retour au menu principal");

    QList<QPushButton*> allButtons = {
        startButton, nextButton, backButton, toggleGripperButton, rotateLeftButton,
        rotateRightButton, testButton, restartButton, retryButton, menuButton
    };
    for (auto *b : allButtons) {
        styleButton(b);
        b->setFixedSize(200, 50);
    }

    // === IMAGE ===
    imageLabel = new QLabel(this);
    imageLabel->setAlignment(Qt::AlignCenter);
    imageLabel->setScaledContents(true);
    imageLabel->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    imageLabel->setFixedSize(500, 500);
    QPixmap original("./Ressources/image/Calibration/welcome_calibration.png");
    QPixmap rounded(original.size());
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(original.rect(), 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, original);
    painter.end();
    imageLabel->setPixmap(rounded);
    imageLabel->setStyleSheet("background: transparent; border: none;");
    auto *shadow = new QGraphicsDropShadowEffect(this);
    shadow->setOffset(5, 5);
    shadow->setColor(QColor(0, 0, 0, 0));
    imageLabel->setGraphicsEffect(shadow);

    // === LAYOUT PRINCIPAL ===
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(10);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);
    mainLayout->addWidget(progressBar, 0, Qt::AlignHCenter);
    mainLayout->addSpacing(30);

    // === CONTENU PRINCIPAL (IMAGE + FORMULAIRE) ===
    QHBoxLayout *contentLayout = new QHBoxLayout();
    contentLayout->setSpacing(30);

    // --- Colonne de gauche : Image ---
    imageContainer = new QWidget(this);
    QVBoxLayout *imgLayout = new QVBoxLayout(imageContainer);
    imgLayout->setContentsMargins(20, 0, 0, 0);
    imgLayout->addWidget(imageLabel, 0, Qt::AlignCenter);
    imgLayout->addStretch();

    // --- Colonne de droite : Formulaire ---
    formContainer = new QWidget(this);
    formContainer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QVBoxLayout *formLayout = new QVBoxLayout(formContainer);
    formLayout->setSpacing(15);
    formLayout->setAlignment(Qt::AlignTop);
    formLayout->addWidget(label, 0, Qt::AlignCenter);
    formLayout->addSpacing(15);
    formLayout->addWidget(loadingLabel, 0, Qt::AlignCenter);

    // Groupe pince/rotation
    QHBoxLayout *gripLayout = new QHBoxLayout();
    gripLayout->addStretch();
    gripLayout->addWidget(toggleGripperButton);
    gripLayout->addSpacing(10);
    gripLayout->addWidget(rotateLeftButton);
    gripLayout->addSpacing(10);
    gripLayout->addWidget(rotateRightButton);
    gripLayout->addStretch();
    formLayout->addLayout(gripLayout);

    // Bouton "Commencer"
    formLayout->addWidget(startButton, 0, Qt::AlignCenter);

    // Navigation (Retour / Suivant)
    QHBoxLayout *navLayout = new QHBoxLayout();
    navLayout->addStretch();
    navLayout->addWidget(backButton);
    navLayout->addSpacing(20);
    navLayout->addWidget(nextButton);
    navLayout->addStretch();
    formLayout->addLayout(navLayout);

    // Autres boutons
    formLayout->addWidget(testButton, 0, Qt::AlignCenter);
    formLayout->addWidget(restartButton, 0, Qt::AlignCenter);
    formLayout->addWidget(menuButton, 0, Qt::AlignCenter);
    formLayout->addWidget(retryButton, 0, Qt::AlignCenter);
    formLayout->addStretch();

    // Ajout des conteneurs au layout horizontal
    contentLayout->addWidget(imageContainer, 3);
    contentLayout->addWidget(formContainer, 7);
    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch();

    // === VISIBILITÉ INITIALE ===
    for (auto *b : allButtons) {
        b->hide();
    }

    // === ÉTAPES ===
    steps = {
        { "Videz les réservoirs, sauf un pion dans le réservoir de gauche à l'emplacement 1.",
         "./Ressources/image/Calibration/Etape1.png", true, false, false, false, false, false, false },
        { "Attrapez le pion du réservoir de gauche avec la pince du robot (Elle doit être fermée).",
         "./Ressources/image/Calibration/Etape2.png", true, true, true, true, false, false, false },
        { "Amenez le pion à l'emplacement 4 du réservoir de gauche.",
         "./Ressources/image/Calibration/Etape3.png", true, true, true, true, false, false, false },
        { "Amenez le pion à l'emplacement 1 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape4.png", true, true, true, true, false, false, false },
        { "Amenez le pion à l'emplacement 4 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape5.png", true, true, true, true, false, false, false },
        { "Amenez le pion à la colonne tout à gauche de la grille.",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Amenez le pion à la colonne tout à droite de la grille.",
         "./Ressources/image/Calibration/Etape7.png", true, true, true, true, false, false, false },
        { "Calibration terminée.<br>Vous pouvez maintenant tester les positions, recommencer ou quitter.",
         "./Ressources/image/welcome_calibration.png", false, false, false, false, true, true, true }
    };

    // === CONNEXIONS ===
    connect(startButton, &QPushButton::clicked, this, &CalibrationScreen::onStartClicked);
    connect(nextButton, &QPushButton::clicked, this, &CalibrationScreen::onNextClicked);
    connect(backButton, &QPushButton::clicked, this, &CalibrationScreen::onBackClicked);
    connect(toggleGripperButton, &QPushButton::clicked, this, &CalibrationScreen::onToggleGripperClicked);
    connect(rotateLeftButton, &QPushButton::clicked, this, &CalibrationScreen::onRotateLeftClicked);
    connect(rotateRightButton, &QPushButton::clicked, this, &CalibrationScreen::onRotateRightClicked);
    connect(testButton, &QPushButton::clicked, this, &CalibrationScreen::onTestClicked);
    connect(restartButton, &QPushButton::clicked, this, &CalibrationScreen::onRestartClicked);
    connect(retryButton, &QPushButton::clicked, this, &CalibrationScreen::attemptConnection);
    connect(menuButton, &QPushButton::clicked, this, [this]() {
        resetCalibration();
        progressBar->setValue(0);
        label->setText("Connexion au robot en cours...");
        startButton->hide();
        nextButton->hide();
        backButton->hide();
        toggleGripperButton->hide();
        rotateLeftButton->hide();
        rotateRightButton->hide();
        testButton->hide();
        restartButton->hide();
        retryButton->hide();
        menuButton->hide();
        emit backToMenu();
    });

    // === CONNEXION INITIALE ===
    connectionTimer = new QTimer(this);
    connectionTimer->setSingleShot(true);
    connect(connectionTimer, &QTimer::timeout, this, &CalibrationScreen::attemptConnection);
    connectionTimer->start(200);

    // === ANIMATION ===
    fadeAnimation = new QPropertyAnimation(this, "opacity");
    fadeAnimation->setDuration(300); // Durée de l'animation en ms
    connect(fadeAnimation, &QPropertyAnimation::finished, this, &CalibrationScreen::onFadeAnimationFinished);

    loadCalibration();
}

// === MÉTHODES D'ANIMATION ===
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

void CalibrationScreen::onFadeAnimationFinished() {
    if (fadeAnimation->endValue() == 0.0) {
        // Mise à jour de l'UI après le fade-out
        updateStepUI();
        fadeIn();
    }
}

// === MÉTHODES DE CALIBRATION ===
void CalibrationScreen::onStartClicked() {
    currentStep = 0;
    progressBar->setValue(currentStep);
    startButton->hide();
    fadeOut();
}

void CalibrationScreen::onNextClicked() {
    recordCalibrationStep();
    currentStep++;
    progressBar->setValue(currentStep);
    fadeOut();
}

void CalibrationScreen::onBackClicked() {
    if (currentStep > 0) {
        currentStep--;
        progressBar->setValue(currentStep);
        fadeOut();
    }
}

void CalibrationScreen::updateStepUI() {
    if (currentStep < 0 || currentStep >= static_cast<int>(steps.size())) return;
    const CalibrationStep &s = steps[currentStep];
    label->setText(s.text);
    QPixmap original(s.imagePath);
    QPixmap rounded(original.size());
    rounded.fill(Qt::transparent);
    QPainter painter(&rounded);
    painter.setRenderHint(QPainter::Antialiasing);
    QPainterPath path;
    path.addRoundedRect(original.rect(), 40, 40);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, original);
    painter.end();
    imageLabel->setPixmap(rounded);
    imageLabel->setStyleSheet("background: transparent; border: none;");
    nextButton->setVisible(s.showNext);
    backButton->setVisible(s.showBack);
    toggleGripperButton->setVisible(s.showGripper);
    rotateLeftButton->setVisible(s.showRotation);
    rotateRightButton->setVisible(s.showRotation);
    testButton->setVisible(s.showTest);
    restartButton->setVisible(s.showRestart);
    menuButton->setVisible(s.showMenu);
}

// === AUTRES MÉTHODES ===
void CalibrationScreen::onToggleGripperClicked() {
    static bool open = false;
    if (open) robot->closeGripper();
    else robot->openGripper();
    open = !open;
}

void CalibrationScreen::onRotateLeftClicked() {
    robot->rotate(+5);
}

void CalibrationScreen::onRotateRightClicked() {
    robot->rotate(-5);
}

void CalibrationScreen::onTestClicked() {
    label->setText("Test des positions calibrées en cours...");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    int totalSteps = 7;
    for (int i = 0; i < 7; ++i) {
        robot->goTo(robot->getColumnPose(i));
        progressBar->setValue(static_cast<int>((float(i)/totalSteps)*100));
        QApplication::processEvents();
    }
    label->setText("Test terminé ! Vous pouvez recommencer, ou quitter si tout est ok.");
}

void CalibrationScreen::onRestartClicked() {
    resetCalibration();
    label->setText("Calibration réinitialisée.<br>Cliquez sur 'Commencer' pour recommencer.");
    startButton->show();
    testButton->hide();
    restartButton->hide();
    menuButton->hide();
    toggleGripperButton->hide();
    rotateLeftButton->hide();
    rotateRightButton->hide();
    backButton->hide();
    nextButton->hide();
}

void CalibrationScreen::resetCalibration() {
    currentStep = 0;
}

void CalibrationScreen::styleButton(QPushButton *button, const QString &c1, const QString &c2) {
    button->setStyleSheet(QString(
                              "QPushButton {"
                              "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
                              "   color: white;"
                              "   font-size: 16px;"
                              "   font-weight: bold;"
                              "   border: 2px solid #1B3B5F;"
                              "   border-radius: 30px;"
                              "   padding: 8px 16px;"
                              "   min-width: 120px;"
                              "   min-height: 40px;"
                              "   max-width: 200px;"
                              "   text-align: center;"
                              "   white-space: normal;"
                              "   word-wrap: break-word;"
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

// === CONNEXION AU ROBOT ===
void CalibrationScreen::attemptConnection() {
    label->setText("Connexion au robot en cours...");
    loadingLabel->show();
    loadingMovie->start();
    retryButton->hide();
    if (!robot) return;
    bool ok = robot->connect();
    QTimer::singleShot(1500, this, [this, ok]() { onConnectionFinished(ok); });
}

void CalibrationScreen::onConnectionFinished(bool success) {
    loadingMovie->stop();
    loadingLabel->hide();
    if (!success) {
        label->setText("<b>Impossible de se connecter au robot.<br>Vérifiez la connexion USB et cliquez sur 'Réessayer'.");
        retryButton->show();
        return;
    }
    label->setText("Retour en position initiale du robot, veuillez patienter...");
    QApplication::processEvents();
    std::thread([this]() {
        robot->Home();
        int stableCount = 0;
        while (true) {
            if (!robot->isMoving()) stableCount++;
            else stableCount = 0;
            if (stableCount >= 5) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
        QMetaObject::invokeMethod(this, [this]() {
            label->setText("Le robot est prêt.<br>Cliquez sur 'Commencer la calibration' pour démarrer.");
            startButton->show();
        }, Qt::QueuedConnection);
    }).detach();
}

void CalibrationScreen::recordCalibrationStep() {
    Pose p;
    GetPose(&p);
}

void CalibrationScreen::applyCalibration() {}

void CalibrationScreen::saveCalibration(const QString &path) {
    QJsonObject o;
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(o).toJson());
}

void CalibrationScreen::loadCalibration(const QString &path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
}
