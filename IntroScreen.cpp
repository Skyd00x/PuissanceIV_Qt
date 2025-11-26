#include "IntroScreen.hpp"
#include <QPixmap>
#include <QFile>
#include <QDebug>

IntroScreen::IntroScreen(QWidget *parent)
    : QWidget(parent)
{
    // Fond noir et style de base
    setStyleSheet("background-color: black;");
    setAttribute(Qt::WA_StyledBackground, true);

    // === Titre principal ===
    titleLabel = new QLabel("PUISSANCE IV", this);
    titleLabel->setStyleSheet("color: white; font-size: 72px; font-weight: bold;");
    titleLabel->setAlignment(Qt::AlignCenter);

    // === Paragraphe descriptif ===
    QLabel* paragraphLabel = new QLabel(this);
    paragraphLabel->setText(
        "Ce projet a été réalisé par des étudiants de Polytech Tours.\n"
        "Il permet de jouer au puissance 4 contre une machine, qui détecte la grille par vision artificielle,\n"
        "prend ses décisions grâce à une intelligence artificielle, puis pilote le bras robotisé pour jouer.\n"
        "En cas de problème, veuillez contacter l'administrateur du jeu."
        );
    paragraphLabel->setStyleSheet("color: white; font-size: 20px;");
    paragraphLabel->setAlignment(Qt::AlignCenter);
    paragraphLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    paragraphLabel->setMaximumWidth(1200);

    // === Bloc d’avertissement (icône + texte) ===
    QLabel* warningIcon = new QLabel(this);
    QString warningPath = "./Ressources/image/warning.png";
    if (!QFile::exists(warningPath))
        qWarning() << "Warning icon introuvable :" << warningPath;

    QPixmap warningPixmap(warningPath);
    if (warningPixmap.isNull())
        qWarning() << "Échec du chargement du warning icon depuis :" << warningPath;
    else
        warningIcon->setPixmap(warningPixmap.scaled(115, 115, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    warningIcon->setAlignment(Qt::AlignVCenter);

    QLabel* warningText = new QLabel(
        "Ne jamais mettre votre main ou quelconque obstacle dans la zone de mouvement du robot, "
        "lorsque la partie est lancée.", this);
    warningText->setStyleSheet("color: yellow; font-size: 22px; font-weight: bold;");
    warningText->setAlignment(Qt::AlignVCenter);
    warningText->setWordWrap(true);

    // Layout horizontal pour l’avertissement
    QHBoxLayout* warningLayout = new QHBoxLayout();
    warningLayout->addStretch();
    warningLayout->addWidget(warningIcon);
    warningLayout->addSpacing(20);
    warningLayout->addWidget(warningText);
    warningLayout->addStretch();

    // === Logo Polytech ===
    logoLabel = new QLabel(this);
    QString logoPath = "./Ressources/image/Logo_PolytechTours.png";
    if (!QFile::exists(logoPath))
        qWarning() << "Logo introuvable :" << logoPath;

    QPixmap logo(logoPath);
    if (logo.isNull())
        qWarning() << "Échec du chargement du logo depuis :" << logoPath;
    else
        logoLabel->setPixmap(logo.scaled(250, 250, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    logoLabel->setAlignment(Qt::AlignCenter);

    // === Copyright ===
    copyrightLabel = new QLabel("© 2019–2025  –  Polytech Tours", this);
    copyrightLabel->setStyleSheet("color: white; font-size: 18px;");
    copyrightLabel->setAlignment(Qt::AlignCenter);

    // === Construction du layout principal ===
    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    // Bloc supérieur : titre
    QVBoxLayout* topLayout = new QVBoxLayout();
    topLayout->addWidget(titleLabel, 0, Qt::AlignTop | Qt::AlignHCenter);
    topLayout->addSpacing(100); // Espace vertical augmenté entre le titre et le paragraphe

    // Bloc central : paragraphe + avertissement
    QVBoxLayout* centerLayout = new QVBoxLayout();
    centerLayout->addWidget(paragraphLabel, 0, Qt::AlignHCenter);
    centerLayout->addSpacing(30);
    centerLayout->addLayout(warningLayout);
    centerLayout->addStretch();

    // Bloc inférieur : logo et copyright
    QVBoxLayout* bottomLayout = new QVBoxLayout();
    bottomLayout->addStretch();
    bottomLayout->addWidget(logoLabel, 0, Qt::AlignHCenter);
    bottomLayout->addSpacing(10);
    bottomLayout->addWidget(copyrightLabel, 0, Qt::AlignHCenter);

    // Assemblage final du layout
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(centerLayout);
    mainLayout->addLayout(bottomLayout);
    mainLayout->setContentsMargins(100, 60, 100, 40);
    mainLayout->setSpacing(0);
    setLayout(mainLayout);

    // === Configuration des animations d’opacité ===
    opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(0.0);

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

    // Configuration des connexions pour la séquence d'animation
    connect(fadeInAnim, &QPropertyAnimation::finished, this, [this]() {
        holdTimer->start(8500);
    });

    connect(holdTimer, &QTimer::timeout, this, [this]() {
        fadeOutAnim->start();
    });

    connect(fadeOutAnim, &QPropertyAnimation::finished, this, [this]() {
        QTimer::singleShot(1000, this, [this]() {
            emit introFinished();
            hide();
        });
    });
}

void IntroScreen::start()
{
    // Lance la séquence d'animation :
    // 1 seconde de noir
    // 2 secondes de fade-in
    // 7,5 secondes d'affichage complet
    // 2 secondes de fade-out
    // 1 seconde de noir avant fermeture
    QTimer::singleShot(1000, this, [this]() {
        fadeInAnim->start();
    });
}
