#include "CheckDevicesScreen.hpp"
#include "CameraAi.hpp"
#include "Robot.hpp"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QDebug>
#include <QTimer>
#include <QPropertyAnimation>

// ====================== DeviceChecker ======================

DeviceChecker::DeviceChecker(QObject *parent)
    : QObject(parent)
{
}

DeviceChecker::~DeviceChecker()
{
}

void DeviceChecker::checkDevices()
{
    bool camOk = CameraAI::isAvailable();
    bool robOk = Robot::isAvailable();

    emit statusUpdated(camOk, robOk);
}


// ====================== CheckDevicesScreen ======================

CheckDevicesScreen::CheckDevicesScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: white;");
    setAttribute(Qt::WA_StyledBackground, true);

    // === Titre ===
    QLabel *title = new QLabel("Vérification des équipements nécessaires");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 40px; font-weight: bold; color: black;");

    // === Chargement images ===
    auto loadImage = [&](const QString &path, QSize size) -> QPixmap {
        if (!QFile::exists(path))
            qWarning() << "Image introuvable:" << path;
        QPixmap pix(path);
        if (pix.isNull())
            qWarning() << "Échec chargement:" << path;
        return pix.scaled(size, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    };

    // === Icônes ===
    cameraIcon = new QLabel(this);
    cameraIcon->setPixmap(loadImage("./Ressources/image/camera.png", {128,128}));
    cameraIcon->setAlignment(Qt::AlignCenter);

    robotIcon = new QLabel(this);
    robotIcon->setPixmap(loadImage("./Ressources/image/robot.png", {128,128}));
    robotIcon->setAlignment(Qt::AlignCenter);

    // === Labels statut ===
    cameraStatusIcon = new QLabel(this);
    robotStatusIcon  = new QLabel(this);
    cameraStatusLabel = new QLabel("Vérification...");
    robotStatusLabel  = new QLabel("Vérification...");

    cameraStatusLabel->setAlignment(Qt::AlignCenter);
    robotStatusLabel ->setAlignment(Qt::AlignCenter);
    cameraStatusLabel->setStyleSheet("font-size: 20px; color: black;");
    robotStatusLabel ->setStyleSheet("font-size: 20px; color: black;");
    cameraStatusIcon->setAlignment(Qt::AlignCenter);
    robotStatusIcon ->setAlignment(Qt::AlignCenter);

    // === Bouton continuer ===
    continueButton = new QPushButton("Continuer", this);
    continueButton->setVisible(false);
    continueButton->setStyleSheet(
        "QPushButton { background-color: #3498db; color: white; font-size: 26px; "
        "font-weight: bold; border-radius: 15px; padding: 15px 60px; }"
        "QPushButton:hover { background-color: #2980b9; }"
        );
    connect(continueButton, &QPushButton::clicked, this, &CheckDevicesScreen::onContinueClicked);

    // === Layouts ===
    QVBoxLayout *camLayout = new QVBoxLayout;
    camLayout->addWidget(cameraIcon);
    camLayout->addWidget(cameraStatusIcon);
    camLayout->addWidget(cameraStatusLabel);

    QVBoxLayout *robotLayout = new QVBoxLayout;
    robotLayout->addWidget(robotIcon);
    robotLayout->addWidget(robotStatusIcon);
    robotLayout->addWidget(robotStatusLabel);

    QHBoxLayout *devicesLayout = new QHBoxLayout;
    devicesLayout->addStretch();
    devicesLayout->addLayout(camLayout);
    devicesLayout->addSpacing(120);
    devicesLayout->addLayout(robotLayout);
    devicesLayout->addStretch();

    QHBoxLayout *buttonLayout = new QHBoxLayout;
    buttonLayout->addStretch();
    buttonLayout->addWidget(continueButton);
    buttonLayout->addStretch();

    QWidget *buttonContainer = new QWidget(this);
    buttonContainer->setLayout(buttonLayout);
    buttonContainer->setMinimumHeight(100);

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(title);
    mainLayout->addStretch();
    mainLayout->addLayout(devicesLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(buttonContainer, 0, Qt::AlignCenter);
    mainLayout->setContentsMargins(60, 40, 60, 60);

    // === Effet = fade-in/out ===
    opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(0.0);

    // === Thread checker ===
    checker = new DeviceChecker();
    checker->moveToThread(&checkerThread);

    connect(&checkerThread, &QThread::started, checker, &DeviceChecker::checkDevices);
    connect(checker, &DeviceChecker::statusUpdated, this, &CheckDevicesScreen::updateStatus);

    // Destruction propre
    connect(&checkerThread, &QThread::finished, checker, &QObject::deleteLater);
}

CheckDevicesScreen::~CheckDevicesScreen()
{
    checkerThread.quit();
    checkerThread.wait();
}

void CheckDevicesScreen::startChecking()
{
    fadeIn();
    checkerThread.start();
}

void CheckDevicesScreen::updateStatus(bool cameraOk, bool robotOk)
{
    auto imgOk  = QPixmap("./Ressources/image/check_green.png");
    auto imgBad = QPixmap("./Ressources/image/cross_red.png");

    cameraStatusIcon->setPixmap((cameraOk ? imgOk : imgBad)
                                    .scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    robotStatusIcon->setPixmap((robotOk ? imgOk : imgBad)
                                   .scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    cameraStatusLabel->setText(cameraOk ? "Connectée" : "Non connectée");
    robotStatusLabel ->setText(robotOk ? "Connecté"   : "Non connecté");

    cameraStatusLabel->setStyleSheet(QString("font-size:20px; color:%1;")
                                         .arg(cameraOk ? "green" : "red"));
    robotStatusLabel ->setStyleSheet(QString("font-size:20px; color:%1;")
                                        .arg(robotOk ? "green" : "red"));

    continueButton->setVisible(cameraOk && robotOk);

    // Re-check toutes les 2 sec
    if (checkerThread.isRunning())
        QTimer::singleShot(2000, checker, &DeviceChecker::checkDevices);
}

void CheckDevicesScreen::fadeIn()
{
    auto *a = new QPropertyAnimation(opacityEffect, "opacity");
    a->setDuration(800);
    a->setStartValue(0.0);
    a->setEndValue(1.0);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void CheckDevicesScreen::fadeOut()
{
    auto *a = new QPropertyAnimation(opacityEffect, "opacity");
    a->setDuration(800);
    a->setStartValue(1.0);
    a->setEndValue(0.0);

    connect(a, &QPropertyAnimation::finished, this, &CheckDevicesScreen::finishedFadeOut);
    a->start(QAbstractAnimation::DeleteWhenStopped);
}

void CheckDevicesScreen::onContinueClicked()
{
    // Stop checking before leaving this screen
    checkerThread.quit();
    checkerThread.wait();
    disconnect(checker, nullptr, this, nullptr);

    fadeOut();

    connect(this, &CheckDevicesScreen::finishedFadeOut,
            this, &CheckDevicesScreen::readyToContinue);
}
