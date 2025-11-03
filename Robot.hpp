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
// ============================================================================
class Robot : public QObject
{
    Q_OBJECT

public:
    explicit Robot(QObject *parent = nullptr);
    ~Robot();

    // === Connexion et disponibilité ===
    bool connect();
    void disconnect();
    static bool isAvailable();

    // === Mouvements de base ===
    void Home();
    void goTo(Pose p);
    void goToSecurized(Pose target);
    void rotate(float delta);

    // === Contrôle de la pince ===
    void openGripper();
    void closeGripper();
    void turnOffGripper();

    // === Accès aux positions calibrées ===
    Pose getColumnPose(int i) const;
    Pose getPiecePose(int i) const;

    // === Temporisation ===
    void waitForCompletion(uint64_t targetIndex);

private:
    // === Méthodes internes ===
    void gripper(bool enable, bool grip);

    // === Coordonnées calibrées ===
    std::array<Pose, 7> columnCoordinates;
    std::array<Pose, 8> pieceCoordinates;
};
