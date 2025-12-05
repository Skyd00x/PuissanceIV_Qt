#include "GameScreen.hpp"
#include <QPixmap>
#include <QFont>
#include <QPalette>

GameScreen::GameScreen(QWidget *parent)
    : QWidget(parent)
{
    // === FOND BLANC ===
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // === STACK PRINCIPAL ===
    stack = new QStackedWidget(this);
    createInitializingWidget();
    createGameWidget();
    createConfirmWidget();

    stack->addWidget(initializingWidget);
    stack->addWidget(gameWidget);
    stack->addWidget(confirmWidget);
    stack->setCurrentWidget(gameWidget);

    // === LAYOUT GLOBAL ===
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->addWidget(stack);
    setLayout(mainLayout);

    // === CONNEXIONS TIMERS ===
    connect(&chronometer, &QTimer::timeout, this, &GameScreen::updateChronometer);
    connect(&countdownTimer, &QTimer::timeout, this, &GameScreen::updateCountdown);

    // Timer pour activer les avertissements de grille après un délai
    gridWarningDelayTimer.setSingleShot(true);
    connect(&gridWarningDelayTimer, &QTimer::timeout, [this]() {
        allowGridWarning = true;
    });
}

// ============================================================
// CRÉATION DU WIDGET DE JEU
// ============================================================
void GameScreen::createGameWidget()
{
    gameWidget = new QWidget(this);

    // ============================
    //  BOUTON QUITTER
    // ============================
    quitButton = new QPushButton("Quitter");
    quitButton->setFixedSize(160, 55);
    quitButton->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 27px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
        );
    quitButton->setCursor(Qt::PointingHandCursor);
    connect(quitButton, &QPushButton::clicked, this, &GameScreen::showConfirmationScreen);

    // Ombre
    auto *shadowQuit = new QGraphicsDropShadowEffect;
    shadowQuit->setBlurRadius(20);
    shadowQuit->setOffset(3, 3);
    shadowQuit->setColor(QColor(0, 0, 0, 80));
    quitButton->setGraphicsEffect(shadowQuit);

    // ============================
    //   TITRE CENTRAL
    // ============================
    titleLabel = new QLabel("Partie en mode ");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 50px; font-weight: bold; color: #1B3B5F;");

    // ============================
    //   CHRONOMÈTRE (EN HAUT DROITE)
    // ============================
    timerLabel = new QLabel("00:00");
    timerLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    timerLabel->setStyleSheet("font-size: 35px; font-weight: bold; color: #1B3B5F;");
    timerLabel->setFixedWidth(150);

    // ============================
    //   SOUS-TITRE (tour joueur/robot)
    // ============================
    turnLabel = new QLabel("");
    turnLabel->setAlignment(Qt::AlignCenter);
    turnLabel->setStyleSheet("font-size: 35px; font-weight: bold; color: #1B3B5F;");

    // ============================
    //   IMAGE CAMÉRA
    // ============================
    cameraLabel = new QLabel;
    cameraLabel->setMinimumSize(800, 450);
    cameraLabel->setStyleSheet("background-color: #F0F0F0; border: 2px solid #CCC;");
    cameraLabel->setAlignment(Qt::AlignCenter);

    // ============================
    //   COMPTE À REBOURS
    // ============================
    countdownTextLabel = new QLabel("Lancement de la partie dans");
    countdownTextLabel->setAlignment(Qt::AlignCenter);
    countdownTextLabel->setStyleSheet("font-size: 45px; font-weight: bold; color: #1B3B5F;");
    countdownTextLabel->hide();

    countdownLabel = new QLabel("3");
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet("font-size: 120px; font-weight: bold; color: #1B3B5F;");
    countdownLabel->hide();

    // ============================
    //   OVERLAY MESSAGE D'AVERTISSEMENT
    // ============================
    // Créer l'overlay qui se superpose à tout
    warningOverlay = new QWidget(gameWidget);
    warningOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 100);");  // Fond semi-transparent
    warningOverlay->hide();
    warningOverlay->raise();  // Met l'overlay au-dessus de tout

    // Layout pour centrer le message dans l'overlay
    QVBoxLayout *overlayLayout = new QVBoxLayout(warningOverlay);
    overlayLayout->setAlignment(Qt::AlignCenter);

    warningLabel = new QLabel();
    warningLabel->setAlignment(Qt::AlignCenter);
    warningLabel->setStyleSheet(
        "background-color: rgba(255, 140, 0, 230);"
        "color: white;"
        "font-size: 28px;"
        "font-weight: bold;"
        "padding: 25px 35px;"
        "border-radius: 15px;"
        "border: 4px solid rgba(255, 100, 0, 255);"
        );
    warningLabel->setWordWrap(true);
    warningLabel->setMaximumWidth(1100);

    // Ombre pour le message
    auto *warningShadow = new QGraphicsDropShadowEffect;
    warningShadow->setBlurRadius(40);
    warningShadow->setOffset(0, 8);
    warningShadow->setColor(QColor(0, 0, 0, 180));
    warningLabel->setGraphicsEffect(warningShadow);

    warningQuitButton = new QPushButton("Quitter la partie");
    warningQuitButton->setFixedSize(250, 70);
    warningQuitButton->setStyleSheet(
        "QPushButton { background-color: #E74C3C; color: white; font-size: 24px; font-weight: bold; border-radius: 35px; }"
        "QPushButton:hover { background-color: #C0392B; }"
        "QPushButton:pressed { background-color: #A93226; }"
        );
    warningQuitButton->setCursor(Qt::PointingHandCursor);
    connect(warningQuitButton, &QPushButton::clicked, this, &GameScreen::onQuitButtonClicked);

    overlayLayout->addWidget(warningLabel);
    overlayLayout->addSpacing(30);
    overlayLayout->addWidget(warningQuitButton, 0, Qt::AlignCenter);


    // ============================
    //   OVERLAY TRICHE DÉTECTÉE
    // ============================
    cheatOverlay = new QWidget(gameWidget);
    cheatOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 150);");
    cheatOverlay->hide();

    QVBoxLayout *cheatLayout = new QVBoxLayout(cheatOverlay);
    cheatLayout->setAlignment(Qt::AlignCenter);

    cheatLabel = new QLabel();
    cheatLabel->setAlignment(Qt::AlignCenter);
    cheatLabel->setStyleSheet(
        "background-color: rgba(231, 76, 60, 230);"
        "color: white;"
        "font-size: 32px;"
        "font-weight: bold;"
        "padding: 35px 120px;"
        "border-radius: 15px;"
        );
    cheatLabel->setWordWrap(true);
    cheatLabel->setMinimumWidth(1000);

    auto *cheatShadow = new QGraphicsDropShadowEffect;
    cheatShadow->setBlurRadius(40);
    cheatShadow->setOffset(0, 8);
    cheatShadow->setColor(QColor(0, 0, 0, 180));
    cheatLabel->setGraphicsEffect(cheatShadow);

    cheatQuitButton = new QPushButton("Quitter la partie");
    cheatQuitButton->setFixedSize(250, 70);
    cheatQuitButton->setStyleSheet(
        "QPushButton { background-color: #1B3B5F; color: white; font-size: 24px; font-weight: bold; border-radius: 35px; }"
        "QPushButton:hover { background-color: #2C4D6F; }"
        "QPushButton:pressed { background-color: #0A1B2F; }"
        );
    cheatQuitButton->setCursor(Qt::PointingHandCursor);
    connect(cheatQuitButton, &QPushButton::clicked, this, &GameScreen::onQuitButtonClicked);

    cheatLayout->addWidget(cheatLabel);
    cheatLayout->addSpacing(30);
    cheatLayout->addWidget(cheatQuitButton, 0, Qt::AlignCenter);

    // ============================
    //   OVERLAY RÉSERVOIRS VIDES
    // ============================
    reservoirOverlay = new QWidget(gameWidget);
    reservoirOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 150);");
    reservoirOverlay->hide();

    QVBoxLayout *reservoirLayout = new QVBoxLayout(reservoirOverlay);
    reservoirLayout->setAlignment(Qt::AlignCenter);

    reservoirLabel = new QLabel("RÉSERVOIRS VIDES\nVeuillez remplir les réservoirs de pions");
    reservoirLabel->setAlignment(Qt::AlignCenter);
    reservoirLabel->setStyleSheet(
        "background-color: rgba(241, 196, 15, 230);"
        "color: white;"
        "font-size: 32px;"
        "font-weight: bold;"
        "padding: 35px 120px;"
        "border-radius: 15px;"
        );
    reservoirLabel->setWordWrap(true);
    reservoirLabel->setMinimumWidth(1000);

    auto *reservoirShadow = new QGraphicsDropShadowEffect;
    reservoirShadow->setBlurRadius(40);
    reservoirShadow->setOffset(0, 8);
    reservoirShadow->setColor(QColor(0, 0, 0, 180));
    reservoirLabel->setGraphicsEffect(reservoirShadow);

    reservoirRefillButton = new QPushButton("C'est fait !");
    reservoirRefillButton->setFixedSize(250, 70);
    reservoirRefillButton->setStyleSheet(
        "QPushButton { background-color: #2ECC71; color: white; font-size: 24px; font-weight: bold; border-radius: 35px; }"
        "QPushButton:hover { background-color: #27AE60; }"
        "QPushButton:pressed { background-color: #1E8449; }"
        );
    reservoirRefillButton->setCursor(Qt::PointingHandCursor);
    connect(reservoirRefillButton, &QPushButton::clicked, [this]() {
        reservoirOverlay->hide();
        emit reservoirsRefilled();
    });

    reservoirLayout->addWidget(reservoirLabel);
    reservoirLayout->addSpacing(30);
    reservoirLayout->addWidget(reservoirRefillButton, 0, Qt::AlignCenter);

    // ============================
    //   OVERLAY RÉSULTAT DE LA PARTIE
    // ============================
    resultOverlay = new QWidget(gameWidget);
    resultOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 180);");
    resultOverlay->hide();

    QVBoxLayout *resultLayout = new QVBoxLayout(resultOverlay);
    resultLayout->setAlignment(Qt::AlignCenter);

    resultLabel = new QLabel();
    resultLabel->setAlignment(Qt::AlignCenter);
    resultLabel->setStyleSheet(
        "background-color: rgba(27, 59, 95, 240);"
        "color: white;"
        "font-size: 36px;"
        "font-weight: bold;"
        "padding: 40px 100px;"
        "border-radius: 20px;"
        );
    resultLabel->setWordWrap(true);
    resultLabel->setMinimumWidth(1100);

    auto *resultShadow = new QGraphicsDropShadowEffect;
    resultShadow->setBlurRadius(50);
    resultShadow->setOffset(0, 10);
    resultShadow->setColor(QColor(0, 0, 0, 200));
    resultLabel->setGraphicsEffect(resultShadow);

    resultQuitButton = new QPushButton("Retour au menu");
    resultQuitButton->setFixedSize(300, 80);
    resultQuitButton->setStyleSheet(
        "QPushButton { background-color: #2ECC71; color: white; font-size: 26px; font-weight: bold; border-radius: 40px; }"
        "QPushButton:hover { background-color: #27AE60; }"
        "QPushButton:pressed { background-color: #1E8449; }"
        );
    resultQuitButton->setCursor(Qt::PointingHandCursor);
    connect(resultQuitButton, &QPushButton::clicked, this, &GameScreen::onQuitButtonClicked);

    resultLayout->addWidget(resultLabel);
    resultLayout->addSpacing(40);
    resultLayout->addWidget(resultQuitButton, 0, Qt::AlignCenter);

    // ============================
    //   OVERLAY ERREUR DE CONNEXION ROBOT
    // ============================
    connectionErrorOverlay = new QWidget(gameWidget);
    connectionErrorOverlay->setStyleSheet("background-color: rgba(0, 0, 0, 180);");
    connectionErrorOverlay->hide();

    QVBoxLayout *connectionErrorLayout = new QVBoxLayout(connectionErrorOverlay);
    connectionErrorLayout->setAlignment(Qt::AlignCenter);

    connectionErrorLabel = new QLabel("ERREUR DE CONNEXION\nImpossible de se connecter au robot");
    connectionErrorLabel->setAlignment(Qt::AlignCenter);
    connectionErrorLabel->setStyleSheet(
        "background-color: rgba(231, 76, 60, 230);"
        "color: white;"
        "font-size: 32px;"
        "font-weight: bold;"
        "padding: 35px 120px;"
        "border-radius: 15px;"
        );
    connectionErrorLabel->setWordWrap(true);
    connectionErrorLabel->setMinimumWidth(1000);

    auto *connectionErrorShadow = new QGraphicsDropShadowEffect;
    connectionErrorShadow->setBlurRadius(40);
    connectionErrorShadow->setOffset(0, 8);
    connectionErrorShadow->setColor(QColor(0, 0, 0, 180));
    connectionErrorLabel->setGraphicsEffect(connectionErrorShadow);

    retryConnectionButton = new QPushButton("Réessayer la connexion");
    retryConnectionButton->setFixedSize(350, 70);
    retryConnectionButton->setStyleSheet(
        "QPushButton { background-color: #2ECC71; color: white; font-size: 24px; font-weight: bold; border-radius: 35px; }"
        "QPushButton:hover { background-color: #27AE60; }"
        "QPushButton:pressed { background-color: #1E8449; }"
        );
    retryConnectionButton->setCursor(Qt::PointingHandCursor);
    connect(retryConnectionButton, &QPushButton::clicked, [this]() {
        connectionErrorOverlay->hide();
        emit prepareGame();  // Réessayer la connexion
    });

    quitFromConnectionErrorButton = new QPushButton("Retour au menu");
    quitFromConnectionErrorButton->setFixedSize(250, 70);
    quitFromConnectionErrorButton->setStyleSheet(
        "QPushButton { background-color: #E74C3C; color: white; font-size: 24px; font-weight: bold; border-radius: 35px; }"
        "QPushButton:hover { background-color: #C0392B; }"
        "QPushButton:pressed { background-color: #A93226; }"
        );
    quitFromConnectionErrorButton->setCursor(Qt::PointingHandCursor);
    connect(quitFromConnectionErrorButton, &QPushButton::clicked, this, &GameScreen::onQuitButtonClicked);

    QHBoxLayout *connectionErrorButtonLayout = new QHBoxLayout;
    connectionErrorButtonLayout->addWidget(retryConnectionButton);
    connectionErrorButtonLayout->addSpacing(30);
    connectionErrorButtonLayout->addWidget(quitFromConnectionErrorButton);

    connectionErrorLayout->addWidget(connectionErrorLabel);
    connectionErrorLayout->addSpacing(30);
    connectionErrorLayout->addLayout(connectionErrorButtonLayout);

    // Layout pour le countdown centré
    QVBoxLayout *countdownLayout = new QVBoxLayout;
    countdownLayout->addStretch();
    countdownLayout->addWidget(countdownTextLabel, 0, Qt::AlignCenter);
    countdownLayout->addSpacing(20);
    countdownLayout->addWidget(countdownLabel, 0, Qt::AlignCenter);
    countdownLayout->addStretch();

    // Widget pour le countdown (pour le centrage absolu)
    QWidget *countdownWidget = new QWidget;
    countdownWidget->setLayout(countdownLayout);

    // ============================
    //   LAYOUT TOP
    // ============================
    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->setContentsMargins(20, 20, 20, 0);
    topBar->addWidget(quitButton, 0, Qt::AlignLeft | Qt::AlignTop);
    topBar->addWidget(titleLabel, 1, Qt::AlignCenter);
    topBar->addWidget(timerLabel, 0, Qt::AlignRight | Qt::AlignTop);

    // ============================
    //   LAYOUT GLOBAL DU WIDGET
    // ============================
    QVBoxLayout *main = new QVBoxLayout(gameWidget);
    main->setContentsMargins(20, 20, 20, 20);
    main->setSpacing(20);

    main->addLayout(topBar);
    main->addWidget(turnLabel);
    main->addWidget(cameraLabel, 1);
    main->addWidget(countdownWidget, 1);
    main->addStretch();

    // ============================
    //   INITIALISATION : tout est caché
    // ============================
    quitButton->hide();
    titleLabel->hide();
    timerLabel->hide();
    turnLabel->hide();
    cameraLabel->hide();
    countdownTextLabel->hide();
    countdownLabel->hide();
}

// ============================================================
// CRÉATION DU WIDGET DE CONFIRMATION
// ============================================================
void GameScreen::createConfirmWidget()
{
    confirmWidget = new QWidget(this);
    QVBoxLayout *outerLayout = new QVBoxLayout(confirmWidget);
    outerLayout->setContentsMargins(80, 80, 80, 80);
    outerLayout->setSpacing(0);

    QVBoxLayout *centerLayout = new QVBoxLayout;
    centerLayout->setSpacing(50);

    QLabel *title = new QLabel("Confirmation");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    QLabel *confirmLabel = new QLabel("Voulez-vous vraiment quitter la partie en cours ?");
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

    connect(noButton, &QPushButton::clicked, this, &GameScreen::returnToGame);
    connect(yesButton, &QPushButton::clicked, this, &GameScreen::onQuitButtonClicked);

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

// ============================================================
// CRÉATION DU WIDGET D'INITIALISATION
// ============================================================
void GameScreen::createInitializingWidget()
{
    initializingWidget = new QWidget(this);

    QVBoxLayout* layout = new QVBoxLayout(initializingWidget);
    layout->setAlignment(Qt::AlignCenter);
    layout->setSpacing(40);
    layout->setContentsMargins(100, 50, 100, 50);

    // Label de statut (comme dans CalibrationScreen)
    initializingLabel = new QLabel("Mise en position initiale du robot...", initializingWidget);
    initializingLabel->setAlignment(Qt::AlignCenter);
    initializingLabel->setWordWrap(true);
    initializingLabel->setStyleSheet("font-size: 26px; color: #1B3B5F; font-weight: bold;");
    initializingLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    initializingLabel->setMinimumWidth(900);

    // Animation de chargement (comme dans CalibrationScreen)
    initializingLoadingLabel = new QLabel(initializingWidget);
    initializingLoadingMovie = new QMovie("./Ressources/image/Gifs/simple_loading.gif");
    initializingLoadingLabel->setMovie(initializingLoadingMovie);
    initializingLoadingLabel->setFixedSize(85, 85);
    initializingLoadingLabel->setScaledContents(true);
    initializingLoadingLabel->setAlignment(Qt::AlignCenter);

    layout->addWidget(initializingLabel, 0, Qt::AlignCenter);
    layout->addWidget(initializingLoadingLabel, 0, Qt::AlignCenter);
}

// ============================================================
// RÉINITIALISATION COMPLÈTE DE L'ÉCRAN DE JEU
// ============================================================
void GameScreen::resetGame()
{
    qDebug() << "[GameScreen] === RESET GAME ===";

    // Arrêter tous les timers
    chronometer.stop();
    countdownTimer.stop();
    gridWarningDelayTimer.stop();

    // Réinitialiser le chronomètre
    elapsedSeconds = 0;
    timerLabel->setText("00:00");

    // Réinitialiser les flags
    allowGridWarning = false;
    countdownValue = 3;

    // Cacher tous les overlays
    resetAllOverlays();
    cheatOverlay->hide();
    reservoirOverlay->hide();
    resultOverlay->hide();
    connectionErrorOverlay->hide();

    // Réinitialiser les labels
    titleLabel->setText("Partie en mode ");
    turnLabel->setText("");
    cameraLabel->clear();

    // S'assurer que le gameWidget est visible
    gameWidget->show();

    // Retour au widget de jeu
    stack->setCurrentWidget(gameWidget);
    qDebug() << "[GameScreen] gameWidget is now current widget";
}

// ============================================================
// DÉMARRAGE DU JEU (à appeler quand on affiche GameScreen)
// ============================================================
void GameScreen::startGame()
{
    qDebug() << "[GameScreen] === START GAME ===";

    // Réinitialiser complètement l'écran avant de commencer
    resetGame();

    // Tout est caché pendant le countdown
    quitButton->hide();
    titleLabel->hide();
    timerLabel->hide();
    turnLabel->hide();
    cameraLabel->hide();

    qDebug() << "[GameScreen] Émission du signal prepareGame()";

    // Préparer le robot AVANT le countdown (connexion + Home)
    emit prepareGame();

    // Le countdown sera lancé automatiquement quand le robot sera prêt
    qDebug() << "[GameScreen] En attente de robotInitialized() pour démarrer le countdown";
    // via le signal robotInitialized connecté à startCountdownWhenReady()
}

// ============================================================
// DÉMARRAGE DU COUNTDOWN QUAND LE ROBOT EST PRÊT
// ============================================================
void GameScreen::startCountdownWhenReady()
{
    // Réinitialiser le flag d'avertissement de grille
    allowGridWarning = false;

    // Lancer le countdown maintenant que le robot est prêt
    startCountdown();
}

// ============================================================
// COUNTDOWN : 3 → 2 → 1 → GO
// ============================================================
void GameScreen::startCountdown()
{
    countdownValue = 3;
    countdownTextLabel->show();
    countdownLabel->setText("3");
    countdownLabel->show();

    countdownTimer.start(1000);
}

void GameScreen::updateCountdown()
{
    countdownValue--;
    if (countdownValue == 0)
    {
        countdownLabel->setText("GO !");
        countdownTimer.stop();
        QTimer::singleShot(800, [this]() {
            // Cacher le countdown et afficher l'interface de jeu
            countdownTextLabel->hide();
            countdownLabel->hide();
            quitButton->show();
            titleLabel->show();
            timerLabel->show();
            turnLabel->show();
            cameraLabel->show();

            // Démarrer le chronomètre et la partie
            chronometer.start(1000);
            emit countdownFinished();

            // Activer les avertissements de grille après 5 secondes
            // (temps pour que la caméra s'initialise correctement)
            gridWarningDelayTimer.start(5000);
        });
    }
    else
    {
        countdownLabel->setText(QString::number(countdownValue));
    }
}

// ============================================================
//   CHRONOMÈTRE LOGIQUE
// ============================================================
void GameScreen::updateChronometer()
{
    elapsedSeconds++;
    int m = elapsedSeconds / 60;
    int s = elapsedSeconds % 60;
    timerLabel->setText(QString("%1:%2")
                            .arg(m, 2, 10, QChar('0'))
                            .arg(s, 2, 10, QChar('0')));
}

// ============================================================
//   SLOTS APPELÉS PAR GAMELOGIC
// ============================================================
void GameScreen::updateCameraFrame(const QImage &img)
{
    if (!img.isNull()) {
        // Utiliser la taille réelle du label, ou une taille par défaut si le layout n'est pas prêt
        QSize targetSize = cameraLabel->size();
        if (targetSize.width() <= 0 || targetSize.height() <= 0) {
            // Fallback : utiliser la taille minimale définie
            targetSize = QSize(800, 450);
        }

        cameraLabel->setPixmap(QPixmap::fromImage(img).scaled(
            targetSize,
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
        ));
    }
}

void GameScreen::setPlayerColor(int color)
{
    playerColor = color;
}

void GameScreen::setTurnPlayer()
{
    // Couleur du joueur : rouge si playerColor=1, jaune si playerColor=2
    QString color = (playerColor == 1) ? "#B22222" : "#EFCB00";
    turnLabel->setText("Au tour du joueur");
    turnLabel->setStyleSheet(QString("font-size: 35px; font-weight: bold; color: %1;").arg(color));
}

void GameScreen::setTurnRobot()
{
    // Couleur du robot : jaune si playerColor=1 (donc robot=2), rouge si playerColor=2 (donc robot=1)
    QString color = (playerColor == 1) ? "#EFCB00" : "#B22222";
    turnLabel->setText("Au tour du robot");
    turnLabel->setStyleSheet(QString("font-size: 35px; font-weight: bold; color: %1;").arg(color));
}

void GameScreen::setRobotStatus(const QString &status)
{
    // Couleur du robot : jaune si playerColor=1 (donc robot=2), rouge si playerColor=2 (donc robot=1)
    QString color = (playerColor == 1) ? "#EFCB00" : "#B22222";
    turnLabel->setText(QString("Au tour du robot : %1").arg(status));
    turnLabel->setStyleSheet(QString("font-size: 35px; font-weight: bold; color: %1;").arg(color));
}

void GameScreen::setDifficultyText(const QString &txt)
{
    titleLabel->setText(QString("Partie en mode %1").arg(txt));
}

void GameScreen::showEndOfGame(const QString &winnerText, int totalSeconds)
{
    chronometer.stop();

    QString final = winnerText +
                    QString("\nTemps total : %1:%2")
                        .arg(totalSeconds / 60, 2, 10, QChar('0'))
                        .arg(totalSeconds % 60, 2, 10, QChar('0'));

    turnLabel->setText(final);
    turnLabel->setStyleSheet("font-size: 40px; font-weight: bold; color: #1B3B5F;");
}

// ============================================================
//   NAVIGATION ENTRE ÉCRANS
// ============================================================
void GameScreen::showConfirmationScreen()
{
    stack->setCurrentWidget(confirmWidget);
}

void GameScreen::returnToGame()
{
    stack->setCurrentWidget(gameWidget);
}

void GameScreen::onQuitButtonClicked()
{
    emit quitRequested();
}

// ============================================================
//   AFFICHAGE AVERTISSEMENT GRILLE INCOMPLÈTE
// ============================================================
void GameScreen::showGridIncompleteWarning(int detectedCount)
{
    // Ne pas afficher l'avertissement trop tôt (pendant l'initialisation de la caméra)
    if (!allowGridWarning) {
        return;
    }

    warningLabel->setText(QString("GRILLE INCOMPLÈTE - %1/42 cases détectées\nAjustez le cadrage ou l'éclairage").arg(detectedCount));

    // Redimensionner l'overlay pour qu'il prenne toute la taille du gameWidget
    warningOverlay->setGeometry(gameWidget->rect());
    warningOverlay->show();
    warningOverlay->raise();  // S'assurer qu'il est au-dessus
}

// ============================================================
//   AFFICHAGE MISE EN POSITION INITIALE
// ============================================================
void GameScreen::showRobotInitializing()
{
    initializingLoadingMovie->start();
    stack->setCurrentWidget(initializingWidget);
}

void GameScreen::hideRobotInitializing()
{
    initializingLoadingMovie->stop();
    stack->setCurrentWidget(gameWidget);
}

// ============================================================
//   AFFICHAGE TRICHE DÉTECTÉE
// ============================================================
void GameScreen::showCheatDetected(const QString &reason)
{
    cheatLabel->setText(reason);
    cheatOverlay->setGeometry(gameWidget->rect());
    cheatOverlay->show();
    cheatOverlay->raise();
}

// ============================================================
//   AFFICHAGE RÉSERVOIRS VIDES
// ============================================================
void GameScreen::showReservoirEmpty()
{
    reservoirOverlay->setGeometry(gameWidget->rect());
    reservoirOverlay->show();
    reservoirOverlay->raise();
}

// ============================================================
//   RESET TOUS LES OVERLAYS
// ============================================================
void GameScreen::resetAllOverlays()
{
    warningOverlay->hide();
    cheatOverlay->hide();
    // NE PAS cacher reservoirOverlay - il est prioritaire et ne peut être fermé que par le bouton
}

// ============================================================
//   AFFICHAGE RÉSULTAT DE LA PARTIE
// ============================================================
void GameScreen::showGameResult(const QString &winner, const QString &difficulty, int totalSeconds)
{
    chronometer.stop();

    // Utiliser le temps du chronomètre local, pas celui passé en paramètre (qui est toujours 0)
    int actualTime = elapsedSeconds;

    QString message;
    if (winner == "Joueur") {
        message = "VICTOIRE !\n\n";
    } else if (winner == "Robot") {
        message = "DÉFAITE\n\n";
    } else {
        message = "ÉGALITÉ\n\n";
    }

    message += QString("Difficulté : %1\n").arg(difficulty);
    message += QString("Temps : %1:%2")
                   .arg(actualTime / 60, 2, 10, QChar('0'))
                   .arg(actualTime % 60, 2, 10, QChar('0'));

    resultLabel->setText(message);
    resultOverlay->setGeometry(gameWidget->rect());
    resultOverlay->show();
    resultOverlay->raise();
}

// ============================================================
//   VÉRIFIER SI OVERLAY RÉSERVOIRS EST VISIBLE
// ============================================================
bool GameScreen::isReservoirOverlayVisible() const
{
    return reservoirOverlay->isVisible();
}

// ============================================================
//   AFFICHAGE ERREUR DE CONNEXION AU ROBOT
// ============================================================
void GameScreen::showConnectionError()
{
    // Revenir au gameWidget si on était sur l'écran d'initialisation
    stack->setCurrentWidget(gameWidget);

    // Afficher l'overlay d'erreur de connexion
    connectionErrorOverlay->setGeometry(gameWidget->rect());
    connectionErrorOverlay->show();
    connectionErrorOverlay->raise();
}

// ============================================================
//   EVENT DE REDIMENSIONNEMENT
// ============================================================
void GameScreen::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);
    // Redimensionner les overlays s'ils sont visibles
    if (warningOverlay && warningOverlay->isVisible()) {
        warningOverlay->setGeometry(gameWidget->rect());
    }
    if (cheatOverlay && cheatOverlay->isVisible()) {
        cheatOverlay->setGeometry(gameWidget->rect());
    }
    if (reservoirOverlay && reservoirOverlay->isVisible()) {
        reservoirOverlay->setGeometry(gameWidget->rect());
    }
    if (resultOverlay && resultOverlay->isVisible()) {
        resultOverlay->setGeometry(gameWidget->rect());
    }
    if (connectionErrorOverlay && connectionErrorOverlay->isVisible()) {
        connectionErrorOverlay->setGeometry(gameWidget->rect());
    }
}
