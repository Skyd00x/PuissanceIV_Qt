#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <vector>
#include "Robot.hpp"

struct CalibrationStep {
    QString text;
    QString imagePath;
    bool showNext;
    bool showBack;
    bool showGripper;
    bool showRotation;
    bool showTest;
    bool showRestart;
    bool showMenu;
};

struct CalibrationStepData {
    QString name;
    Pose pose;
};

class CalibrationLogic : public QObject {
    Q_OBJECT
public:
    explicit CalibrationLogic(Robot* robot, QObject* parent = nullptr);

    bool connectToRobot();
    void homeRobot();
    void startCalibration();
    void recordStep(int index);
    void previousStep();
    void resetCalibration();

    void testCalibration();
    void toggleGripper();
    void rotateLeft();
    void rotateRight();

    void saveCalibration(const QString& path);
    void loadCalibration(const QString& path);

signals:
    void connectionFinished(bool success);
    void robotReady();
    void progressChanged(int value);
    void stepChanged(const CalibrationStep& step, int index);
    void calibrationFinished();
    void calibrationTestFinished();

private:
    void waitForRobotStable();
    std::vector<Pose> interpolatePoints(const Pose& start, const Pose& end, int count);
    void computeAllPositions();  // 🔹 génère tous les points calculés

private:
    Robot* robot;
    bool connected;
    int stepIndex;
    bool gripperOpen;
    std::vector<CalibrationStep> steps;
    std::vector<CalibrationStepData> calibrationData;
};
