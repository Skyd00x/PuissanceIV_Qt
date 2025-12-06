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
    stack->setAttribute(Qt::WA_TranslucentBackground, false);

    // IMPORTANT : Empêcher que les widgets animés sortent du stack et affectent les autres éléments
    stack->setStyleSheet("QStackedWidget { background-color: white; }");

    createMainMenu();
    createDifficultyMenu();
    createColorMenu();
    createConfirmationMenu();

    stack->addWidget(mainMenuWidget);
    stack->addWidget(difficultyWidget);
    stack->addWidget(colorWidget);
    stack->addWidget(confirmWidget);
    stack->setCurrentWidget(mainMenuWidget);

    // === LAYOUT GLOBAL (SEULEMENT LE STACK) ===
    QVBoxLayout *global = new QVBoxLayout(this);
    global->setContentsMargins(0, 0, 0, 0);
    global->setSpacing(0);
    global->addWidget(stack, 1);
    setLayout(global);

    // === BOUTON AIDE (POSITION ABSOLUE - NE BOUGE JAMAIS) ===
    helpButton = new QPushButton("?", this);
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

    // Ombre pour le bouton d'aide
    QGraphicsDropShadowEffect *helpShadow = new QGraphicsDropShadowEffect;
    helpShadow->setBlurRadius(20);
    helpShadow->setOffset(3, 3);
    helpShadow->setColor(QColor(0, 0, 0, 120));
    helpButton->setGraphicsEffect(helpShadow);

    // === LOGO POLYTECH (POSITION ABSOLUE - NE BOUGE JAMAIS) ===
    logoLabel = new QLabel(this);
    QPixmap logo("./Ressources/image/Logo_PolytechTours.png");
    if (!logo.isNull())
        logoLabel->setPixmap(logo.scaled(160, 160, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setFixedSize(160, 160);

    // Les positionner sera fait dans resizeEvent()
    // Pour le moment, les mettre hors écran
    helpButton->move(25, height() - 95);
    logoLabel->move(width() - 185, height() - 185);

    // S'assurer qu'ils sont toujours au-dessus
    helpButton->raise();
    logoLabel->raise();
}

// ============================================================
// MENU PRINCIPAL
// ============================================================
void MainMenu::createMainMenu()
{
    mainMenuWidget = new QWidget(this);

    // === IMAGE ===
    QLabel *leftImage = new QLabel;
    QPixmap imgLeft("./Ressources/image/menu_principal.png");
    if (!imgLeft.isNull()) {
        leftImage->setPixmap(imgLeft.scaled(450, 450, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        // Rendre l'image arrondie
        leftImage->setStyleSheet("border-radius: 20px; background-color: transparent;");
        leftImage->setScaledContents(false);
    }
    leftImage->setAlignment(Qt::AlignCenter);

    // === TITRE CENTRÉ INDÉPENDANT ===
    QLabel *topTitle = new QLabel("PUISSANCE IV");
    topTitle->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
    topTitle->setStyleSheet("font-size: 80px; font-weight: bold; color: #1B3B5F;");

    // === VERSION (coin supérieur droit) ===
    QLabel *versionLabel = new QLabel("Version 1.0");
    versionLabel->setAlignment(Qt::AlignRight | Qt::AlignTop);
    versionLabel->setStyleSheet("font-size: 16px; color: #666666; padding: 5px 15px;");

    // === BOUTONS PRINCIPAUX ===
    QVBoxLayout *centerLayout = new QVBoxLayout;
    centerLayout->setSpacing(25);
    centerLayout->setContentsMargins(0, 0, 0, 0);

    const int buttonWidth = 420;
    const int buttonHeight = 95;

    launchButton = new QPushButton("Lancer une partie");
    calibrationButton = new QPushButton("Calibration");
    testCalibrationButton = new QPushButton("Tester la calibration");
    quitButton = new QPushButton("Quitter le jeu");

    QList<QPushButton*> buttons = {launchButton, calibrationButton, testCalibrationButton, quitButton};
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
    hLayout->setContentsMargins(100, 0, 100, 40);  // Marges égales gauche/droite
    hLayout->setSpacing(80);
    hLayout->addWidget(leftImage, 0, Qt::AlignCenter);  // Image centrée verticalement avec les boutons
    hLayout->addLayout(centerLayout, 0);
    hLayout->addSpacing(100);  // Espacement fixe à droite pour équilibrer

    // === LAYOUT TITRE + VERSION ===
    // Utiliser un layout horizontal avec stretches égaux pour centrer le titre parfaitement
    QHBoxLayout *topLayout = new QHBoxLayout;
    topLayout->setContentsMargins(20, 10, 20, 0);  // Petites marges, surtout en haut

    // Ajouter une version "fantôme" invisible à gauche pour équilibrer
    QLabel *spacerLeft = new QLabel;
    spacerLeft->setFixedWidth(100);  // Même largeur que la version à droite

    topLayout->addWidget(spacerLeft);
    topLayout->addStretch(1);
    topLayout->addWidget(topTitle, 0, Qt::AlignCenter);
    topLayout->addStretch(1);
    topLayout->addWidget(versionLabel, 0, Qt::AlignRight | Qt::AlignTop);  // AlignTop pour monter

    // === LAYOUT GLOBAL DU MENU ===
    QVBoxLayout *mainLayout = new QVBoxLayout(mainMenuWidget);
    mainLayout->addLayout(topLayout);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(hLayout, 1);
    mainLayout->setContentsMargins(0, 20, 0, 40);  // Marge haute réduite

    // === Connexions ===
    connect(launchButton, &QPushButton::clicked, this, &MainMenu::showDifficultyMenu);
    connect(calibrationButton, &QPushButton::clicked, [this]() {
        showConfirmationMenu("Voulez-vous lancer la calibration du robot ?", ConfirmationType::Calibration);
    });
    connect(testCalibrationButton, &QPushButton::clicked, this, &MainMenu::startCalibrationTest);
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

    backButtonDiff = new QPushButton("Retour");
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

    QList<QPushButton*> diffButtons = {diffEasy, diffNormal, diffHard};
    QList<QString> colors = {"#2ECC71", "#E67E22", "#B22222"};

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

    backButtonColor = new QPushButton("Retour");
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
// TRANSITION PAR FONDU CROISÉ (N'AFFECTE PAS LES AUTRES WIDGETS)
// ============================================================
void MainMenu::animateTransition(QWidget *from, QWidget *to, bool forward)
{
    Q_UNUSED(forward);  // Non utilisé dans le fondu croisé

    if (!from || !to) return;

    // Créer les effets d'opacité
    QGraphicsOpacityEffect *effectOut = new QGraphicsOpacityEffect(from);
    QGraphicsOpacityEffect *effectIn = new QGraphicsOpacityEffect(to);

    from->setGraphicsEffect(effectOut);
    to->setGraphicsEffect(effectIn);

    // Préparer le widget entrant
    effectIn->setOpacity(0.0);
    to->show();
    stack->setCurrentWidget(to);  // Changer immédiatement pour éviter les glitches

    // Animation de fondu sortant
    auto *fadeOut = new QPropertyAnimation(effectOut, "opacity");
    fadeOut->setDuration(300);
    fadeOut->setStartValue(1.0);
    fadeOut->setEndValue(0.0);
    fadeOut->setEasingCurve(QEasingCurve::InOutQuad);

    // Animation de fondu entrant
    auto *fadeIn = new QPropertyAnimation(effectIn, "opacity");
    fadeIn->setDuration(300);
    fadeIn->setStartValue(0.0);
    fadeIn->setEndValue(1.0);
    fadeIn->setEasingCurve(QEasingCurve::InOutQuad);

    // Nettoyage après l'animation
    connect(fadeIn, &QPropertyAnimation::finished, [from, to]() {
        from->setGraphicsEffect(nullptr);
        to->setGraphicsEffect(nullptr);
    });

    // Lancer les animations en parallèle
    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(fadeOut);
    group->addAnimation(fadeIn);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

// ============================================================
// CHOIX DE DIFFICULTÉ
// ============================================================
void MainMenu::onDifficultySelected()
{
    QPushButton *btn = qobject_cast<QPushButton*>(sender());
    if (!btn) return;

    if (btn == diffHard)
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
    }

    QString colorName = (selectedColor == StateMachine::PlayerColor::Red) ? "Rouge" : "Jaune";

    showConfirmationMenu(QString("Voulez-vous démarrer une partie en mode %1 avec les pions %2 ?").arg(diffName).arg(colorName),
                         ConfirmationType::StartGame);
}

// ============================================================
// AFFICHAGE DES MENUS
// ============================================================
void MainMenu::showDifficultyMenu()
{
    // Cacher le bouton d'aide et le logo dans les sous-menus
    helpButton->hide();
    logoLabel->hide();
    animateTransition(stack->currentWidget(), difficultyWidget, true);
}

void MainMenu::showColorMenu()
{
    // Cacher le bouton d'aide et le logo dans les sous-menus
    helpButton->hide();
    logoLabel->hide();
    animateTransition(stack->currentWidget(), colorWidget, true);
}

void MainMenu::showMainMenu()
{
    // Réafficher le bouton d'aide et le logo dans le menu principal
    helpButton->show();
    logoLabel->show();
    animateTransition(stack->currentWidget(), mainMenuWidget, false);
}

void MainMenu::showConfirmationMenu(const QString &text, ConfirmationType type)
{
    // Cacher le bouton d'aide et le logo dans les sous-menus
    helpButton->hide();
    logoLabel->hide();
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

    // Réafficher le bouton d'aide et le logo
    helpButton->show();
    logoLabel->show();
}

// ============================================================
// REPOSITIONNEMENT DU BOUTON D'AIDE ET DU LOGO LORS DU RESIZE
// ============================================================
void MainMenu::resizeEvent(QResizeEvent *event)
{
    QWidget::resizeEvent(event);

    // Repositionner le bouton d'aide (coin bas gauche)
    helpButton->move(25, height() - 95);

    // Repositionner le logo Polytech (complètement dans le coin bas droit)
    logoLabel->move(width() - 185, height() - 185);

    // S'assurer qu'ils restent au-dessus pendant les animations
    helpButton->raise();
    logoLabel->raise();
}
