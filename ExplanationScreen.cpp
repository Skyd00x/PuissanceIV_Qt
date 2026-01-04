#include "ExplanationScreen.hpp"
#include "ResourcesPath.hpp"
#include <QPalette>
#include <QPixmap>
#include <QDebug>

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
    QLabel *titleLabel = new QLabel("Fonctionnement du jeu", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    // === IMAGE D'EXPLICATION ===
    QLabel *imageLabel = new QLabel(this);
    QString imagePath = getResourcePath("image/fonctionnement.png");

    QPixmap image(imagePath);
    if (image.isNull()) {
        qWarning() << "[ExplanationScreen] Impossible de charger l'image:" << imagePath;
        // Afficher un message de fallback si l'image n'est pas trouvée
        imageLabel->setText("Image d'explication non disponible");
        imageLabel->setStyleSheet("font-size: 24px; color: red;");
    } else {
        // Afficher l'image en la redimensionnant pour qu'elle tienne dans l'écran
        // Garder le ratio et limiter la taille max à 1000x600 pixels
        imageLabel->setPixmap(image.scaled(1000, 600, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }
    imageLabel->setAlignment(Qt::AlignCenter);

    // === LAYOUT ===
    layout->addWidget(backButton, 0, Qt::AlignLeft);
    layout->addSpacing(20);
    layout->addWidget(titleLabel, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addStretch();
    layout->addWidget(imageLabel, 0, Qt::AlignCenter);
    layout->addStretch();
}
