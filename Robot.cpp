#include "Robot.hpp"

// ============================================================================
//  Constructeur
//  Charge les tableaux de coordonnées (normalement définis via calibration)
// ============================================================================
Robot::Robot(QObject *parent)
    : QObject(parent)
{

}

// ============================================================================
//  Connexion / déconnexion au robot
// ============================================================================
bool Robot::connect()
{
    char dobotNameList[64] = {0};

    // Recherche d’un Dobot connecté
    if (SearchDobot(dobotNameList, sizeof(dobotNameList)) <= 0)
        return false;

    // Tentative de connexion
    if (ConnectDobot(dobotNameList, 115200, nullptr, nullptr) != DobotConnect_NoError)
        return false;

    // Nettoyage de l'état et démarrage de la file d'exécution
    ClearAllAlarmsState();
    SetQueuedCmdStartExec();

    // Configurer la vitesse normale par défaut
    setNormalSpeed();

    return true;
}

void Robot::disconnect()
{
    SetQueuedCmdStopExec();
    SetQueuedCmdClear();
    DisconnectDobot();
}

bool Robot::isAvailable()
{
    char list[64] = {0};
    return SearchDobot(list, sizeof(list)) > 0;
}

void Robot::clearAlarms()
{
    ClearAllAlarmsState();
    qDebug() << "[Robot] Alarmes clearées";
}

// ============================================================================
//  Contrôle de la vitesse
// ============================================================================
void Robot::setNormalSpeed()
{
    // Vitesse normale pour les déplacements rapides
    PTPCoordinateParams coordParams;
    coordParams.xyzVelocity = 200.0f;      // mm/s
    coordParams.xyzAcceleration = 200.0f;  // mm/s²
    coordParams.rVelocity = 200.0f;        // °/s
    coordParams.rAcceleration = 200.0f;    // °/s²
    SetPTPCoordinateParams(&coordParams, false, nullptr);

    PTPCommonParams commonParams;
    commonParams.velocityRatio = 100;      // 100% de la vitesse
    commonParams.accelerationRatio = 100;  // 100% de l'accélération
    SetPTPCommonParams(&commonParams, false, nullptr);

    qDebug() << "[Robot] Vitesse normale activée";
}

void Robot::setPrecisionSpeed()
{
    // Vitesse très réduite pour les mouvements de précision (descente au-dessus de la grille/pion)
    PTPCoordinateParams coordParams;
    coordParams.xyzVelocity = 30.0f;       // mm/s (réduit de 85%)
    coordParams.xyzAcceleration = 30.0f;   // mm/s² (réduit de 85%)
    coordParams.rVelocity = 30.0f;         // °/s
    coordParams.rAcceleration = 30.0f;     // °/s²
    SetPTPCoordinateParams(&coordParams, false, nullptr);

    PTPCommonParams commonParams;
    commonParams.velocityRatio = 15;       // 15% de la vitesse (très lent)
    commonParams.accelerationRatio = 15;   // 15% de l'accélération
    SetPTPCommonParams(&commonParams, false, nullptr);

    qDebug() << "[Robot] Vitesse de précision activée (15% - très lent pour précision maximale)";
}

// ============================================================================
//  Position Home
// ============================================================================
void Robot::Home()
{
    qDebug() << "[Robot] 🔒 Home() - Tentative de verrouillage du mutex...";
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex pour toute la durée de la fonction
    qDebug() << "[Robot] ✅ ==== DÉBUT Home() ==== (Mutex verrouillé, accès exclusif au robot)";

    // ÉTAPE 0 : FORCER L'ARRÊT de toute commande en cours pour éviter les conflits
    qDebug() << "[Robot] Arrêt forcé de toute commande en cours...";
    SetQueuedCmdForceStopExec();
    std::this_thread::sleep_for(std::chrono::milliseconds(500)); // Laisser le temps au robot de s'arrêter complètement
    qDebug() << "[Robot] Robot arrêté, prêt pour Home";

    // ÉTAPE 1 : Monter à la hauteur de sécurité (Z de la grille = 104.0f)
    qDebug() << "[Robot] Récupération de la position actuelle...";
    Pose current;
    GetPose(&current);
    qDebug() << "[Robot] Position actuelle : X=" << current.x << " Y=" << current.y << " Z=" << current.z << " R=" << current.r;

    // Créer une nouvelle position avec le Z du point générique de la grille (104.0f)
    // Cette hauteur est la même que celle utilisée dans la calibration
    const float GRID_SAFE_Z = 104.0f;  // Hauteur du point générique au-dessus de la grille
    Pose safePos = current;
    safePos.z = GRID_SAFE_Z;
    qDebug() << "[Robot] Montée à la hauteur de sécurité (Z grille) : Z=" << GRID_SAFE_Z;

    // Effacer la queue et préparer pour la montée
    SetQueuedCmdClear();
    SetQueuedCmdStartExec();

    // Monter à la position de sécurité
    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = safePos.x;
    cmd.y = safePos.y;
    cmd.z = safePos.z;
    cmd.r = safePos.r;

    uint64_t safeIdx = 0;
    SetPTPCmd(&cmd, true, &safeIdx);
    waitForCompletion(safeIdx);
    qDebug() << "[Robot] Position de sécurité atteinte";

    // ÉTAPE 2 : Retour à la position Home
    HOMECmd homeCmd = {0};
    uint64_t idx = 0;

    qDebug() << "[Robot] Appel SetQueuedCmdClear()...";
    SetQueuedCmdClear();
    qDebug() << "[Robot] Appel SetQueuedCmdStartExec()...";
    SetQueuedCmdStartExec();
    qDebug() << "[Robot] Appel SetHOMECmd() - Cette commande peut faire plusieurs mouvements physiques";
    SetHOMECmd(&homeCmd, true, &idx);

    qDebug() << "[Robot] Attente de la fin du mouvement Home (idx=" << idx << ")...";
    waitForCompletion(idx);

    // CRITIQUE : Attente supplémentaire pour garantir que le mouvement physique est VRAIMENT terminé
    // La commande HOME peut prendre plusieurs secondes, on attend que la queue soit complètement vide
    qDebug() << "[Robot] Home() - Vérification finale que la queue est vide...";
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    uint64_t finalIndex = 0;
    GetQueuedCmdCurrentIndex(&finalIndex);
    qDebug() << "[Robot] Home() - Index final de la queue : " << finalIndex << " (devrait être >= " << idx << ")";

    // Attendre un peu de plus pour que le robot se stabilise physiquement
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    qDebug() << "[Robot] ==== FIN Home() ==== (Mouvement physique terminé, Mutex déverrouillé)";
    // Le mutex sera automatiquement déverrouillé à la sortie de la fonction (QMutexLocker)
}

// ============================================================================
//  Déplacements
// ============================================================================
void Robot::goTo(Pose p, bool precise)
{
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex
    qDebug() << "[Robot] goTo() - Mutex verrouillé";

    // Clear les alarmes avant le mouvement pour éviter les blocages
    clearAlarms();

    // Activer la vitesse de précision si demandé
    if (precise) {
        setPrecisionSpeed();
    }

    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);
    waitForCompletion(idx);

    // Revenir à la vitesse normale après le mouvement de précision
    if (precise) {
        setNormalSpeed();
    }

    qDebug() << "[Robot] goTo() - Mutex déverrouillé";
}

void Robot::goToSecurized(Pose target, float safeZ)
{
    qDebug() << "[Robot] 🔒 goToSecurized() - Tentative de verrouillage du mutex...";
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex (récursif)
    qDebug() << "[Robot] ✅ goToSecurized() - Mutex verrouillé (vers X=" << target.x << ", Y=" << target.y << ", Z=" << target.z << ")";

    // === SYSTÈME DE POINTS DE PASSAGE POUR ÉVITER LES COLLISIONS ===
    // 1. Monter à la hauteur de sécurité (safeZ)
    // 2. Se déplacer horizontalement à safeZ
    // 3. Descendre RAPIDEMENT jusqu'à 15mm au-dessus de la cible
    // 4. Descendre LENTEMENT les derniers 15mm pour la précision

    // Récupère la pose actuelle
    Pose current;
    GetPose(&current);

    // Étape 1 : Monter à safeZ avec la position actuelle (x, y) - vitesse normale
    qDebug() << "[Robot] Étape 1/4 : Montée à z=" << safeZ << " (sécurité)";
    Pose stepUp = current;
    stepUp.z = safeZ;
    goTo(stepUp, false);  // Vitesse normale

    // Étape 2 : Se déplacer horizontalement au-dessus de la cible à safeZ - vitesse normale
    qDebug() << "[Robot] Étape 2/4 : Déplacement horizontal vers (x=" << target.x << ", y=" << target.y << ", z=" << safeZ << ")";
    Pose stepOver = target;
    stepOver.z = safeZ;
    goTo(stepOver, false);  // Vitesse normale

    // Étape 3 : Descendre RAPIDEMENT jusqu'à 15mm au-dessus de la cible
    const float approachDistance = 15.0f;  // Distance finale à parcourir lentement
    qDebug() << "[Robot] Étape 3/4 : Descente RAPIDE jusqu'à z=" << (target.z + approachDistance);
    Pose stepApproach = target;
    stepApproach.z = target.z + approachDistance;
    goTo(stepApproach, false);  // Vitesse normale

    // Étape 4 : Descendre LENTEMENT les derniers millimètres pour la précision
    qDebug() << "[Robot] Étape 4/4 : Descente PRÉCISE finale à z=" << target.z;
    goTo(target, true);  // VITESSE RÉDUITE pour la précision

    qDebug() << "[Robot] Déplacement sécurisé terminé - Mutex déverrouillé";
}

void Robot::rotate(float delta)
{
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex
    qDebug() << "[Robot] rotate() - Mutex verrouillé";

    // Clear les alarmes avant le mouvement
    clearAlarms();

    // Récupère la pose actuelle
    Pose p;
    GetPose(&p);

    // Clamp de la rotation
    p.r = std::clamp(p.r + delta, -100.0f, 100.0f);

    // Envoie de la commande
    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;

    // On utilise une queue dédiée pour cette action
    SetQueuedCmdClear();
    SetQueuedCmdStartExec();
    SetPTPCmd(&cmd, true, &idx);

    waitForCompletion(idx);

    qDebug() << "[Robot] rotate() - Mutex déverrouillé";
}

void Robot::moveAxis(char axis, float delta)
{
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex
    qDebug() << "[Robot] moveAxis() - Mutex verrouillé";

    // Clear les alarmes avant le mouvement
    clearAlarms();

    // Récupère la pose actuelle
    Pose p;
    GetPose(&p);

    // Applique le delta sur l'axe spécifié
    switch (axis) {
        case 'x':
        case 'X':
            p.x += delta;
            qDebug() << "[Robot] Déplacement X de" << delta << "mm -> nouvelle position X =" << p.x;
            break;
        case 'y':
        case 'Y':
            p.y += delta;
            qDebug() << "[Robot] Déplacement Y de" << delta << "mm -> nouvelle position Y =" << p.y;
            break;
        case 'z':
        case 'Z':
            p.z += delta;
            qDebug() << "[Robot] Déplacement Z de" << delta << "mm -> nouvelle position Z =" << p.z;
            break;
        default:
            qWarning() << "[Robot] Axe invalide:" << axis;
            return;
    }

    // Envoie de la commande
    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;

    // On utilise une queue dédiée pour cette action
    SetQueuedCmdClear();
    SetQueuedCmdStartExec();
    SetPTPCmd(&cmd, true, &idx);

    waitForCompletion(idx);

    qDebug() << "[Robot] moveAxis() - Mutex déverrouillé";
}

uint64_t Robot::moveAxisContinuous(char axis, float delta)
{
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex
    qDebug() << "[Robot] moveAxisContinuous() - Mutex verrouillé (non-bloquant)";

    // Version non-bloquante pour mouvements continus (retourne l'index de la commande)
    // Récupère la pose actuelle
    Pose p;
    GetPose(&p);

    // Applique le delta sur l'axe spécifié
    switch (axis) {
        case 'x':
        case 'X':
            p.x += delta;
            break;
        case 'y':
        case 'Y':
            p.y += delta;
            break;
        case 'z':
        case 'Z':
            p.z += delta;
            break;
        default:
            return 0;
    }

    // Envoie de la commande SANS bloquer
    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);  // false = ne pas clear la queue

    qDebug() << "[Robot] moveAxisContinuous() - Commande envoyée, Mutex déverrouillé";
    return idx;  // Retourne l'index pour vérifier la complétion
}

bool Robot::isCommandCompleted(uint64_t commandIndex)
{
    // Vérifie si une commande est terminée
    uint64_t currentIndex = 0;
    GetQueuedCmdCurrentIndex(&currentIndex);
    return currentIndex >= commandIndex;
}

// ============================================================================
//  Gripper
// ============================================================================
void Robot::openGripper()    { gripper(true, false); }
void Robot::closeGripper()   { gripper(true, true); }
void Robot::turnOffGripper() { gripper(false, false); }

void Robot::gripper(bool enable, bool grip)
{
    qDebug() << "[Robot] 🔒 gripper() - Tentative de verrouillage du mutex...";
    QMutexLocker locker(&robotMutex);  // Verrouille le mutex
    qDebug() << "[Robot] ✅ gripper() - Mutex verrouillé (enable=" << enable << ", grip=" << grip << ")";

    uint64_t idx = 0;

    SetQueuedCmdClear();
    SetQueuedCmdStartExec();
    SetEndEffectorGripper(enable, grip, true, &idx);

    waitForCompletion(idx);

    qDebug() << "[Robot] gripper() - Mutex déverrouillé";
}

// ============================================================================
//  Attente de fin d'exécution (avec timeout et détection de blocage)
// ============================================================================
void Robot::waitForCompletion(uint64_t targetIndex)
{
    const int TIMEOUT_SECONDS = 90;  // Timeout global augmenté (Home peut être très long)
    const int STUCK_THRESHOLD = 400;  // 400 x 50ms = 20 secondes sans mouvement (Home peut être lent)

    auto start = std::chrono::steady_clock::now();
    uint64_t currentIndex = 0;
    uint64_t lastIndex = 0;
    int stuckCount = 0;

    qDebug() << "[Robot] ⏳ Attente de la commande idx=" << targetIndex << " (timeout=" << TIMEOUT_SECONDS << "s, stuck=" << (STUCK_THRESHOLD*50)/1000 << "s)";

    while (true)
    {
        GetQueuedCmdCurrentIndex(&currentIndex);

        // Succès : l'index a atteint ou dépassé la commande visée
        if (currentIndex >= targetIndex) {
            qDebug() << "[Robot] ✅ Commande terminée (currentIndex=" << currentIndex << ", target=" << targetIndex << ")";
            return;
        }

        // Détection de blocage : l'index ne progresse plus
        if (currentIndex == lastIndex) {
            stuckCount++;
            if (stuckCount >= STUCK_THRESHOLD) {
                qWarning() << "[Robot] ⚠️ ATTENTION : Le robot semble bloqué (index=" << currentIndex << " depuis " << (STUCK_THRESHOLD*50)/1000 << "s, target=" << targetIndex << ")";
                qWarning() << "[Robot] Vérifier les alarmes et l'état du robot";
                // NE PAS retourner immédiatement, vérifier le timeout global d'abord
                // pour donner plus de temps aux mouvements très lents
            }
        } else {
            // L'index a bougé, réinitialiser le compteur de blocage
            if (stuckCount > 0) {
                qDebug() << "[Robot] ⏩ Index progresse: " << lastIndex << " -> " << currentIndex;
            }
            stuckCount = 0;
        }
        lastIndex = currentIndex;

        // Vérification du timeout global
        auto elapsed = std::chrono::duration_cast<std::chrono::seconds>(
            std::chrono::steady_clock::now() - start
        ).count();

        if (elapsed > TIMEOUT_SECONDS) {
            qWarning() << "[Robot] ⏱️ TIMEOUT : La commande n'a pas terminé après " << TIMEOUT_SECONDS << "s";
            qWarning() << "[Robot] currentIndex=" << currentIndex << ", targetIndex=" << targetIndex;
            return;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
