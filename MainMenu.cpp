#include "MainMenu.hpp"
#include <QPixmap>
#include <QSpacerItem>
#include <QParallelAnimationGroup>
#include <QGraphicsDropShadowEffect>
#include <QPropertyAnimation>
#include <QEasingCurve>
#include <QStackedWidget>
#include <QDebug>

// ============================================================
// CONSTRUCTEUR PRINCIPAL
// ============================================================
MainMenu::MainMenu(QWidget *parent)
    : QWidget(parent)
{
    // === FOND BLANC ===
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    // === STACK PRINCIPAL ===
    stack = new QStackedWidget(this);
    createMainMenu();
    createDifficultyMenu();
    createColorMenu();
    createConfirmationMenu();

    stack->addWidget(mainMenuWidget);
    stack->addWidget(difficultyWidget);
    stack->addWidget(colorWidget);
    stack->addWidget(confirmWidget);
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
        "QPushButton:pressed { background-color: #2A5A8A; }"
        );
    helpButton->setCursor(Qt::PointingHandCursor);
    connect(helpButton, &QPushButton::clicked, this, &MainMenu::openExplanation);

    // Ombre pour le bouton d’aide
    QGraphicsDropShadowEffect *helpShadow = new QGraphicsDropShadowEffect;
    helpShadow->setBlurRadius(20);
    helpShadow->setOffset(3, 3);
    helpShadow->setColor(QColor(0, 0, 0, 120));
    helpButton->setGraphicsEffect(helpShadow);

    // === LOGO POLYTECH (coin bas droit) ===
    QLabel *logoLabel = new QLabel(this);
    QPixmap logo("./Ressources/image/Logo_PolytechTours.png");
    if (!logo.isNull())
        logoLabel->setPixmap(logo.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignRight | Qt::AlignBottom);

    // === LAYOUT GLOBAL ===
    QVBoxLayout *global = new QVBoxLayout(this);
    global->setContentsMargins(0, 0, 0, 0);
    global->setSpacing(0);
    global->addWidget(stack, 1);

    QHBoxLayout *bottomBar = new QHBoxLayout;
    bottomBar->setContentsMargins(25, 10, 25, 25);
    bottomBar->addWidget(helpButton, 0, Qt::AlignLeft | Qt::AlignBottom);
    bottomBar->addStretch();
    bottomBar->addWidget(logoLabel, 0, Qt::AlignRight | Qt::AlignBottom);

    global->addLayout(bottomBar);
    setLayout(global);
}

// ============================================================
// MENU PRINCIPAL
// ============================================================
void MainMenu::createMainMenu()
{
    mainMenuWidget = new QWidget(this);

    // === IMAGE DES JETONS ===
    QLabel *leftImage = new QLabel;
    QPixmap imgLeft("./Ressources/image/jetons.png");
    if (!imgLeft.isNull())
        leftImage->setPixmap(imgLeft.scaled(280, 280, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    leftImage->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);

    // === TITRE CENTRÉ INDÉPENDANT ===
    QLabel *topTitle = new QLabel("PUISSANCE IV");
    topTitle->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    topTitle->setStyleSheet("font-size: 80px; font-weight: bold; color: #1B3B5F;");

    // === BOUTONS PRINCIPAUX ===
    QVBoxLayout *centerLayout = new QVBoxLayout;
    centerLayout->setSpacing(25);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    const int buttonWidth = 420;
    const int buttonHeight = 95;

    launchButton = new QPushButton("Lancer une partie");
    calibrationButton = new QPushButton("Calibration");
    quitButton = new QPushButton("Quitter le jeu");

    QList<QPushButton*> buttons = {launchButton, calibrationButton, quitButton};
    for (auto *b : buttons) {
        b->setFixedSize(buttonWidth, buttonHeight);
        b->setStyleSheet(
            "QPushButton {"
            " background-color: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #4F8ED8, stop:1 #2C5FA3);"
            " color: white;"
            " font-size: 30px;"
            " font-weight: bold;"
            " border-radius: 45px;"
            "}"
            "QPushButton:hover { background-color: #5EA1F2; }"
            "QPushButton:pressed { background-color: #2C5FA3; }"
            );
        b->setCursor(Qt::PointingHandCursor);

        QGraphicsDropShadowEffect *shadow = new QGraphicsDropShadowEffect;
        shadow->setBlurRadius(25);
        shadow->setOffset(3, 5);
        shadow->setColor(QColor(0, 0, 0, 100));
        b->setGraphicsEffect(shadow);
    }

    centerLayout->addStretch();
    for (auto *b : buttons)
        centerLayout->addWidget(b, 0, Qt::AlignCenter);
    centerLayout->addStretch();

    // === HORIZONTAL ===
    QHBoxLayout *hLayout = new QHBoxLayout;
    hLayout->setContentsMargins(120, 0, 100, 40);
    hLayout->setSpacing(120);
    hLayout->addWidget(leftImage, 0, Qt::AlignVCenter | Qt::AlignLeft);
    hLayout->addLayout(centerLayout, 1);
    hLayout->addStretch();

    // === LAYOUT GLOBAL DU MENU ===
    QVBoxLayout *mainLayout = new QVBoxLayout(mainMenuWidget);
    mainLayout->addWidget(topTitle, 0, Qt::AlignTop);
    mainLayout->addSpacing(40);
    mainLayout->addLayout(hLayout, 1);
    mainLayout->setContentsMargins(0, 40, 0, 40);

    // === Connexions ===
    connect(launchButton, &QPushButton::clicked, this, &MainMenu::showDifficultyMenu);
    connect(calibrationButton, &QPushButton::clicked, [this]() {
        showConfirmationMenu("Voulez-vous lancer la calibration du robot ?", ConfirmationType::Calibration);
    });
    connect(quitButton, &QPushButton::clicked, [this]() {
        showConfirmationMenu("Êtes-vous sûr de vouloir quitter le jeu ?", ConfirmationType::Quit);
    });
}

// ============================================================
// MENU DES DIFFICULTÉS
// ============================================================
void MainMenu::createDifficultyMenu()
{
    difficultyWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(difficultyWidget);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);

    backButtonDiff = new QPushButton("← Retour");
    backButtonDiff->setFixedSize(160, 55);
    backButtonDiff->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 27px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
        );
    backButtonDiff->setCursor(Qt::PointingHandCursor);
    connect(backButtonDiff, &QPushButton::clicked, this, &MainMenu::showMainMenu);

    QLabel *title = new QLabel("Choisissez la difficulté");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    const int buttonWidth = 400;
    const int buttonHeight = 90;

    diffEasy = new QPushButton("Facile");
    diffNormal = new QPushButton("Normal");
    diffHard = new QPushButton("Difficile");
    diffImpossible = new QPushButton("Impossible");

    QList<QPushButton*> diffButtons = {diffEasy, diffNormal, diffHard, diffImpossible};
    QList<QString> colors = {"#2ECC71", "#E67E22", "#B22222", "#6B0F1A"};

    for (int i = 0; i < diffButtons.size(); ++i) {
        diffButtons[i]->setFixedSize(buttonWidth, buttonHeight);
        diffButtons[i]->setStyleSheet(QString(
                                          "QPushButton { background-color: %1; color: white; font-size: 28px; font-weight: bold; border-radius: 45px; }"
                                          "QPushButton:hover { background-color: #333333; }"
                                          "QPushButton:pressed { background-color: #111111; }"
                                          ).arg(colors[i]));
        diffButtons[i]->setCursor(Qt::PointingHandCursor);

        connect(diffButtons[i], &QPushButton::clicked, this, &MainMenu::onDifficultySelected);
    }

    layout->addWidget(backButtonDiff, 0, Qt::AlignLeft);
    layout->addSpacing(20);
    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addSpacing(40);
    for (auto *b : diffButtons)
        layout->addWidget(b, 0, Qt::AlignCenter);
    layout->addStretch();
}

// ============================================================
// MENU DE SÉLECTION DE COULEUR
// ============================================================
void MainMenu::createColorMenu()
{
    colorWidget = new QWidget(this);
    QVBoxLayout *layout = new QVBoxLayout(colorWidget);
    layout->setContentsMargins(60, 30, 60, 30);
    layout->setSpacing(20);

    backButtonColor = new QPushButton("← Retour");
    backButtonColor->setFixedSize(160, 55);
    backButtonColor->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 27px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
        );
    backButtonColor->setCursor(Qt::PointingHandCursor);
    connect(backButtonColor, &QPushButton::clicked, this, &MainMenu::showDifficultyMenu);

    QLabel *title = new QLabel("Choisissez votre couleur");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    const int buttonWidth = 400;
    const int buttonHeight = 90;

    colorRed = new QPushButton("Pion Rouge");
    colorYellow = new QPushButton("Pion Jaune");

    QList<QPushButton*> colorButtons = {colorRed, colorYellow};
    QList<QString> colors = {"#B22222", "#EFCB00"};

    for (int i = 0; i < colorButtons.size(); ++i) {
        colorButtons[i]->setFixedSize(buttonWidth, buttonHeight);
        colorButtons[i]->setStyleSheet(QString(
                                          "QPushButton { background-color: %1; color: white; font-size: 28px; font-weight: bold; border-radius: 45px; }"
                                          "QPushButton:hover { background-color: #333333; }"
                                          "QPushButton:pressed { background-color: #111111; }"
                                          ).arg(colors[i]));
        colorButtons[i]->setCursor(Qt::PointingHandCursor);

        connect(colorButtons[i], &QPushButton::clicked, this, &MainMenu::onColorSelected);
    }

    layout->addWidget(backButtonColor, 0, Qt::AlignLeft);
    layout->addSpacing(20);
    layout->addWidget(title, 0, Qt::AlignCenter);
    layout->addSpacing(40);
    for (auto *b : colorButtons)
        layout->addWidget(b, 0, Qt::AlignCenter);
    layout->addStretch();
}

// ============================================================
// MENU DE CONFIRMATION
// ============================================================
void MainMenu::createConfirmationMenu()
{
    confirmWidget = new QWidget(this);
    QVBoxLayout *outerLayout = new QVBoxLayout(confirmWidget);
    outerLayout->setContentsMargins(60, 30, 60, 30);
    outerLayout->setSpacing(0);

    QVBoxLayout *centerLayout = new QVBoxLayout;
    centerLayout->setSpacing(30);

    QLabel *title = new QLabel("Confirmation");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    confirmLabel = new QLabel("");
    confirmLabel->setAlignment(Qt::AlignCenter);
    confirmLabel->setStyleSheet("font-size: 30px; color: #333;");

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

    connect(noButton, &QPushButton::clicked, this, &MainMenu::showMainMenu);
    connect(yesButton, &QPushButton::clicked, [this]() {
        if (currentConfirm == ConfirmationType::StartGame)
            emit startGame(selectedDifficulty, selectedColor);
        else if (currentConfirm == ConfirmationType::Calibration)
            emit startCalibration();
        else if (currentConfirm == ConfirmationType::Quit)
            emit quitGame();
    });

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(yesButton);
    buttonLayout->addSpacing(40);
    buttonLayout->addWidget(noButton);
    buttonLayout->addStretch();

    centerLayout->addWidget(title, 0, Qt::AlignCenter);
    centerLayout->addWidget(confirmLabel, 0, Qt::AlignCenter);
    centerLayout->addSpacing(40);
    centerLayout->addLayout(buttonLayout);

    outerLayout->addStretch();
    outerLayout->addLayout(centerLayout);
    outerLayout->addStretch();
}

// ============================================================
// TRANSITION PAR GLISSADE DIRECTIONNELLE
// ============================================================
void MainMenu::animateTransition(QWidget *from, QWidget *to, bool forward)
{
    if (!from || !to) return;

    int w = stack->width();
    int h = stack->height();

    // Position selon la direction
    int startXTo = forward ? w : -w;
    int endXTo   = 0;
    int endXFrom = forward ? -w : w;

    to->setGeometry(startXTo, 0, w, h);
    to->show();

    auto *slideOut = new QPropertyAnimation(from, "geometry");
    slideOut->setDuration(400);
    slideOut->setEasingCurve(QEasingCurve::InOutQuad);
    slideOut->setStartValue(QRect(0, 0, w, h));
    slideOut->setEndValue(QRect(endXFrom, 0, w, h));

    auto *slideIn = new QPropertyAnimation(to, "geometry");
    slideIn->setDuration(400);
    slideIn->setEasingCurve(QEasingCurve::InOutQuad);
    slideIn->setStartValue(QRect(startXTo, 0, w, h));
    slideIn->setEndValue(QRect(endXTo, 0, w, h));

    connect(slideOut, &QPropertyAnimation::finished, [this, to]() {
        stack->setCurrentWidget(to);
    });

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(slideOut);
    group->addAnimation(slideIn);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

// ============================================================
// CHOIX DE DIFFICULTÉ
// ============================================================
void MainMenu::onDifficultySelected()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    if (btn == diffImpossible)
        selectedDifficulty = StateMachine::Difficulty::Impossible;
    else if (btn == diffHard)
        selectedDifficulty = StateMachine::Difficulty::Hard;
    else if (btn == diffNormal)
        selectedDifficulty = StateMachine::Difficulty::Medium;
    else
        selectedDifficulty = StateMachine::Difficulty::Easy;

    // Aller vers le choix de couleur au lieu de la confirmation
    showColorMenu();
}

// ============================================================
// CHOIX DE COULEUR
// ============================================================
void MainMenu::onColorSelected()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    if (btn == colorYellow)
        selectedColor = StateMachine::PlayerColor::Yellow;
    else
        selectedColor = StateMachine::PlayerColor::Red;

    QString diffName;
    switch (selectedDifficulty) {
    case StateMachine::Difficulty::Easy: diffName = "Facile"; break;
    case StateMachine::Difficulty::Medium: diffName = "Normal"; break;
    case StateMachine::Difficulty::Hard: diffName = "Difficile"; break;
    case StateMachine::Difficulty::Impossible: diffName = "Impossible"; break;
    }

    QString colorName = (selectedColor == StateMachine::PlayerColor::Red) ? "Rouge" : "Jaune";

    showConfirmationMenu(QString("Voulez-vous démarrer une partie en mode %1 avec les pions %2 ?").arg(diffName).arg(colorName),
                         ConfirmationType::StartGame);
}

// ============================================================
// AFFICHAGE DES MENUS
// ============================================================
void MainMenu::showDifficultyMenu() { animateTransition(stack->currentWidget(), difficultyWidget, true); }
void MainMenu::showColorMenu() { animateTransition(stack->currentWidget(), colorWidget, true); }
void MainMenu::showMainMenu() { animateTransition(stack->currentWidget(), mainMenuWidget, false); }

void MainMenu::showConfirmationMenu(const QString &text, ConfirmationType type)
{
    confirmLabel->setText(text);
    currentConfirm = type;
    animateTransition(stack->currentWidget(), confirmWidget, true);
}

// ============================================================
// RÉINITIALISATION DU MENU
// ============================================================
void MainMenu::resetToMainMenu()
{
    // Retourne directement au menu principal sans animation
    stack->setCurrentWidget(mainMenuWidget);
    currentConfirm = ConfirmationType::None;
}
