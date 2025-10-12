#include "CheckDevicesScreen.hpp"
#include "Camera.hpp"
#include "Robot.hpp"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFile>
#include <QDebug>
#include <QTimer>

// ====================== DeviceChecker ======================

DeviceChecker::DeviceChecker(QObject *parent)
    : QObject(parent)
{
}

DeviceChecker::~DeviceChecker() {}

void DeviceChecker::checkDevices()
{
    bool camOk = Camera::isAvailable();
    bool robOk = Robot::isAvailable();
    emit statusUpdated(camOk, robOk);
}

// ====================== CheckDevicesScreen ======================

CheckDevicesScreen::CheckDevicesScreen(QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: white;");
    setAttribute(Qt::WA_StyledBackground, true);

    QLabel *title = new QLabel("Vérification des équipements nécessaires");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size: 28px; font-weight: bold; color: black;");

    // === Icônes caméra et robot ===
    cameraIcon = new QLabel(this);
    QString cameraPath = "./Ressources/image/camera.png";
    if (!QFile::exists(cameraPath))
        qWarning() << "Image caméra introuvable:" << cameraPath;
    QPixmap cameraPixmap(cameraPath);
    if (cameraPixmap.isNull())
        qWarning() << "Échec chargement caméra:" << cameraPath;
    else
        cameraIcon->setPixmap(cameraPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    cameraIcon->setAlignment(Qt::AlignCenter);

    robotIcon = new QLabel(this);
    QString robotPath = "./Ressources/image/robot.png";
    if (!QFile::exists(robotPath))
        qWarning() << "Image robot introuvable:" << robotPath;
    QPixmap robotPixmap(robotPath);
    if (robotPixmap.isNull())
        qWarning() << "Échec chargement robot:" << robotPath;
    else
        robotIcon->setPixmap(robotPixmap.scaled(128, 128, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    robotIcon->setAlignment(Qt::AlignCenter);

    // === Statuts ===
    cameraStatusIcon = new QLabel(this);
    robotStatusIcon  = new QLabel(this);
    cameraStatusLabel = new QLabel("Vérification...");
    robotStatusLabel  = new QLabel("Vérification...");
    cameraStatusIcon->setAlignment(Qt::AlignCenter);
    robotStatusIcon->setAlignment(Qt::AlignCenter);
    cameraStatusLabel->setAlignment(Qt::AlignCenter);
    robotStatusLabel->setAlignment(Qt::AlignCenter);

    cameraStatusLabel->setStyleSheet("font-size: 20px; color: black;");
    robotStatusLabel->setStyleSheet("font-size: 20px; color: black;");

    // === Bouton continuer ===
    continueButton = new QPushButton("Continuer", this);
    continueButton->setVisible(false);
    continueButton->setStyleSheet(
        "QPushButton { background-color: #2ecc71; color: white; font-size: 22px; "
        "font-weight: bold; border-radius: 12px; padding: 10px 40px; }"
        "QPushButton:hover { background-color: #27ae60; }"
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
    devicesLayout->addSpacing(100);
    devicesLayout->addLayout(robotLayout);
    devicesLayout->addStretch();

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(title);
    mainLayout->addStretch();
    mainLayout->addLayout(devicesLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(continueButton, 0, Qt::AlignCenter);
    setLayout(mainLayout);

    // === Effet de fondu ===
    opacityEffect = new QGraphicsOpacityEffect(this);
    setGraphicsEffect(opacityEffect);
    opacityEffect->setOpacity(0.0);

    // === Thread de vérification ===
    checker = new DeviceChecker();
    checker->moveToThread(&checkerThread);
    connect(&checkerThread, &QThread::started, checker, &DeviceChecker::checkDevices);
    connect(checker, &DeviceChecker::statusUpdated, this, &CheckDevicesScreen::updateStatus);
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
    QString checkGreenPath = "./Ressources/image/check_green.png";
    QString crossRedPath   = "./Ressources/image/cross_red.png";

    QPixmap checkGreen(checkGreenPath);
    QPixmap crossRed(crossRedPath);

    if (checkGreen.isNull()) qWarning() << "Image check_green introuvable:" << checkGreenPath;
    if (crossRed.isNull()) qWarning() << "Image cross_red introuvable:" << crossRedPath;

    cameraStatusIcon->setPixmap((cameraOk ? checkGreen : crossRed)
                                    .scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));
    robotStatusIcon->setPixmap((robotOk ? checkGreen : crossRed)
                                   .scaled(48,48,Qt::KeepAspectRatio,Qt::SmoothTransformation));

    cameraStatusLabel->setText(cameraOk ? "Connecté" : "Non connecté");
    robotStatusLabel->setText(robotOk ? "Connecté" : "Non connecté");
    cameraStatusLabel->setStyleSheet(QString("font-size:20px; color:%1;").arg(cameraOk ? "green" : "red"));
    robotStatusLabel->setStyleSheet(QString("font-size:20px; color:%1;").arg(robotOk ? "green" : "red"));

    continueButton->setVisible(cameraOk && robotOk);

    // Vérification périodique
    if (checkerThread.isRunning())
        QTimer::singleShot(2000, checker, &DeviceChecker::checkDevices);
}

void CheckDevicesScreen::fadeIn()
{
    QPropertyAnimation *anim = new QPropertyAnimation(opacityEffect, "opacity");
    anim->setDuration(1000);
    anim->setStartValue(0.0);
    anim->setEndValue(1.0);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CheckDevicesScreen::fadeOut()
{
    QPropertyAnimation *anim = new QPropertyAnimation(opacityEffect, "opacity");
    anim->setDuration(1000);
    anim->setStartValue(1.0);
    anim->setEndValue(0.0);
    connect(anim, &QPropertyAnimation::finished, this, &CheckDevicesScreen::finishedFadeOut);
    anim->start(QAbstractAnimation::DeleteWhenStopped);
}

void CheckDevicesScreen::onContinueClicked()
{
    fadeOut();
    connect(this, &CheckDevicesScreen::finishedFadeOut, this, &CheckDevicesScreen::readyToContinue);
}
