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
    createGameWidget();
    createConfirmWidget();

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
    quitButton = new QPushButton("← Quitter");
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
// DÉMARRAGE DU JEU (à appeler quand on affiche GameScreen)
// ============================================================
void GameScreen::startGame()
{
    // Retour au widget de jeu
    stack->setCurrentWidget(gameWidget);

    // Réinitialiser le chronomètre
    elapsedSeconds = 0;
    timerLabel->setText("00:00");

    // Tout est caché pendant le countdown
    quitButton->hide();
    titleLabel->hide();
    timerLabel->hide();
    turnLabel->hide();
    cameraLabel->hide();

    // Lancer le countdown
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
    if (!img.isNull())
        cameraLabel->setPixmap(QPixmap::fromImage(img).scaled(
            cameraLabel->size(),
            Qt::KeepAspectRatio,
            Qt::SmoothTransformation
            ));
}

void GameScreen::setTurnPlayer()
{
    turnLabel->setText("Au tour du joueur");
    turnLabel->setStyleSheet("font-size: 35px; font-weight: bold; color: #B22222;");
}

void GameScreen::setTurnRobot()
{
    turnLabel->setText("Au tour du robot");
    turnLabel->setStyleSheet("font-size: 35px; font-weight: bold; color: #EFCB00;");
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
