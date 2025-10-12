#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QThread>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>

class DeviceChecker : public QObject
{
    Q_OBJECT
public:
    explicit DeviceChecker(QObject *parent = nullptr);
    ~DeviceChecker();

signals:
    void statusUpdated(bool cameraOk, bool robotOk);

public slots:
    void checkDevices();  // Vérifie la caméra et le robot
};

class CheckDevicesScreen : public QWidget
{
    Q_OBJECT

public:
    explicit CheckDevicesScreen(QWidget *parent = nullptr);
    ~CheckDevicesScreen();

    void startChecking();
    void fadeIn();
    void fadeOut();

signals:
    void readyToContinue();
    void finishedFadeOut();

private slots:
    void updateStatus(bool cameraOk, bool robotOk);
    void onContinueClicked();

private:
    QLabel *cameraIcon;
    QLabel *robotIcon;
    QLabel *cameraStatusLabel;
    QLabel *robotStatusLabel;
    QLabel *cameraStatusIcon;
    QLabel *robotStatusIcon;
    QPushButton *continueButton;

    QGraphicsOpacityEffect *opacityEffect;
    QThread checkerThread;
    DeviceChecker *checker;
};
