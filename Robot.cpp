#include "Robot.hpp"

Robot::Robot(QObject *parent)
    : QObject(parent)
{
    // Coordonnées par défaut (seront remplacées par calibration)
    columnCoordinates = {{
        {258, -111, 88, 10},
        {259, -77,  88, 10},
        {259, -38,  88, 6},
        {259,  -6,  88, 6},
        {259,  32,  88, 6},
        {258,  70,  88, 6},
        {257, 107,  88, 6}
    }};

    pieceCoordinates = {{
        {-58, -220, -126, -90},
        {-13, -220, -126, -90},
        { 28, -220, -126, -90},
        { 75, -220, -126, -90},
        {-55,  222, -126,  90},
        {-11,  223, -126,  90},
        { 33,  223, -126,  90},
        { 78,  222, -126,  90}
    }};
}

Robot::~Robot() {}

// =====================================================
// === Connexion au robot ===
// =====================================================
bool Robot::connect()
{
    char dobotNameList[64] = {0};
    int found = SearchDobot(dobotNameList, sizeof(dobotNameList));
    if (found <= 0)
        return false;

    int result = ConnectDobot(dobotNameList, 115200, nullptr, nullptr);
    if (result != DobotConnect_NoError)
        return false;

    ClearAllAlarmsState();
    SetQueuedCmdStartExec();
    return true;
}

void Robot::disconnect()
{
    // Stoppe la file d’attente
    SetQueuedCmdStopExec();
    SetQueuedCmdClear();
    DisconnectDobot();
}

bool Robot::isAvailable()
{
    char list[64] = {0};
    return (SearchDobot(list, sizeof(list)) > 0);
}

// =====================================================
// === Retour en position Home ===
// =====================================================
void Robot::Home()
{
    HOMECmd homeCmd = {0};
    uint64_t idx = 0;
    SetQueuedCmdClear();        // vide la file d’attente
    SetQueuedCmdStartExec();    // démarre l’exécution
    SetHOMECmd(&homeCmd, true, &idx);
    waitForCompletion(idx);
}

// =====================================================
// === Déplacements ===
// =====================================================
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
    Pose current;
    GetPose(&current);

    // Étape 1 : remonter à une hauteur sûre pour bouger
    Pose zOk = current;
    zOk.z = current.z + 100.0f;
    if (zOk.z > 150.0f) zOk.z = 150.0f;
    goTo(zOk);

    // Étape 2 : Aller à la cible avec marge hauteur
    Pose TargetHauteurOk = target;
    TargetHauteurOk.z += 50.0f;
    if (TargetHauteurOk.z > 150.0f) TargetHauteurOk.z = 150.0f;
    goTo(TargetHauteurOk);

    // Étape 5 : Aller à la cible
    goTo(target);
}

// =====================================================
// === Rotation de la pince ===
// =====================================================
void Robot::rotate(float delta)
{
    Pose p;
    GetPose(&p);

    p.r = std::clamp(p.r + delta, -100.0f, 100.0f);

    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetQueuedCmdClear();        // vide la file d’attente
    SetQueuedCmdStartExec();    // démarre l’exécution
    SetPTPCmd(&cmd, true, &idx);
    waitForCompletion(idx);
}

// =====================================================
// === Commandes du Gripper ===
// =====================================================
void Robot::openGripper()   { gripper(true, false); }
void Robot::closeGripper()  { gripper(true, true);  }
void Robot::turnOffGripper(){ gripper(false, false); }

void Robot::gripper(bool enable, bool grip)
{
    uint64_t idx = 0;
    SetQueuedCmdClear();        // vide la file d’attente
    SetQueuedCmdStartExec();    // démarre l’exécution
    SetEndEffectorGripper(enable, grip, true, &idx);
}

// =====================================================
// === Temporisation ===
// =====================================================
void Robot::waitForCompletion(uint64_t targetIndex)
{
    uint64_t currentIndex = 0;
    while (true)
    {
        GetQueuedCmdCurrentIndex(&currentIndex);
        if (currentIndex >= targetIndex)
            break;
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}
// =====================================================
// === Accès aux coordonnées ===
// =====================================================
Pose Robot::getColumnPose(int i) const
{
    return columnCoordinates.at(i);
}

Pose Robot::getPiecePose(int i) const
{
    return pieceCoordinates.at(i);
}
