#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <vector>
#include "Robot.hpp"

struct CalibrationStepData {
    QString name;
    Pose pose;
};

class CalibrationLogic : public QObject
{
    Q_OBJECT

public:
    explicit CalibrationLogic(Robot* robot, QObject* parent = nullptr);

    bool connectToRobot();
    void waitForRobotStable();

public slots:
    // === Commandes principales ===
    void homeRobot();
    void startCalibration();
    void recordStep(int index);
    void testCalibration();
    void resetCalibration();

    // === Commandes de manipulation du robot ===
    void toggleGripper();   // ✅ déplacement depuis Screen
    void rotateLeft();      // ✅ déplacement depuis Screen
    void rotateRight();     // ✅ déplacement depuis Screen

    // === Sauvegarde / chargement ===
    void saveCalibration(const QString& path);
    void loadCalibration(const QString& path);

signals:
    void connectionFinished(bool success);
    void robotReady();
    void progressChanged(int value);

private:
    Robot* robot;
    bool connected;
    int stepIndex;
    bool gripperOpen = false;
    std::vector<CalibrationStepData> calibrationData;
};
