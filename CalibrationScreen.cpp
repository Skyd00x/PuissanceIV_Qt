#include "CalibrationScreen.hpp"
#include <QDebug>
#include <QMessageBox>
#include <algorithm> // pour std::clamp
#include <thread>
#include <chrono>

constexpr float X_MIN = -260.0f, X_MAX = 300.0f;
constexpr float Y_MIN = -250.0f, Y_MAX = 250.0f;
constexpr float Z_MIN = -140.0f, Z_MAX = 200.0f;
constexpr float R_MIN = -120.0f, R_MAX = 120.0f;

CalibrationScreen::CalibrationScreen(Robot *robot, QWidget *parent)
    : QWidget(parent), robot(robot)
{
    // === FOND BLANC ===
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
    progressBar->setFormat("%p%");    // ✅ affichage du pourcentage
    progressBar->setAlignment(Qt::AlignCenter);
    progressBar->setStyleSheet(
        "QProgressBar { background-color: #E0E0E0; border-radius: 12px; color: #1B3B5F; font-size: 18px; font-weight: bold; }"
        "QProgressBar::chunk { background-color: #4F8ED8; border-radius: 12px; }"
        );


    // === TEXTE DE CONSIGNE ===
    label = new QLabel("Connexion au robot en cours...", this);
    label->setAlignment(Qt::AlignCenter);
    label->setMaximumWidth(750);
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

    QList<QPushButton*> allButtons = {
        startButton, nextButton, backButton, toggleGripperButton, rotateLeftButton,
        rotateRightButton, testButton, restartButton, retryButton
    };
    for (auto *b : allButtons) {
        styleButton(b);
        b->setFixedSize(280, 60); // ✅ boutons plus petits
    }

    // === LAYOUT PRINCIPAL ===
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 40, 40, 40);
    mainLayout->setSpacing(15);

    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(progressBar, 0, Qt::AlignCenter);
    mainLayout->addSpacing(10);

    QHBoxLayout *contentLayout = new QHBoxLayout;
    contentLayout->setSpacing(50);

    QLabel *imageLabel = new QLabel(this);
    QPixmap img(":/assets/polytech.png");
    img = img.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    imageLabel->setPixmap(img);
    imageLabel->setAlignment(Qt::AlignCenter);
    contentLayout->addWidget(imageLabel, 1);

    QVBoxLayout *formLayout = new QVBoxLayout;
    formLayout->setSpacing(10);
    formLayout->addSpacing(80); // ✅ décale le formulaire vers le bas
    formLayout->addWidget(label, 0, Qt::AlignCenter);
    formLayout->addSpacing(15);

    QHBoxLayout *gripLayout = new QHBoxLayout;
    gripLayout->addStretch();
    gripLayout->addWidget(toggleGripperButton);
    gripLayout->addSpacing(10);
    gripLayout->addWidget(rotateLeftButton);
    gripLayout->addSpacing(10);
    gripLayout->addWidget(rotateRightButton);
    gripLayout->addStretch();
    formLayout->addLayout(gripLayout);

    QHBoxLayout *navLayout = new QHBoxLayout;
    navLayout->addStretch();
    navLayout->addWidget(backButton);
    navLayout->addSpacing(20);
    navLayout->addWidget(nextButton);
    navLayout->addStretch();

    formLayout->addWidget(startButton, 0, Qt::AlignCenter);
    formLayout->addLayout(navLayout);
    formLayout->addWidget(testButton, 0, Qt::AlignCenter);
    formLayout->addWidget(restartButton, 0, Qt::AlignCenter);
    formLayout->addWidget(retryButton, 0, Qt::AlignCenter);

    formLayout->addStretch(1);
    contentLayout->addLayout(formLayout, 2);
    mainLayout->addLayout(contentLayout);
    mainLayout->addStretch();
    setLayout(mainLayout);

    // === VISIBILITÉ INITIALE ===
    nextButton->hide();
    backButton->hide();
    toggleGripperButton->hide();
    rotateLeftButton->hide();
    rotateRightButton->hide();
    testButton->hide();
    restartButton->hide();
    startButton->hide();
    retryButton->hide();

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

    connect(robot, &Robot::calibrationStepChanged, this, &CalibrationScreen::updateStep);
    connect(robot, &Robot::calibrationFinished, this, [=](bool success){
        if (success) {
            nextButton->hide();
            backButton->hide();
            toggleGripperButton->hide();
            rotateLeftButton->hide();
            rotateRightButton->hide();
            testButton->show();
            restartButton->show();
            robot->turnOffGripper();
        }
    });

    connectionTimer = new QTimer(this);
    connectionTimer->setSingleShot(true);
    connect(connectionTimer, &QTimer::timeout, this, &CalibrationScreen::attemptConnection);
    connectionTimer->start(200);
}

// =====================================================
// === Connexion + HOME ===
// =====================================================
void CalibrationScreen::attemptConnection()
{
    label->setText("Connexion au robot en cours...");
    retryButton->hide();

    if (!robot) return;
    bool ok = robot->connect();
    QTimer::singleShot(1500, this, [this, ok]() { onConnectionFinished(ok); });
}

void CalibrationScreen::onConnectionFinished(bool success)
{
    if (!success) {
        label->setText("<b>Erreur :</b> Impossible de se connecter au robot.<br>"
                       "Vérifiez la connexion USB et cliquez sur 'Réessayer'.");
        retryButton->show();
        return;
    }

    label->setText("Robot connecté ! Retour en position initiale...");
    QApplication::processEvents();

    // 🕐 Retour Home exécuté dans un thread pour éviter de bloquer l’UI
    std::thread([this]() {
        robot->Home();  // mouvement initial + attente interne

        // Après Home terminé → afficher bouton "Commencer"
        QMetaObject::invokeMethod(this, [this]() {
            label->setText("✅ Robot prêt !<br><br>Cliquez sur 'Commencer la calibration' pour démarrer.");
            startButton->show();
        }, Qt::QueuedConnection);
    }).detach();
}


// =====================================================
// === Séquence calibration ===
// =====================================================
void CalibrationScreen::onStartClicked()
{
    startButton->hide();
    nextButton->show();
    backButton->show();

    progressBar->setValue(1);
    label->setText("Videz les réservoirs de pions, mais laissez un pion dans celui de gauche, en position 1.<br>"
                   "Cliquez sur 'Suivant' lorsque c’est prêt.");
}

void CalibrationScreen::onNextClicked()
{
    robot->nextCalibrationStep();
}

void CalibrationScreen::onBackClicked()
{
    int step = progressBar->value();

    if (step <= 1) {
        label->setText("Videz les réservoirs de pions, mais laissez un pion dans celui de gauche, en position 1.");
        return;
    }

    // Reculer d'une étape et informer le robot
    int previous = step - 1;
    progressBar->setValue(previous);

    // Met à jour currentStep côté robot
    robot->resetCalibration();
    for (int i = 1; i < previous; ++i)
        robot->recordCalibrationStep();

    // Récupération du texte correspondant
    QString message;
    switch (previous)
    {
    case 1:
        message = "Videz les réservoirs de pions, mais laissez un pion dans celui de gauche, en position 1.";
        break;
    case 2:
        message = "Attrapez le pion du réservoir de gauche avec la pince.";
        break;
    case 3:
        message = "Amenez le pion dans l'emplacement 4 du réservoir de gauche.";
        break;
    case 4:
        message = "Amenez le pion dans l'emplacement 1 du réservoir de droite.";
        break;
    case 5:
        message = "Amenez le pion dans l'emplacement 4 du réservoir de droite.";
        break;
    case 6:
        message = "Amenez le pion au-dessus de la colonne tout à gauche de la grille.";
        break;
    case 7:
        message = "Amenez le pion au-dessus de la colonne tout à droite de la grille.";
        break;
    default:
        message = "";
        break;
    }

    label->setText(QString("<b>Étape %1 / 7</b><br><br>%2").arg(previous).arg(message));

    // Gérer visibilité des boutons selon l’étape
    toggleGripperButton->setVisible(previous == 2);
    rotateLeftButton->setVisible(previous >= 2 && previous <= 7);
    rotateRightButton->setVisible(previous >= 2 && previous <= 7);
}

void CalibrationScreen::onToggleGripperClicked()
{
    static bool open = false;
    if (open) robot->closeGripper();
    else robot->openGripper();
    open = !open;
}

// ✅ Inversion rotation (corrigé)
void CalibrationScreen::onRotateLeftClicked()  { robot->rotate(+5); }
void CalibrationScreen::onRotateRightClicked() { robot->rotate(-5); }

void CalibrationScreen::onTestClicked()
{
    QList<QWidget*> toHide = { nextButton, backButton, toggleGripperButton,
                               rotateLeftButton, rotateRightButton,
                               testButton, restartButton, startButton };
    for (auto *w : toHide) w->hide();

    label->setText("Test des positions calibrées en cours, veuillez patienter...");
    progressBar->setRange(0, 100);
    progressBar->setValue(0);
    progressBar->setFormat("%p%");
    QApplication::processEvents();

    int totalSteps = 8 + 7 + 2;
    int done = 0;

    auto updateProgress = [&](int step) {
        int percent = static_cast<int>((float(step) / totalSteps) * 100.0f);
        progressBar->setValue(percent);
        QApplication::processEvents();
    };

    for (int i = 0; i < 4; ++i) { robot->goTo(robot->getPiecePose(i)); updateProgress(++done); }
    for (int i = 4; i < 8; ++i) { robot->goTo(robot->getPiecePose(i)); updateProgress(++done); }
    for (int i = 0; i < 7; ++i) { robot->goTo(robot->getColumnPose(i)); updateProgress(++done); }

    robot->goTo(robot->getPiecePose(0));
    updateProgress(++done);

    robot->openGripper();
    robot->wait(0.8f);
    robot->turnOffGripper();
    updateProgress(++done);

    progressBar->setValue(100);
    label->setText("✅ Test complet terminé ! Le pion a été relâché dans le réservoir gauche.");
    restartButton->show();
}

void CalibrationScreen::onRestartClicked()
{
    label->setText("Recommençons la calibration...");
    progressBar->setValue(1);
    nextButton->show();
    backButton->show();
    testButton->hide();
    restartButton->hide();
    toggleGripperButton->hide();
    rotateLeftButton->hide();
    rotateRightButton->hide();

    robot->resetCalibration();
    robot->turnOffGripper();

    label->setText("Videz les réservoirs de pions, mais laissez un pion dans celui de gauche, en position 1.");
}

void CalibrationScreen::updateStep(int step, const QString &message)
{
    progressBar->setValue(step);
    label->setText(QString("<b>Étape %1 / 7</b><br><br>%2").arg(step).arg(message));

    toggleGripperButton->setVisible(step == 2);
    rotateLeftButton->setVisible(step >= 2 && step <= 7);
    rotateRightButton->setVisible(step >= 2 && step <= 7);
}

void CalibrationScreen::styleButton(QPushButton *button, const QString &color1, const QString &color2)
{
    button->setStyleSheet(QString(
                              "QPushButton {"
                              " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                              " stop:0 %1, stop:1 %2);"
                              " color: white;"
                              " font-size: 24px;"
                              " font-weight: bold;"
                              " border: 2px solid #1B3B5F;"
                              " border-radius: 45px;"
                              " padding: 12px 30px;"
                              " min-width: 240px;"
                              " min-height: 80px;"
                              " text-align: center;"
                              " white-space: nowrap;"
                              "}"
                              "QPushButton:hover {"
                              " background: qlineargradient(x1:0, y1:0, x2:0, y2:1,"
                              " stop:0 #66B0FF, stop:1 #347AD1);"
                              "}"
                              "QPushButton:pressed { background-color: %2; }"
                              ).arg(color1, color2));

    button->setFont(QFont("Segoe UI", 18, QFont::Bold));
    button->setCursor(Qt::PointingHandCursor);

    auto *shadow = new QGraphicsDropShadowEffect(button);
    shadow->setBlurRadius(30);
    shadow->setOffset(3, 5);
    shadow->setColor(QColor(0, 0, 0, 120));
    button->setGraphicsEffect(shadow);
}
