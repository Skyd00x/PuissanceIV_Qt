#include "Robot.hpp"
#include <string>
#include <thread>
#include <chrono>

Robot::Robot()
{
    columnCoordinates[0] = { 258, -111, 88, 10 };
    columnCoordinates[1] = { 259, -77, 88, 10 };
    columnCoordinates[2] = { 259, -38, 88, 6 };
    columnCoordinates[3] = { 259, -6, 88, 6 };
    columnCoordinates[4] = { 259, 32, 88, 6 };
    columnCoordinates[5] = { 258, 70, 88, 6 };
    columnCoordinates[6] = { 257, 107, 88, 6 };

    pieceCoordinates[0] = { -58, -220, -126, -90 };
    pieceCoordinates[1] = { -13, -220, -126, -90 };
    pieceCoordinates[2] = { 28, -220, -126, -90 };
    pieceCoordinates[3] = { 75, -220, -126, -90 };

    pieceCoordinates[4] = { -55, 222, -126, 90 };
    pieceCoordinates[5] = { -11, 223, -126, 90 };
    pieceCoordinates[6] = { 33, 223, -126, 90 };
    pieceCoordinates[7] = { 78, 222, -126, 90 };
}

Robot::~Robot() {}

bool Robot::connect()
{
    char dobotNameList[64] = {0};
    int found = SearchDobot(dobotNameList, sizeof(dobotNameList));
    if (found <= 0) {
        std::cerr << "❌ Aucun Dobot détecté." << std::endl;
        return false;
    }

    std::cout << "🔌 Connexion à : " << dobotNameList << std::endl;
    int result = ConnectDobot(dobotNameList, 115200, nullptr, nullptr);
    if (result != DobotConnect_NoError) {
        std::cerr << "❌ Erreur de connexion (code " << result << ")" << std::endl;
        return false;
    }

    std::cout << "✅ Dobot connecté." << std::endl;

    ClearAllAlarmsState();

    JOGCoordinateParams jogParams;
    if (GetJOGCoordinateParams(&jogParams) == DobotCommunicate_NoError) {
        std::cout << "⚙️  Paramètres JOG récupérés : "
                  << "Vx=" << jogParams.velocity[0] << ", Vy=" << jogParams.velocity[1]
                  << ", Vz=" << jogParams.velocity[2] << ", Vr=" << jogParams.velocity[3]
                  << std::endl;
    }

    return true;
}

bool Robot::isAvailable()
{
    // Essayons d’éviter le scan complet du réseau de SearchDobot()
    // en tentant directement de se connecter sur un port connu ou un port valide.
    char dobotNameList[64] = {0};

    // Utilisation de SearchDobot, mais avec une durée limitée
    // pour ne pas bloquer le thread trop longtemps.
    int found = SearchDobot(dobotNameList, sizeof(dobotNameList));

    // Si la recherche échoue (pas trouvé ou timeout), on peut tester les ports COM manuellement.
    if (found <= 0)
    {
        // Petite détection manuelle basique :
        // On essaie de se connecter sur les premiers ports COM disponibles.
        for (int i = 1; i <= 20; ++i) {
            std::string portName = "COM" + std::to_string(i);
            int result = ConnectDobot(portName.c_str(), 115200, nullptr, nullptr);
            if (result == DobotConnect_NoError) {
                DisconnectDobot(); // on ferme immédiatement
                std::cout << "Robot détecté sur " << portName << std::endl;
                return true;
            }
        }

        std::cerr << "Aucun Dobot détecté (ports COM testés jusqu’à COM20)." << std::endl;
        return false;
    }

    std::cout << "Robot détecté via SearchDobot: " << dobotNameList << std::endl;
    return true;
}

void Robot::Home()
{
    std::cout << "🏠 Retour à la position HOME..." << std::endl;
    HOMECmd homeCmd = {0};
    uint64_t queuedIndex = 0;

    int res = SetHOMECmd(&homeCmd, true, &queuedIndex);
    if (res != DobotCommunicate_NoError) {
        std::cerr << "❌ Impossible d'envoyer la commande HOME." << std::endl;
        return;
    }

    std::cout << "✅ Commande HOME envoyée." << std::endl;
    goTo(pieceCoordinates[0], 90);
}

void Robot::Play(int column)
{
    if (remainingPieces == 0) {
        std::cerr << "⚠️  Plus de pièces disponibles." << std::endl;
        return;
    }

    if (column < 0 || column > 6) {
        std::cerr << "⚠️  Colonne invalide." << std::endl;
        return;
    }

    std::cout << "🤖 Placement dans la colonne " << column << std::endl;
    grabPiece();

    goTo(columnCoordinates[column], 90);
    goTo(columnCoordinates[column]);
    openGripper();
    wait(0.5f);
    turnOffGripper();

    if (remainingPieces > 0)
        goTo(pieceCoordinates[8 - remainingPieces], 90);
    else
        goTo(pieceCoordinates[0], 90);
}

void Robot::Refill()
{
    remainingPieces = 8;
    std::cout << "♻️  Réinitialisation du nombre de pièces." << std::endl;
}

int Robot::getRemainingPieces()
{
    return remainingPieces;
}

void Robot::goTo(Pose position)
{
    PTPCmd cmd = {0};
    cmd.ptpMode = PTPJUMPXYZMode;
    cmd.x = position.x;
    cmd.y = position.y;
    cmd.z = position.z;
    cmd.r = position.r;

    uint64_t queuedCmdIndex = 0;
    int res = SetPTPCmd(&cmd, true, &queuedCmdIndex);
    if (res != DobotCommunicate_NoError)
        std::cerr << "❌ Erreur mouvement PTP." << std::endl;
}

void Robot::goTo(Pose position, float z)
{
    Pose temp = position;
    temp.z = z;
    goTo(temp);
}

void Robot::openGripper()
{
    gripper(true, false);
}

void Robot::closeGripper()
{
    gripper(true, true);
}

void Robot::grabPiece()
{
    std::cout << "🟡 Préhension d'une pièce..." << std::endl;
    openGripper();
    goTo(pieceCoordinates[8 - remainingPieces]);
    closeGripper();
    wait(0.5f);
    goTo(pieceCoordinates[8 - remainingPieces], 90);
    remainingPieces--;
}

void Robot::gripper(bool enable, bool grip)
{
    uint64_t queuedCmdIndex = 0;
    int res = SetEndEffectorGripper(enable, grip, true, &queuedCmdIndex);
    if (res != DobotCommunicate_NoError)
        std::cerr << "❌ Erreur commande du préhenseur." << std::endl;
}

void Robot::turnOffGripper()
{
    gripper(false, false);
}

void Robot::wait(float seconds)
{
    WAITCmd waitCmd = { static_cast<uint32_t>(seconds * 1000) };
    uint64_t queuedCmdIndex = 0;
    int res = SetWAITCmd(&waitCmd, true, &queuedCmdIndex);
    if (res != DobotCommunicate_NoError)
        std::cerr << "❌ Erreur commande WAIT." << std::endl;
}
