#include "CalibrationScreen.hpp"

CalibrationScreen::CalibrationScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #F9F9F9;");
    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Écran de calibration (placeholder)", this);
    label->setStyleSheet("font-size: 32px; color: #333;");
    label->setAlignment(Qt::AlignCenter);

    QPushButton *backButton = new QPushButton("Retour au menu", this);
    backButton->setStyleSheet("background-color: #347BB7; color: white; padding: 10px; font-size: 20px;");
    connect(backButton, &QPushButton::clicked, this, &CalibrationScreen::backToMenu);

    layout->addStretch();
    layout->addWidget(label);
    layout->addWidget(backButton, 0, Qt::AlignCenter);
    layout->addStretch();
}
