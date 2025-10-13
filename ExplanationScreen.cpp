#include "ExplanationScreen.hpp"

ExplanationScreen::ExplanationScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #EAF7FF;");
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Explication du jeu (placeholder)", this);
    label->setStyleSheet("font-size: 28px; color: #1B3B5F;");
    label->setAlignment(Qt::AlignCenter);

    QPushButton *backButton = new QPushButton("Retour au menu", this);
    backButton->setStyleSheet("background-color: #347BB7; color: white; padding: 10px; font-size: 20px;");
    connect(backButton, &QPushButton::clicked, this, &ExplanationScreen::backToMenu);

    layout->addStretch();
    layout->addWidget(label);
    layout->addWidget(backButton, 0, Qt::AlignCenter);
    layout->addStretch();
}
