#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDebug>
#include <array>
#include "DobotDll.h"
#include "DobotType.h"

// === Données de calibration ===
struct CalibrationData {
    Pose gridP1, gridP2;
    Pose leftP1, leftP2;
    Pose rightP1, rightP2;
};

class Robot : public QObject
{
    Q_OBJECT

public:
    explicit Robot(QObject *parent = nullptr);
    ~Robot();

    // === Connexion ===
    bool connect();
    static bool isAvailable();
    void Home();

    // === Calibration ===
    void nextCalibrationStep();
    QString getStepMessage(int step) const;
    void recordCalibrationStep();
    void applyCalibration();
    void saveCalibration(const QString& path = "calibration.json");
    void loadCalibration(const QString& path = "calibration.json");
    void resetCalibration();  // ✅ remet currentStep à 1

    // === Commandes principales ===
    void goTo(Pose position);              // ✅ toujours "safe"
    void goTo(Pose position, float z);
    void wait(float seconds);

    // === Commandes pince ===
    void openGripper();
    void closeGripper();
    void turnOffGripper();

    // === Commande rotation sécurisée ===
    void rotate(float delta);              // rotation protégée

    // === Accès positions calibrées ===
    Pose getColumnPose(int index) const;
    Pose getPiecePose(int index) const;

signals:
    void calibrationStepChanged(int step, const QString &message);
    void calibrationFinished(bool success);

private:
    void gripper(bool enable, bool grip);

    CalibrationData calib;
    int currentStep = 1;

    std::array<Pose, 7> columnCoordinates;
    std::array<Pose, 8> pieceCoordinates;
};
