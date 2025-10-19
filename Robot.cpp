#include "Robot.hpp"

// === Limites mécaniques Dobot ===
constexpr float X_MIN = -260.0f, X_MAX = 300.0f;
constexpr float Y_MIN = -250.0f, Y_MAX = 250.0f;
constexpr float Z_MIN = -140.0f, Z_MAX = 200.0f;
constexpr float R_MIN = -120.0f, R_MAX = 120.0f;

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
    return true;
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
    SetHOMECmd(&homeCmd, false, &idx);
    wait(5.0f); // Attente de sécurité (5 secondes)
}

bool Robot::isMoving() const
{
    Pose prev, curr;
    GetPose(&prev);
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    GetPose(&curr);

    // Si la différence entre les deux positions est significative → le robot bouge
    auto diff = [](float a, float b) { return std::fabs(a - b); };
    const float threshold = 0.5f; // tolérance (mm)

    bool moving =
        diff(prev.x, curr.x) > threshold ||
        diff(prev.y, curr.y) > threshold ||
        diff(prev.z, curr.z) > threshold ||
        diff(prev.r, curr.r) > 1.0f; // tolérance plus large sur la rotation

    return moving;
}


// =====================================================
// === Déplacements ===
// =====================================================
void Robot::goTo(Pose p)
{
    // Bornes de sécurité
    p.x = std::clamp(p.x, X_MIN, X_MAX);
    p.y = std::clamp(p.y, Y_MIN, Y_MAX);
    p.z = std::clamp(p.z, Z_MIN, Z_MAX);
    p.r = std::clamp(p.r, R_MIN, R_MAX);

    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);
    wait(0.4f);
}

void Robot::goTo(Pose p, float z)
{
    p.z = z;
    goTo(p);
}

// =====================================================
// === Rotation de la pince ===
// =====================================================
void Robot::rotate(float delta)
{
    Pose p;
    GetPose(&p);

    p.r = std::clamp(p.r + delta, R_MIN, R_MAX);

    PTPCmd cmd = {0};
    cmd.ptpMode = PTPMOVJXYZMode;
    cmd.x = p.x;
    cmd.y = p.y;
    cmd.z = p.z;
    cmd.r = p.r;

    uint64_t idx = 0;
    SetPTPCmd(&cmd, false, &idx);
    wait(0.2f);
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
    SetEndEffectorGripper(enable, grip, false, &idx);
}

// =====================================================
// === Temporisation interne ===
// =====================================================
void Robot::wait(float seconds)
{
    WAITCmd cmd = { static_cast<uint32_t>(seconds * 1000) };
    uint64_t idx = 0;
    SetWAITCmd(&cmd, false, &idx);
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
