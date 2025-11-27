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

    // Nettoyage de l’état et démarrage de la file d’exécution
    ClearAllAlarmsState();
    SetQueuedCmdStartExec();
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

// ============================================================================
//  Position Home
// ============================================================================
void Robot::Home()
{
    qDebug() << "[Robot] ==== DÉBUT Home() ====";
    HOMECmd homeCmd = {0};
    uint64_t idx = 0;

    qDebug() << "[Robot] Appel SetQueuedCmdClear()...";
    SetQueuedCmdClear();
    qDebug() << "[Robot] Appel SetQueuedCmdStartExec()...";
    SetQueuedCmdStartExec();
    qDebug() << "[Robot] Appel SetHOMECmd() - Cette commande peut faire plusieurs mouvements physiques";
    SetHOMECmd(&homeCmd, true, &idx);

    qDebug() << "[Robot] Attente de la fin du mouvement (idx=" << idx << ")...";
    waitForCompletion(idx);
    qDebug() << "[Robot] ==== FIN Home() ====";
}

// ============================================================================
//  Déplacements
// ============================================================================
void Robot::goTo(Pose p)
{
    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);
    waitForCompletion(idx);
}

void Robot::goToSecurized(Pose target)
{
    // === SYSTÈME DE POINTS DE PASSAGE POUR ÉVITER LES COLLISIONS ===
    // 1. Monter à z=150 (position de sécurité)
    // 2. Se déplacer horizontalement à z=150
    // 3. Descendre à la position cible

    // Récupère la pose actuelle
    Pose current;
    GetPose(&current);

    // Étape 1 : Monter à z=150 avec la position actuelle (x, y)
    qDebug() << "[Robot] Étape 1/3 : Montée à z=150 (sécurité)";
    Pose stepUp = current;
    stepUp.z = 150.0f;
    goTo(stepUp);

    // Étape 2 : Se déplacer horizontalement au-dessus de la cible à z=150
    qDebug() << "[Robot] Étape 2/3 : Déplacement horizontal vers (x=" << target.x << ", y=" << target.y << ", z=150)";
    Pose stepOver = target;
    stepOver.z = 150.0f;
    goTo(stepOver);

    // Étape 3 : Descendre à la position cible finale
    qDebug() << "[Robot] Étape 3/3 : Descente à z=" << target.z;
    goTo(target);

    qDebug() << "[Robot] Déplacement sécurisé terminé";
}

void Robot::rotate(float delta)
{
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
}

// ============================================================================
//  Gripper
// ============================================================================
void Robot::openGripper()    { gripper(true, false); }
void Robot::closeGripper()   { gripper(true, true); }
void Robot::turnOffGripper() { gripper(false, false); }

void Robot::gripper(bool enable, bool grip)
{
    uint64_t idx = 0;

    SetQueuedCmdClear();
    SetQueuedCmdStartExec();
    SetEndEffectorGripper(enable, grip, true, &idx);

    waitForCompletion(idx);
}

// ============================================================================
//  Attente de fin d'exécution
// ============================================================================
void Robot::waitForCompletion(uint64_t targetIndex)
{
    uint64_t currentIndex = 0;

    // Boucle tant que l’index courant n’a pas atteint la commande visée
    while (true)
    {
        GetQueuedCmdCurrentIndex(&currentIndex);
        if (currentIndex >= targetIndex)
            break;

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
