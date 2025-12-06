#pragma once

#include <thread>
#include <chrono>
#include <atomic>
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
    void emergencyDisconnect(); // Déconnexion d'urgence : arrêt immédiat + coupe compresseur + déconnexion
    static bool isAvailable();   // Vérifie si un Dobot est détectable
    void clearAlarms();   // Clear toutes les alarmes du robot
    void emergencyStop(); // Arrêt d'urgence immédiat du robot

    // === Mouvements ===
    bool Home();                           // Retourne le robot en position Home (retourne false en cas d'échec)
    bool goTo(Pose p, bool precise = false); // Déplacement direct (PTP), precise = vitesse réduite (retourne false en cas d'échec)
    bool goToSecurized(Pose p, float safeZ = 150.0f);  // Déplacement sécurisé avec hauteur de sécurité (retourne false en cas d'échec)
    bool rotate(float delta);              // Rotation relative de la pince (retourne false en cas d'échec)
    bool moveAxis(char axis, float delta); // Déplacement relatif sur un axe ('x', 'y', ou 'z') (retourne false en cas d'échec)
    uint64_t moveAxisContinuous(char axis, float delta); // Version non-bloquante, retourne l'index de la commande
    bool isCommandCompleted(uint64_t commandIndex);      // Vérifie si une commande est terminée

    // === Contrôle de la vitesse ===
    void setNormalSpeed();                 // Vitesse normale (rapide)
    void setPrecisionSpeed();              // Vitesse réduite pour la précision

    // === Contrôle de la pince ===
    bool openGripper();          // Ouvre la pince (retourne false en cas d'échec)
    bool closeGripper();         // Ferme la pince (retourne false en cas d'échec)
    bool turnOffGripper();       // Désactive l'alimentation de la pince (retourne false en cas d'échec)

private:
    // === Méthode interne pour le gripper ===
    bool gripper(bool enable, bool grip);

    // === Attente que la file d'attente du robot atteigne un index ===
    bool waitForCompletion(uint64_t targetIndex, int timeoutSeconds = 5);

    // === Mutex récursif pour protéger l'accès concurrent au robot ===
    // Utilisation d'un QRecursiveMutex pour permettre les appels imbriqués
    // (ex: goToSecurized() appelle goTo() plusieurs fois)
    QRecursiveMutex robotMutex;

    // === Flag d'arrêt d'urgence ===
    // Ce flag est utilisé pour arrêter immédiatement toutes les méthodes de mouvement
    // en cas d'arrêt d'urgence, sans bloquer sur le mutex
    std::atomic<bool> emergencyStopFlag{false};
};

