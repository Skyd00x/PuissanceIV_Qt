#pragma once

#include <QObject>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QString>
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <thread>
#include <chrono>
#include <array>

// === SDK Dobot Magician ===
#include "DobotDll.h"

// ============================================================================
//  Classe Robot : Gère le pilotage matériel du Dobot Magician
//  (connexion, mouvements, pince, rotations, etc.)
//  → La calibration est désormais gérée dans CalibrationScreen
// ============================================================================
class Robot : public QObject
{
    Q_OBJECT

public:
    explicit Robot(QObject *parent = nullptr);
    ~Robot();

    // === Connexion et disponibilité ===
    bool connect();
    static bool isAvailable();   // ✅ statique pour utilisation sans instance

    // === Mouvements de base ===
    void Home();
    bool isMoving() const;
    void goTo(Pose p);
    void goTo(Pose p, float z);
    void rotate(float delta);

    // === Contrôle de la pince ===
    void openGripper();
    void closeGripper();
    void turnOffGripper();

    // === Accès aux positions calibrées ===
    Pose getColumnPose(int i) const;
    Pose getPiecePose(int i) const;

    // === Temporisation ===
    void wait(float seconds);

private:
    // === Méthodes internes ===
    void gripper(bool enable, bool grip);

    // === Coordonnées calibrées ===
    std::array<Pose, 7> columnCoordinates;
    std::array<Pose, 8> pieceCoordinates;
};
