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

    // ============================
    //  BOUTON QUITTER (Q1 STYLE)
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
    connect(quitButton, &QPushButton::clicked, this, &GameScreen::quitRequested);

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

    // Chronomètre logique
    connect(&chronometer, &QTimer::timeout, this, &GameScreen::updateChronometer);

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
    countdownLabel = new QLabel("3");
    countdownLabel->setAlignment(Qt::AlignCenter);
    countdownLabel->setStyleSheet("font-size: 120px; font-weight: bold; color: #1B3B5F;");
    countdownLabel->hide();

    connect(&countdownTimer, &QTimer::timeout, this, &GameScreen::updateCountdown);

    // ============================
    //   LAYOUT TOP
    // ============================
    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->setContentsMargins(20, 20, 20, 0);
    topBar->addWidget(quitButton, 0, Qt::AlignLeft | Qt::AlignTop);
    topBar->addWidget(titleLabel, 1, Qt::AlignCenter);
    topBar->addWidget(timerLabel, 0, Qt::AlignRight | Qt::AlignTop);

    // ============================
    //   LAYOUT GLOBAL
    // ============================
    QVBoxLayout *main = new QVBoxLayout(this);
    main->setContentsMargins(20, 20, 20, 20);
    main->setSpacing(20);

    main->addLayout(topBar);
    main->addWidget(turnLabel);
    main->addWidget(cameraLabel, 1);
    main->addWidget(countdownLabel, 0, Qt::AlignCenter);  // temporaire pendant countdown
    main->addStretch();

    setLayout(main);

    // ============================
    //   DÉMARRER LE COUNTDOWN
    // ============================
    startCountdown();
}

// ============================================================
// COUNTDOWN : 3 → 2 → 1 → GO
// ============================================================
void GameScreen::startCountdown()
{
    countdownValue = 3;
    countdownLabel->setText("3");
    countdownLabel->show();

quartz:
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
            countdownLabel->hide();
            emit countdownFinished();
            chronometer.start(1000);
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
