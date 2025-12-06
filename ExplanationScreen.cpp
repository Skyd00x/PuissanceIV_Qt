#include "ExplanationScreen.hpp"
#include <QPalette>

ExplanationScreen::ExplanationScreen(QWidget *parent)
    : QWidget(parent)
{
    // Fond blanc comme les autres écrans
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setContentsMargins(60, 40, 60, 40);
    layout->setSpacing(30);

    // === BOUTON RETOUR (coin supérieur gauche, même style que CalibrationTestScreen) ===
    QPushButton *backButton = new QPushButton("Retour", this);
    backButton->setFixedSize(160, 55);
    backButton->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 27px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
    );
    backButton->setCursor(Qt::PointingHandCursor);
    connect(backButton, &QPushButton::clicked, this, &ExplanationScreen::backToMenu);

    // === TITRE ===
    QLabel *titleLabel = new QLabel("Aide", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    // === CONTENU ===
    QLabel *label = new QLabel("Explication du jeu (placeholder)", this);
    label->setStyleSheet("font-size: 28px; color: #1B3B5F;");
    label->setAlignment(Qt::AlignCenter);

    // === LAYOUT ===
    layout->addWidget(backButton, 0, Qt::AlignLeft);
    layout->addSpacing(20);
    layout->addWidget(titleLabel, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addStretch();
    layout->addWidget(label, 0, Qt::AlignCenter);
    layout->addStretch();
}
