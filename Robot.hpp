#pragma once

#include <thread>
#include <chrono>
#include <QObject>
#include <QMutex>
#include <qdebug.h>
#include <algorithm>
// === SDK Dobot Magician ===
#include "DobotDll.h"

// Pose est fourni par le SDK Dobot : structure contenant {x, y, z, r}
// ---------------------------------------------------------------------

// ============================================================================
//  Classe Robot : Gestion du Dobot Magician (connexion, mouvements, pince…)
// ============================================================================
class Robot : public QObject
{
    Q_OBJECT

public:
    explicit Robot(QObject *parent = nullptr);
    ~Robot() override = default;

    // === Connexion au robot ===
    bool connect();       // Connecte le robot si un Dobot est détecté
    void disconnect();    // Déconnecte proprement et vide la file de commandes
    static bool isAvailable();   // Vérifie si un Dobot est détectable
    void clearAlarms();   // Clear toutes les alarmes du robot

    // === Mouvements ===
    void Home();                           // Retourne le robot en position Home
    void goTo(Pose p, bool precise = false); // Déplacement direct (PTP), precise = vitesse réduite
    void goToSecurized(Pose p, float safeZ = 150.0f);  // Déplacement sécurisé avec hauteur de sécurité
    void rotate(float delta);              // Rotation relative de la pince
    void moveAxis(char axis, float delta); // Déplacement relatif sur un axe ('x', 'y', ou 'z')
    uint64_t moveAxisContinuous(char axis, float delta); // Version non-bloquante, retourne l'index de la commande
    bool isCommandCompleted(uint64_t commandIndex);      // Vérifie si une commande est terminée

    // === Contrôle de la vitesse ===
    void setNormalSpeed();                 // Vitesse normale (rapide)
    void setPrecisionSpeed();              // Vitesse réduite pour la précision

    // === Contrôle de la pince ===
    void openGripper();          // Ouvre la pince
    void closeGripper();         // Ferme la pince
    void turnOffGripper();       // Désactive l’alimentation de la pince

private:
    // === Méthode interne pour le gripper ===
    void gripper(bool enable, bool grip);

    // === Attente que la file d'attente du robot atteigne un index ===
    void waitForCompletion(uint64_t targetIndex);

    // === Mutex récursif pour protéger l'accès concurrent au robot ===
    // Utilisation d'un QRecursiveMutex pour permettre les appels imbriqués
    // (ex: goToSecurized() appelle goTo() plusieurs fois)
    QRecursiveMutex robotMutex;
};

