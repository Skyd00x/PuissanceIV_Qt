#pragma once
#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QString>
#include <vector>
#include <QJsonArray>
#include "Robot.hpp"

// Structure d'une étape de calibration (si tu veux en réutiliser la logique)
struct CalibrationStepData {
    QString name;
    Pose pose;
};

class CalibrationLogic : public QObject {
    Q_OBJECT

public:
    explicit CalibrationLogic(Robot* robot, QObject* parent = nullptr);

    // Gestion de la calibration
    bool connectToRobot();
    void startCalibration();
    void recordStep(int stepIndex);
    void testCalibration();
    void resetCalibration();
    void saveCalibration(const QString& path = "./calibration.json");
    void loadCalibration(const QString& path = "./calibration.json");

    bool isConnected() const { return connected; }
    int currentStep() const { return stepIndex; }

signals:
    void connectionFinished(bool success);
    void progressChanged(int percent);
    void messageUpdated(const QString& text);

private:
    Robot* robot;
    bool connected;
    int stepIndex;
    std::vector<CalibrationStepData> calibrationData;

    void waitForRobotStable();
};
