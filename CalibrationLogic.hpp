#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <vector>
#include "Robot.hpp"

// === Données de calibration (positions enregistrées) ===
struct CalibrationStepData {
    QString name;
    Pose pose;
};

// === Étapes de procédure (affichage UI) ===
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
    void previousStep();

    // === Commandes de manipulation du robot ===
    void toggleGripper();
    void rotateLeft();
    void rotateRight();

    // === Sauvegarde / chargement ===
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
    Robot* robot;
    bool connected;
    int stepIndex;
    bool gripperOpen = false;

    std::vector<CalibrationStepData> calibrationData;
    std::vector<CalibrationStep> steps;
};
