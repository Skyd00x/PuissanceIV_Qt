#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <vector>
#include <array>
#include "Robot.hpp"

// =============================
//  Enum indexant tous les points
// =============================
enum class CalibPoint {
    Left_1, Left_2, Left_3, Left_4,
    Right_1, Right_2, Right_3, Right_4,
    Grid_1, Grid_2, Grid_3, Grid_4, Grid_5, Grid_6, Grid_7,

    Count // toujours dernier
};

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
    void disconnectToRobot();
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

    // Accès direct par enum
    Pose getPosition(CalibPoint p) const {
        return calibratedPoints[(int)p];
    }

    // === Nouveaux helpers pour le jeu ===
    // Position de prise d'un pion dans la réserve
    Pose getPosePick() const;

    // Position pour déposer dans une colonne de la grille (0..6)
    Pose getPoseForColumn(int col) const;

signals:
    void connectionFinished(bool success);
    void robotReady();
    void progressChanged(int value);
    void stepChanged(const CalibrationStep& step, int index);
    void calibrationFinished();
    void calibrationTestFinished();

private:
    std::vector<Pose> interpolatePoints(const Pose& start, const Pose& end, int count);
    void computeAllPositions();

private:
    Robot* robot;
    bool connected;
    int stepIndex;
    bool gripperOpen;

    std::vector<CalibrationStep> steps;
    std::vector<CalibrationStepData> calibrationData; // bruts des étapes manu

    // Nouveau : tableau indexé par CalibPoint
    std::array<Pose, (int)CalibPoint::Count> calibratedPoints;
};
