#include "MainMenu.hpp"
#include <QPixmap>
#include <QSpacerItem>

MainMenu::MainMenu(QWidget *parent)
    : QWidget(parent)
{
    // === FOND BLANC TOTAL ===
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // === STACK POUR LES DEUX MENUS ===
    stack = new QStackedWidget(this);
    createMainMenu();
    createDifficultyMenu();

    stack->addWidget(mainMenuWidget);
    stack->addWidget(difficultyWidget);
    stack->setCurrentWidget(mainMenuWidget);

    // === BOUTON AIDE ===
    helpButton = new QPushButton("?");
    helpButton->setFixedSize(70, 70);
    helpButton->setStyleSheet(
        "QPushButton {"
        " background-color: #4A90E2;"
        " color: white;"
        " font-size: 32px;"
        " font-weight: bold;"
        " border-radius: 35px;"
        "}"
        "QPushButton:hover { background-color: #357ABD; }"
        );
    helpButton->setCursor(Qt::PointingHandCursor);
    connect(helpButton, &QPushButton::clicked, this, &MainMenu::openExplanation);

    // === LOGO POLYTECH TOURS EN BAS CENTRE ===
    QLabel *logoLabel = new QLabel(this);
    QPixmap logo("./Ressources/image/Logo_PolytechTours.png");
    if (!logo.isNull())
        logoLabel->setPixmap(logo.scaled(180, 180, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);

    // === LAYOUT GLOBAL ===
    QVBoxLayout *global = new QVBoxLayout(this);
    global->setContentsMargins(0, 0, 0, 0);
    global->setSpacing(0);
    global->addWidget(stack, 1);

    // Barre du bas : bouton ? à gauche, logo au centre
    QHBoxLayout *bottomBar = new QHBoxLayout;
    bottomBar->setContentsMargins(25, 10, 25, 25);
    bottomBar->addWidget(helpButton, 0, Qt::AlignLeft | Qt::AlignBottom);
    bottomBar->addStretch();
    bottomBar->addWidget(logoLabel, 0, Qt::AlignBottom | Qt::AlignCenter);
    bottomBar->addStretch();

    global->addLayout(bottomBar);
    setLayout(global);
}

void MainMenu::createMainMenu()
{
    mainMenuWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(mainMenuWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // === TITRE ===
    titleLabel = new QLabel("PUISSANCE IV");
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 80px; font-weight: bold; color: #1B3B5F;");

    // === BOUTONS ===
    const int buttonWidth = 380;
    const int buttonHeight = 80;

    launchButton = new QPushButton("Lancer une partie");
    calibrationButton = new QPushButton("Calibration");
    quitButton = new QPushButton("Quitter le jeu");

    QList<QPushButton*> buttons = {launchButton, calibrationButton, quitButton};
    for (auto *b : buttons) {
        b->setFixedSize(buttonWidth, buttonHeight);
        b->setStyleSheet(
            "QPushButton {"
            " background-color: #347BB7;"
            " color: white;"
            " font-size: 28px;"
            " font-weight: bold;"
            " border-radius: 40px;"
            "}"
            "QPushButton:hover { background-color: #2A6394; }"
            );
        b->setCursor(Qt::PointingHandCursor);
    }

    // === MISE EN PAGE ===
    layout->addStretch();
    layout->addWidget(titleLabel, 0, Qt::AlignCenter);
    layout->addSpacing(50);
    for (auto *b : buttons)
        layout->addWidget(b, 0, Qt::AlignCenter);
    layout->addStretch();

    connect(launchButton, &QPushButton::clicked, this, &MainMenu::showDifficultyMenu);
    connect(calibrationButton, &QPushButton::clicked, this, &MainMenu::onCalibration);
    connect(quitButton, &QPushButton::clicked, this, &MainMenu::onQuit);
}

void MainMenu::createDifficultyMenu()
{
    difficultyWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(difficultyWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(20);

    // === BOUTON RETOUR ===
    backButton = new QPushButton("← Retour");
    backButton->setFixedSize(150, 50);
    backButton->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F; font-size: 20px; border-radius: 25px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        );
    backButton->setCursor(Qt::PointingHandCursor);
    connect(backButton, &QPushButton::clicked, this, &MainMenu::showMainMenu);

    // === TITRE ===
    QLabel *title = new QLabel("Choisissez la difficulté");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    // === BOUTONS DE DIFFICULTÉ ===
    const int buttonWidth = 380;
    const int buttonHeight = 80;

    diffEasy = new QPushButton("Facile");
    diffNormal = new QPushButton("Normal");
    diffHard = new QPushButton("Difficile");
    diffImpossible = new QPushButton("Impossible");

    QList<QPushButton*> diffButtons = {diffEasy, diffNormal, diffHard, diffImpossible};
    QList<QString> colors = {"#2ECC71", "#E67E22", "#B22222", "#6B0F1A"};

    for (int i = 0; i < diffButtons.size(); ++i) {
        diffButtons[i]->setFixedSize(buttonWidth, buttonHeight);
        diffButtons[i]->setStyleSheet(QString(
                                          "QPushButton { background-color: %1; color: white; font-size: 28px; font-weight: bold; border-radius: 40px; }"
                                          "QPushButton:hover { background-color: #333333; }"
                                          ).arg(colors[i]));
        diffButtons[i]->setCursor(Qt::PointingHandCursor);
        connect(diffButtons[i], &QPushButton::clicked, this, &MainMenu::onDifficultySelected);
    }

    // === LAYOUT ===
    QHBoxLayout *topBar = new QHBoxLayout;
    topBar->addWidget(backButton, 0, Qt::AlignLeft);
    topBar->addStretch();

    layout->addLayout(topBar);
    layout->addSpacing(20);
    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addSpacing(50);
    for (auto *b : diffButtons)
        layout->addWidget(b, 0, Qt::AlignCenter);
    layout->addStretch();
}

void MainMenu::animateTransition(QWidget *from, QWidget *to)
{
    auto *fadeOutEffect = new QGraphicsOpacityEffect(from);
    from->setGraphicsEffect(fadeOutEffect);
    auto *fadeOut = new QPropertyAnimation(fadeOutEffect, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);

    auto *fadeInEffect = new QGraphicsOpacityEffect(to);
    to->setGraphicsEffect(fadeInEffect);
    fadeInEffect->setOpacity(0.0);
    auto *fadeIn = new QPropertyAnimation(fadeInEffect, "opacity");
    fadeIn->setDuration(300);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);

    connect(fadeOut, &QPropertyAnimation::finished, [this, to]() {
        stack->setCurrentWidget(to);
    });

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(fadeOut);
    group->addAnimation(fadeIn);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainMenu::showDifficultyMenu() { animateTransition(mainMenuWidget, difficultyWidget); }
void MainMenu::showMainMenu() { animateTransition(difficultyWidget, mainMenuWidget); }

void MainMenu::onDifficultySelected()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    StateMachine::Difficulty diff = StateMachine::Difficulty::Easy;
    if (btn == diffImpossible) diff = StateMachine::Difficulty::Impossible;
    else if (btn == diffHard) diff = StateMachine::Difficulty::Hard;
    else if (btn == diffNormal) diff = StateMachine::Difficulty::Medium;
    else if (btn == diffEasy) diff = StateMachine::Difficulty::Easy;

    if (QMessageBox::question(this, "Confirmation",
                              QString("Démarrer une partie en mode %1 ?").arg(btn->text()),
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        emit startGame(diff);
}

void MainMenu::onCalibration()
{
    if (QMessageBox::question(this, "Confirmation",
                              "Voulez-vous lancer la calibration du robot ?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        emit startCalibration();
}

void MainMenu::onQuit()
{
    if (QMessageBox::question(this, "Quitter le jeu",
                              "Êtes-vous sûr de vouloir quitter ?",
                              QMessageBox::Yes | QMessageBox::No) == QMessageBox::Yes)
        emit quitGame();
}
