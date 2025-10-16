#include "Robot.hpp"
#include <thread>
#include <chrono>
#include <cmath>
#include <iostream>
#include <algorithm>

// === Limites mécaniques Dobot ===
constexpr float X_MIN = -260.0f, X_MAX = 300.0f;
constexpr float Y_MIN = -250.0f, Y_MAX = 250.0f;
constexpr float Z_MIN = -140.0f, Z_MAX = 200.0f;
constexpr float R_MIN = -120.0f, R_MAX = 120.0f;
constexpr float Z_SAFE = 150.0f;  // ✅ hauteur de sécurité pour tous les déplacements

Robot::Robot(QObject *parent) : QObject(parent)
{
    columnCoordinates[0] = {258, -111, 88, 10};
    columnCoordinates[1] = {259, -77, 88, 10};
    columnCoordinates[2] = {259, -38, 88, 6};
    columnCoordinates[3] = {259, -6, 88, 6};
    columnCoordinates[4] = {259, 32, 88, 6};
    columnCoordinates[5] = {258, 70, 88, 6};
    columnCoordinates[6] = {257, 107, 88, 6};

    pieceCoordinates[0] = {-58, -220, -126, -90};
    pieceCoordinates[1] = {-13, -220, -126, -90};
    pieceCoordinates[2] = {28, -220, -126, -90};
    pieceCoordinates[3] = {75, -220, -126, -90};
    pieceCoordinates[4] = {-55, 222, -126, 90};
    pieceCoordinates[5] = {-11, 223, -126, 90};
    pieceCoordinates[6] = {33, 223, -126, 90};
    pieceCoordinates[7] = {78, 222, -126, 90};

    loadCalibration();
}

Robot::~Robot() {}

bool Robot::connect()
{
    char dobotNameList[64] = {0};
    int found = SearchDobot(dobotNameList, sizeof(dobotNameList));
    if (found <= 0) return false;

    int result = ConnectDobot(dobotNameList, 115200, nullptr, nullptr);
    if (result != DobotConnect_NoError) return false;

    ClearAllAlarmsState();
    return true;
}

bool Robot::isAvailable()
{
    char list[64] = {0};
    return (SearchDobot(list, sizeof(list)) > 0);
}

void Robot::Home()
{
    HOMECmd homeCmd = {0};
    uint64_t idx = 0;
    SetHOMECmd(&homeCmd, false, &idx);

    // Attente sécurisée (3 à 5 secondes)
    wait(5.0f);
}


// === Calibration ===
void Robot::nextCalibrationStep()
{
    Pose p;
    GetPose(&p);

    switch (currentStep)
    {
    case 1:
        emit calibrationStepChanged(1, "Videz les réservoirs sauf celui de gauche avec un seul pion (position 1).");
        break;
    case 2:
        GetPose(&calib.leftP1);
        emit calibrationStepChanged(2, "Attrapez le pion du réservoir gauche.");
        break;
    case 3:
        GetPose(&calib.leftP2);
        emit calibrationStepChanged(3, "Amenez le pion à la position 4 du réservoir gauche.");
        break;
    case 4:
        GetPose(&calib.rightP1);
        emit calibrationStepChanged(4, "Amenez le pion au réservoir droit, position 1.");
        break;
    case 5:
        GetPose(&calib.rightP2);
        emit calibrationStepChanged(5, "Amenez le pion à la position 4 du réservoir droit.");
        break;
    case 6:
        GetPose(&calib.gridP1);
        emit calibrationStepChanged(6, "Amenez le pion à la colonne gauche de la grille.");
        break;
    case 7:
        GetPose(&calib.gridP2);
        applyCalibration();
        saveCalibration();
        emit calibrationFinished(true);
        break;
    }
    currentStep++;
}

void Robot::recordCalibrationStep()
{
    Pose p;
    GetPose(&p);

    switch (currentStep)
    {
    case 1:
        emit calibrationStepChanged(2,
                                    "Étape 2 : Amenez la pince sur le pion du réservoir gauche (position 1).");
        break;

    case 2:
        GetPose(&calib.leftP1);
        closeGripper();
        wait(0.3f);
        turnOffGripper();
        emit calibrationStepChanged(3,
                                    "Étape 3 : Amenez le pion au-dessus de la position 4 du réservoir gauche.");
        break;

    case 3:
        GetPose(&calib.leftP2);
        emit calibrationStepChanged(4,
                                    "Étape 4 : Amenez le pion au-dessus de la position 1 du réservoir droit.");
        break;

    case 4:
        GetPose(&calib.rightP1);
        emit calibrationStepChanged(5,
                                    "Étape 5 : Amenez le pion au-dessus de la position 4 du réservoir droit.");
        break;

    case 5:
        GetPose(&calib.rightP2);
        emit calibrationStepChanged(6,
                                    "Étape 6 : Amenez le pion au-dessus de la colonne gauche de la grille.");
        break;

    case 6:
        GetPose(&calib.gridP1);
        emit calibrationStepChanged(7,
                                    "Étape 7 : Amenez le pion au-dessus de la colonne droite de la grille, puis cliquez sur 'Suivant'.");
        break;

    case 7:
        GetPose(&calib.gridP2);
        applyCalibration();
        saveCalibration();
        emit calibrationStepChanged(8,
                                    "✅ Calibration terminée.<br>Cliquez sur 'Tester les positions'.");
        emit calibrationFinished(true);
        break;
    }

    currentStep++;
}

// === Réinitialisation calibration ===
void Robot::resetCalibration()
{
    currentStep = 1;
}

// === Application calibration ===
void Robot::applyCalibration()
{
    auto interp = [](Pose p1, Pose p2, int n, int i) {
        Pose p;
        float t = static_cast<float>(i) / (n - 1);
        p.x = p1.x + (p2.x - p1.x) * t;
        p.y = p1.y + (p2.y - p1.y) * t;
        p.z = p1.z + (p2.z - p1.z) * t;
        p.r = p1.r + (p2.r - p1.r) * t;
        return p;
    };

    for (int i = 0; i < 4; ++i)
        pieceCoordinates[i] = interp(calib.leftP1, calib.leftP2, 4, i);
    for (int i = 0; i < 4; ++i)
        pieceCoordinates[4 + i] = interp(calib.rightP1, calib.rightP2, 4, i);
    for (int i = 0; i < 7; ++i)
        columnCoordinates[i] = interp(calib.gridP1, calib.gridP2, 7, i);
}

// === Rotation sécurisée ===
void Robot::rotate(float delta)
{
    Pose p;
    GetPose(&p);

    // Rotation pure sans mouvement vertical
    p.r = std::clamp(p.r + delta, R_MIN, R_MAX);

    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;      // ⚠️ pas de montée / descente ici
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);
    wait(0.2f);
}


// === Commandes gripper ===
void Robot::openGripper()  { gripper(true, false); }
void Robot::closeGripper() { gripper(true, true); }
void Robot::turnOffGripper(){ gripper(false, false); }

void Robot::gripper(bool enable, bool grip)
{
    uint64_t idx = 0;
    SetEndEffectorGripper(enable, grip, false, &idx);
}

void Robot::goTo(Pose p)
{
    // bornes de sécurité simples
    p.x = std::clamp(p.x, X_MIN, X_MAX);
    p.y = std::clamp(p.y, Y_MIN, Y_MAX);
    p.z = std::clamp(p.z, Z_MIN, Z_MAX);
    p.r = std::clamp(p.r, R_MIN, R_MAX);

    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;   // déplacement direct
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);
    wait(0.4f); // rythme modéré
}

void Robot::goTo(Pose p, float z)
{
    p.z = z;
    goTo(p);
}

void Robot::wait(float seconds)
{
    WAITCmd cmd = { static_cast<uint32_t>(seconds * 1000) };
    uint64_t idx = 0;
    SetWAITCmd(&cmd, false, &idx);
}

// === Calibration JSON ===
void Robot::saveCalibration(const QString &path)
{
    QJsonObject o;
    auto toJson = [](Pose p){
        QJsonObject j; j["x"]=p.x; j["y"]=p.y; j["z"]=p.z; j["r"]=p.r; return j; };
    o["gridP1"]=toJson(calib.gridP1);
    o["gridP2"]=toJson(calib.gridP2);
    o["leftP1"]=toJson(calib.leftP1);
    o["leftP2"]=toJson(calib.leftP2);
    o["rightP1"]=toJson(calib.rightP1);
    o["rightP2"]=toJson(calib.rightP2);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) f.write(QJsonDocument(o).toJson());
}

void Robot::loadCalibration(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) return;
    QJsonObject o = QJsonDocument::fromJson(f.readAll()).object();
    auto fromJson = [](QJsonObject j){
        Pose p; p.x=j["x"].toDouble(); p.y=j["y"].toDouble(); p.z=j["z"].toDouble(); p.r=j["r"].toDouble(); return p; };
    calib.gridP1=fromJson(o["gridP1"].toObject());
    calib.gridP2=fromJson(o["gridP2"].toObject());
    calib.leftP1=fromJson(o["leftP1"].toObject());
    calib.leftP2=fromJson(o["leftP2"].toObject());
    calib.rightP1=fromJson(o["rightP1"].toObject());
    calib.rightP2=fromJson(o["rightP2"].toObject());
    applyCalibration();
}

Pose Robot::getColumnPose(int i) const { return columnCoordinates.at(i); }
Pose Robot::getPiecePose(int i) const { return pieceCoordinates.at(i); }
