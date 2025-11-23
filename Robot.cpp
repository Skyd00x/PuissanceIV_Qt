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
    HOMECmd homeCmd = {0};
    uint64_t idx = 0;

    SetQueuedCmdClear();
    SetQueuedCmdStartExec();
    SetHOMECmd(&homeCmd, true, &idx);

    waitForCompletion(idx);
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
    // Récupère la pose actuelle
    Pose current;
    GetPose(&current);

    // Étape 1 : remonter pour éviter les collisions
    Pose zSafe = current;
    zSafe.z = std::min(current.z + 100.0f, 150.0f);
    goTo(zSafe);

    // Étape 2 : se positionner au-dessus de la cible
    Pose targetSafe = target;
    targetSafe.z = std::min(target.z + 50.0f, 150.0f);
    goTo(targetSafe);

    // Étape 3 : descente finale
    goTo(target);
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
