#pragma once

#include <QObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QFile>
#include <vector>
#include <array>
#include <atomic>
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

    // Déplacements fins sur les axes (par pas de 0.1mm)
    void moveXPlus();
    void moveXMinus();
    void moveYPlus();
    void moveYMinus();
    void moveZPlus();
    void moveZMinus();

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

    // Hauteur de sécurité calculée d'après les points calibrés (max z + 30)
    float getSafeHeight() const;

    // === Fonctions de haut niveau pour manipuler les pions ===
    // Prendre un pion à une position de réservoir (Left_1..Left_4 ou Right_1..Right_4)
    void pickPiece(CalibPoint reservoirPosition);

    // Lâcher un pion dans une colonne de la grille (0..6)
    void dropPiece(int column);

signals:
    void connectionFinished(bool success);
    void robotReady();
    void progressChanged(int value);
    void stepChanged(const CalibrationStep& step, int index);
    void calibrationFinished();
    void calibrationTestFinished();
    void gripperStateChanged(bool isOpen);  // Signal émis quand l'état de la pince change

private:
    std::vector<Pose> interpolatePoints(const Pose& start, const Pose& end, int count);
    void computeAllPositions();

private:
    Robot* robot;
    bool connected;
    int stepIndex;
    bool gripperOpen;

    // Flag pour arrêter les threads en cours
    std::atomic<bool> shouldStop_;

    std::vector<CalibrationStep> steps;
    std::vector<CalibrationStepData> calibrationData; // bruts des étapes manu

    // Nouveau : tableau indexé par CalibPoint
    std::array<Pose, (int)CalibPoint::Count> calibratedPoints;
};
