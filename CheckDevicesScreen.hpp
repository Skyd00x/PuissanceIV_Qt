#pragma once

#include <QWidget>
#include <QThread>
#include <QLabel>
#include <QPushButton>
#include <QGraphicsOpacityEffect>

class DeviceChecker : public QObject
{
    Q_OBJECT

public:
    explicit DeviceChecker(QObject *parent = nullptr);
    ~DeviceChecker();

signals:
    void statusUpdated(bool cameraOk, bool robotOk);

public slots:
    void checkDevices();
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
    QLabel *cameraIcon = nullptr;
    QLabel *robotIcon = nullptr;
    QLabel *cameraStatusLabel = nullptr;
    QLabel *robotStatusLabel = nullptr;
    QLabel *cameraStatusIcon = nullptr;
    QLabel *robotStatusIcon = nullptr;
    QPushButton *continueButton = nullptr;

    QGraphicsOpacityEffect *opacityEffect = nullptr;

    QThread checkerThread;
    DeviceChecker *checker = nullptr;
};
