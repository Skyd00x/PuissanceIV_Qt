#include "IntroScreen.hpp"
#include <QPixmap>

IntroScreen::IntroScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: black;");
    setAttribute(Qt::WA_StyledBackground, true);

    // === Texte principal ===
    titleLabel = new QLabel("Puissance 4", this);
    titleLabel->setStyleSheet("color: white; font-size: 72px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);

    // === Logo Polytech ===
    logoLabel = new QLabel(this);
    QPixmap logo(".\\Ressources\\image\\Logo_PolytechTours.png");
    logoLabel->setPixmap(logo.scaled(300, 300, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    logoLabel->setAlignment(Qt::AlignCenter);

    // === Copyright ===
    copyrightLabel = new QLabel("© 2025 - Projet Robotique Polytech Tours", this);
    copyrightLabel->setStyleSheet("color: white; font-size: 18px;");
    copyrightLabel->setAlignment(Qt::AlignCenter);

    // === Layout vertical ===
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->addStretch();
    layout->addWidget(titleLabel);
    layout->addWidget(logoLabel);
    layout->addSpacing(40);
    layout->addWidget(copyrightLabel);
    layout->addStretch();
    setLayout(layout);

    // === Animation d’opacité ===
    opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);

    fadeInAnim = new QPropertyAnimation(opacityEffect, "opacity");
    fadeInAnim->setDuration(2000);
    fadeInAnim->setStartValue(0.0);
    fadeInAnim->setEndValue(1.0);

    fadeOutAnim = new QPropertyAnimation(opacityEffect, "opacity");
    fadeOutAnim->setDuration(2000);
    fadeOutAnim->setStartValue(1.0);
    fadeOutAnim->setEndValue(0.0);

    holdTimer = new QTimer(this);
    holdTimer->setSingleShot(true);

    // Séquence : fade-in → pause → fade-out
    connect(fadeInAnim, &QPropertyAnimation::finished, this, [this]() {
        holdTimer->start(2000);
    });
    connect(holdTimer, &QTimer::timeout, this, [this]() {
        fadeOutAnim->start();
    });
    connect(fadeOutAnim, &QPropertyAnimation::finished, this, [this]() {
        emit introFinished();
        hide();
    });
}

void IntroScreen::start()
{
    fadeInAnim->start();
    show();
}
