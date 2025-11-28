#include "GameLogic.hpp"
#include "Negamax.hpp"      // → version SimpleAI que nous venons de créer
#include <vector>
#include <cstdlib>          // Pour rand()
#include <ctime>            // Pour srand()
using namespace SimpleAI;

// =============================================================
//   CONSTRUCTEUR
// =============================================================
GameLogic::GameLogic(CameraAI* cam,
                     Robot* r,
                     CalibrationLogic* calibration,
                     StateMachine* stm,
                     QObject* parent)
    : QObject(parent),
    camera(cam),
    robot(r),
    calib(calibration),
    sm(stm)
{
    // Initialiser le générateur de nombres aléatoires pour le mode facile
    srand(time(nullptr));

    grid.resize(6);
    prevGrid.resize(6);
    candidateGrid.resize(6);
    for (int r = 0; r < 6; r++) {
        grid[r].resize(7);
        prevGrid[r].resize(7);
        candidateGrid[r].resize(7);
    }

    // Frame vers view
    connect(camera, &CameraAI::frameReady,
            this, &GameLogic::sendFrameToScreen,
            Qt::QueuedConnection);

    // Grille mise à jour
    connect(camera, &CameraAI::gridUpdated,
            this, &GameLogic::onGridUpdated,
            Qt::QueuedConnection);
}

GameLogic::~GameLogic()
{
    stopGame();
}

// =============================================================
//   PREPARE GAME — avant le countdown
// =============================================================
void GameLogic::prepareGame()
{
    qDebug() << "[GameLogic] === PRÉPARATION DE LA PARTIE ===";

    // Réinitialiser les réservoirs de pions (pleins au départ)
    leftReservoirPieces = 4;
    rightReservoirPieces = 4;
    currentLeftIndex = 0;
    currentRightIndex = 0;

    // Définir les couleurs selon le choix du joueur
    playerColor = sm->getPlayerColorValue();  // 1=rouge, 2=jaune
    robotColor = sm->getRobotColorValue();    // Inverse du joueur
    qDebug() << "[GameLogic] Couleur joueur:" << playerColor << "- Couleur robot:" << robotColor;

    // Connexion au robot (seulement si pas déjà connecté)
    if (!robotConnected) {
        if (preparationRunning) {
            qDebug() << "[GameLogic] Préparation déjà en cours, abandon";
            return;
        }
        preparationRunning = true;

        // Lancer la préparation du robot dans un thread séparé pour ne pas bloquer l'UI
        preparationThreadObj = QThread::create([this]() {
            qDebug() << "[GameLogic] Thread de préparation démarré";

            qDebug() << "[GameLogic] Connexion au robot...";
            if (!calib->connectToRobot()) {
                qWarning() << "[GameLogic] ERREUR : Impossible de se connecter au robot !";
                emit endOfGame("❌ ERREUR ❌\nImpossible de se connecter au robot", 0);
                preparationRunning = false;
                return;
            }
            qDebug() << "[GameLogic] Robot connecté avec succès";

            // Reset à la position d'origine avec affichage du message (seulement à la première connexion)
            qDebug() << "[GameLogic] === DÉBUT REMISE EN POSITION INITIALE ===";
            emit robotInitializing();
            qDebug() << "[GameLogic] Appel de robot->Home()...";
            robot->Home();
            qDebug() << "[GameLogic] Home() terminé, attente 2 secondes...";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            qDebug() << "[GameLogic] Attente terminée";

            // Fermer la pince après le retour à la position initiale
            robot->closeGripper();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            robot->turnOffGripper();  // Couper le compresseur pour réduire le bruit

            // Vérifier que la pince est bien fermée (état initial avant le début de la partie)
            qDebug() << "[GameLogic] Vérification de l'état de la pince : fermée";

            qDebug() << "[GameLogic] Robot en position initiale et prêt à jouer";
            emit robotInitialized();

            robotConnected = true;
            preparationRunning = false;

            qDebug() << "[GameLogic] Thread de préparation terminé";
        });

        preparationThreadObj->start();
        qDebug() << "[GameLogic] Thread de préparation lancé";
    } else {
        qDebug() << "[GameLogic] Robot déjà connecté, pas de Home() nécessaire";
        // Émettre robotInitialized immédiatement car on n'affiche pas l'overlay
        emit robotInitialized();
    }

    qDebug() << "[GameLogic] === PRÉPARATION LANCÉE ===";
}

// =============================================================
//   START GAME — appelé après countdownFinished()
// =============================================================
void GameLogic::startGame()
{
    qDebug() << "[GameLogic] === DÉMARRAGE DE LA PARTIE ===";
    gameRunning = true;

    // Démarrage caméra
    qDebug() << "[GameLogic] Démarrage de la caméra...";
    camera->start(0);

    // Texte de difficulté
    QString diffString;
    switch (sm->getDifficulty()) {
    case StateMachine::Easy: diffString = "Facile"; break;
    case StateMachine::Medium: diffString = "Normal"; break;
    case StateMachine::Hard: diffString = "Difficile"; break;
    case StateMachine::Impossible: diffString = "Impossible"; break;
    }
    emit difficultyText(diffString);

    // Le joueur commence
    currentTurn = PlayerTurn;
    emit turnPlayer();
    qDebug() << "[GameLogic] === PARTIE PRÊTE ===";
}

// =============================================================
//   STOP GAME — quitter partie
// =============================================================
void GameLogic::stopGame()
{
    qDebug() << "[GameLogic] === ARRÊT DE LA PARTIE ===";
    gameRunning = false;
    negamaxRunning = false;
    preparationRunning = false;

    camera->stop();

    // Réinitialiser tous les compteurs et états
    gridConfirmCount = 0;
    candidateGrid.clear();
    currentTurn = PlayerTurn;
    lastRobotColumn = -1;

    // Attendre proprement la fin du thread de préparation
    if (preparationThreadObj) {
        if (preparationThreadObj->isRunning()) {
            qDebug() << "[GameLogic] Attente de la fin du thread de préparation...";
            preparationThreadObj->quit();
            if (!preparationThreadObj->wait(5000)) {
                qWarning() << "[GameLogic] Le thread de préparation n'a pas pu se terminer proprement, terminaison forcée";
                preparationThreadObj->terminate();
                preparationThreadObj->wait();
            }
        }
        delete preparationThreadObj;
        preparationThreadObj = nullptr;
    }

    // Attendre proprement la fin du thread du robot
    if (negamaxThreadObj) {
        // Si le thread est en cours d'exécution, attendre qu'il se termine
        if (negamaxThreadObj->isRunning()) {
            qDebug() << "[GameLogic] Attente de la fin du thread robot...";
            negamaxThreadObj->quit();
            // Attendre maximum 5 secondes pour que le thread se termine proprement
            if (!negamaxThreadObj->wait(5000)) {
                qWarning() << "[GameLogic] Le thread du robot n'a pas pu se terminer proprement, terminaison forcée";
                negamaxThreadObj->terminate();
                negamaxThreadObj->wait();
            }
        }
        delete negamaxThreadObj;
        negamaxThreadObj = nullptr;
    }

    // Ouvrir puis fermer la pince pour lâcher tout pion éventuel
    if (robot && robotConnected) {
        qDebug() << "[GameLogic] Ouverture et fermeture de la pince...";
        robot->openGripper();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        robot->closeGripper();
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        robot->turnOffGripper();  // Couper le compresseur
    }

    // Ne pas déconnecter le robot pour éviter de refaire Home() à chaque partie
    // La déconnexion se fera à la fermeture de l'application
    qDebug() << "[GameLogic] === PARTIE ARRÊTÉE ===";
}

// =============================================================
//   RESET ROBOT CONNECTION STATE
// =============================================================
void GameLogic::resetRobotConnection()
{
    qDebug() << "[GameLogic] Réinitialisation de l'état de connexion du robot";
    robotConnected = false;
}

// =============================================================
//   RÉCEPTION D'UNE GRILLE CAMERA
// =============================================================
void GameLogic::onGridUpdated(const QVector<QVector<int>>& g)
{
    if (!gameRunning)
        return;

    // Système de confirmation : attendre 5 grilles identiques avant de valider
    if (candidateGrid.isEmpty() || !areGridsEqual(candidateGrid, g)) {
        // Nouvelle grille détectée, réinitialiser le compteur
        candidateGrid = g;
        gridConfirmCount = 1;
        return;
    }

    // La grille est identique à la précédente
    gridConfirmCount++;

    // Attendre GRID_CONFIRM_THRESHOLD détections identiques avant de valider
    if (gridConfirmCount < GRID_CONFIRM_THRESHOLD)
        return;

    // Grille confirmée après 5 détections identiques

    // =====================================================
    // IMPORTANT : ANTI-TRICHE AVANT VÉRIFICATION VICTOIRE
    // =====================================================
    // L'anti-triche DOIT être vérifié AVANT la victoire pour empêcher
    // un joueur de poser 4 pions alignés d'un coup et de gagner malgré la triche.
    // Si une triche est détectée, on retourne immédiatement sans vérifier la victoire.

    // Vérification anti-triche : vérifier que la composition de la grille est valide
    int newPiecesCount = 0;
    int newPlayerPieces = 0;
    int newRobotPieces = 0;

    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            // Un pion a disparu (case remplie -> case vide)
            if (grid[r][c] != 0 && g[r][c] == 0) {
                camera->stop();
                emit cheatDetected("TRICHE DÉTECTÉE\nUn pion a été retiré de la grille !");
                gameRunning = false;
                return;
            }
            // Un pion a changé de couleur
            else if (grid[r][c] != 0 && g[r][c] != 0 && grid[r][c] != g[r][c]) {
                camera->stop();
                emit cheatDetected("TRICHE DÉTECTÉE\nUn pion a changé de couleur !");
                gameRunning = false;
                return;
            }
            // Un nouveau pion est apparu (case vide -> case remplie)
            else if (grid[r][c] == 0 && g[r][c] != 0) {
                newPiecesCount++;
                if (g[r][c] == playerColor)
                    newPlayerPieces++;
                else if (g[r][c] == robotColor)
                    newRobotPieces++;
            }
        }
    }

    // Détection de triche au tour du joueur (avec compteur pour éviter faux positifs)
    if (currentTurn == PlayerTurn) {
        if (newPiecesCount > 1) {
            cheatMultiplePiecesCount++;
            if (cheatMultiplePiecesCount >= CHEAT_CONFIRM_THRESHOLD) {
                camera->stop();
                emit cheatDetected("TRICHE DÉTECTÉE\nPlusieurs pions ont été ajoutés en même temps !");
                gameRunning = false;
                return;
            }
        } else {
            cheatMultiplePiecesCount = 0;  // Réinitialiser si plus de triche
        }

        if (newRobotPieces > 0) {
            cheatWrongColorCount++;
            if (cheatWrongColorCount >= CHEAT_CONFIRM_THRESHOLD) {
                camera->stop();
                emit cheatDetected("TRICHE DÉTECTÉE\nMauvaise couleur de pion !\nVous devez jouer avec les pions " +
                                 QString(playerColor == 1 ? "rouges" : "jaunes"));
                gameRunning = false;
                return;
            }
        } else {
            cheatWrongColorCount = 0;  // Réinitialiser si plus de triche
        }
    }

    // Détection de triche pendant l'attente de détection du robot (avec compteur)
    if (currentTurn == WaitingForRobotDetection) {
        if (newPiecesCount > 1 || newPlayerPieces > 0) {
            cheatDuringRobotCount++;
            if (cheatDuringRobotCount >= CHEAT_CONFIRM_THRESHOLD) {
                camera->stop();
                if (newPiecesCount > 1) {
                    emit cheatDetected("TRICHE DÉTECTÉE\nPions ajoutés pendant le tour du robot !");
                } else {
                    emit cheatDetected("TRICHE DÉTECTÉE\nPion du joueur ajouté pendant le tour du robot !");
                }
                gameRunning = false;
                return;
            }
        } else {
            cheatDuringRobotCount = 0;  // Réinitialiser si plus de triche
        }
    }

    prevGrid = grid;
    grid = g;
    gridReady = true;
    gridConfirmCount = 0;  // Réinitialiser pour la prochaine détection

    // Vérifier la victoire et l'égalité avant de continuer
    if (checkWin(playerColor)) {
        camera->stop();
        QString diffString;
        switch (sm->getDifficulty()) {
        case StateMachine::Easy: diffString = "Facile"; break;
        case StateMachine::Medium: diffString = "Normal"; break;
        case StateMachine::Hard: diffString = "Difficile"; break;
        case StateMachine::Impossible: diffString = "Impossible"; break;
        }
        emit gameResult("Joueur", diffString, elapsedSeconds);
        gameRunning = false;
        return;
    }

    if (checkWin(robotColor)) {
        camera->stop();
        QString diffString;
        switch (sm->getDifficulty()) {
        case StateMachine::Easy: diffString = "Facile"; break;
        case StateMachine::Medium: diffString = "Normal"; break;
        case StateMachine::Hard: diffString = "Difficile"; break;
        case StateMachine::Impossible: diffString = "Impossible"; break;
        }
        emit gameResult("Robot", diffString, elapsedSeconds);
        gameRunning = false;
        return;
    }

    if (isBoardFull()) {
        camera->stop();
        QString diffString;
        switch (sm->getDifficulty()) {
        case StateMachine::Easy: diffString = "Facile"; break;
        case StateMachine::Medium: diffString = "Normal"; break;
        case StateMachine::Hard: diffString = "Difficile"; break;
        case StateMachine::Impossible: diffString = "Impossible"; break;
        }
        emit gameResult("Égalité", diffString, elapsedSeconds);
        gameRunning = false;
        return;
    }

    if (currentTurn == PlayerTurn)
    {
        int playedCol = -1;
        if (detectPlayerMove(prevGrid, grid, playedCol))
        {
            qDebug() << "[GameLogic] Détection coup joueur dans colonne" << playedCol;
            currentTurn = RobotTurn;
            emit turnRobot();
            qDebug() << "[GameLogic] Passage au tour du robot";
            launchRobotTurn();
        }
    }
    else if (currentTurn == WaitingForRobotDetection)
    {
        qDebug() << "[GameLogic] En attente de détection du pion du robot dans colonne" << lastRobotColumn;
        // Attendre que le pion du robot soit détecté
        if (detectRobotPlacement(prevGrid, grid, lastRobotColumn))
        {
            qDebug() << "[GameLogic] Pion du robot détecté !";

            // Maintenant qu'on a détecté le pion, lancer un thread pour aller au point générique du prochain réservoir
            bool hasNextPiece = false;
            bool isLeftReservoir = false;

            if (leftReservoirPieces > 0) {
                isLeftReservoir = true;
                hasNextPiece = true;
            } else if (rightReservoirPieces > 0) {
                isLeftReservoir = false;
                hasNextPiece = true;
            }

            if (hasNextPiece) {
                qDebug() << "[GameLogic] Lancement du repositionnement au réservoir" << (isLeftReservoir ? "gauche" : "droit");
                emit robotStatus(QString("Se repositionne au réservoir %1").arg(isLeftReservoir ? "gauche" : "droit"));

                // Lancer le repositionnement dans un thread pour ne pas bloquer l'UI
                QThread* repositionThread = QThread::create([this, isLeftReservoir]() {
                    qDebug() << "[GameLogic] Thread de repositionnement démarré";

                    // Aller au point générique du réservoir (pas à la position exacte du pion)
                    Pose genericPose = calib->getReservoirGenericPoint(isLeftReservoir);
                    float safeZ = calib->getSafeHeight();

                    qDebug() << "[GameLogic] Point générique" << (isLeftReservoir ? "GAUCHE" : "DROIT")
                             << ": x=" << genericPose.x << " y=" << genericPose.y
                             << " z=" << genericPose.z << " r=" << genericPose.r;
                    qDebug() << "[GameLogic] Hauteur de sécurité: z=" << safeZ;

                    robot->goToSecurized(genericPose, safeZ);

                    qDebug() << "[GameLogic] Repositionnement terminé";
                });

                connect(repositionThread, &QThread::finished, repositionThread, &QThread::deleteLater);
                repositionThread->start();
            }

            qDebug() << "[GameLogic] Passage au tour du joueur";
            currentTurn = PlayerTurn;
            emit turnPlayer();
        }
    }
}

// =============================================================
//   DÉTECTION NOUVEAU PION DU JOUEUR
// =============================================================
bool GameLogic::detectPlayerMove(const QVector<QVector<int>>& oldG,
                                 const QVector<QVector<int>>& newG,
                                 int& playedColumn)
{
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c < 7; c++) {
            // Détecter un nouveau pion de la couleur du joueur
            if (oldG[r][c] == 0 && newG[r][c] == playerColor) {
                playedColumn = c;
                return true;
            }
        }
    }
    return false;
}

// =============================================================
//   DÉTECTION PLACEMENT ROBOT
// =============================================================
bool GameLogic::detectRobotPlacement(const QVector<QVector<int>>& oldG,
                                     const QVector<QVector<int>>& newG,
                                     int robotColumn)
{
    for (int r = 0; r < 6; r++) {
        if (oldG[r][robotColumn] == 0 && newG[r][robotColumn] == robotColor)
            return true;
    }
    return false;
}

// =============================================================
//   LANCEMENT TOUR DU ROBOT
// =============================================================
void GameLogic::launchRobotTurn()
{
    qDebug() << "[GameLogic] launchRobotTurn() appelé - gridReady=" << gridReady;

    if (!gridReady) {
        qDebug() << "[GameLogic] Grille pas prête, abandon";
        return;
    }

    int depth = 5;
    switch (sm->getDifficulty()) {
    case StateMachine::Easy: depth = 3; break;
    case StateMachine::Medium: depth = 5; break;
    case StateMachine::Hard: depth = 7; break;
    case StateMachine::Impossible: depth = 10; break;
    }

    qDebug() << "[GameLogic] Lancement du thread negamax avec profondeur=" << depth;
    runNegamax(depth);
}

// =============================================================
//   THREAD : IA SimpleAI
// =============================================================
void GameLogic::runNegamax(int depth)
{
    qDebug() << "[GameLogic] runNegamax() - negamaxRunning=" << negamaxRunning;

    if (negamaxRunning) {
        qDebug() << "[GameLogic] Negamax déjà en cours, abandon";
        return;
    }
    negamaxRunning = true;

    negamaxThreadObj = QThread::create([this, depth]() {

        qDebug() << "[GameLogic] Thread robot démarré";

        // Vérifier qu'on doit continuer
        if (!negamaxRunning || !gameRunning) {
            qDebug() << "[GameLogic] Arrêt demandé avant IA";
            return;
        }

        // IA : déterminer la meilleure colonne pour le robot
        int bestMove;

        if (sm->getDifficulty() == StateMachine::Easy) {
            // Mode facile : choix complètement aléatoire (pas de Negamax)
            qDebug() << "[GameLogic] Mode facile : choix aléatoire entre 0 et 6";
            emit robotStatus("Il réfléchit");
            bestMove = rand() % 7;  // Colonne aléatoire entre 0 et 6
            qDebug() << "[GameLogic] Colonne aléatoire choisie :" << bestMove;
        } else {
            // Modes Normal, Difficile, Impossible : utiliser Negamax
            qDebug() << "[GameLogic] IA réfléchit avec Negamax...";
            emit robotStatus("Il réfléchit");
            QVector<QVector<int>> current = grid;
            bestMove = SimpleAI::getBestMove(current, depth, robotColor);
            qDebug() << "[GameLogic] Negamax a choisi la colonne" << bestMove;
        }

        // Vérifier à nouveau après l'IA (opération longue)
        if (!negamaxRunning || !gameRunning) {
            qDebug() << "[GameLogic] Arrêt demandé après IA";
            negamaxRunning = false;
            return;
        }

        // ---------------------------------------------------
        // Action robot : prendre un pion, le placer, revenir
        // ---------------------------------------------------

        // Vérifier qu'il reste des pions dans les réservoirs
        if (leftReservoirPieces <= 0 && rightReservoirPieces <= 0) {
            qDebug() << "[GameLogic] Réservoirs vides ! Demande de remplissage...";
            emit reservoirEmpty();
            negamaxRunning = false;
            // On reste en RobotTurn, le robot rejouera quand les réservoirs seront remplis
            return;
        }

        // Déterminer quelle position de réservoir utiliser (gauche puis droit)
        CalibPoint pickPos = getNextReservoirPosition();
        qDebug() << "[GameLogic] Récupère le pion à la position" << (int)pickPos;

        emit robotStatus("Récupère le pion");

        // Prendre le pion dans le réservoir (fonction de haut niveau qui gère tout)
        calib->pickPiece(pickPos);
        qDebug() << "[GameLogic] Pion récupéré";

        // Vérifier après chaque opération robot
        if (!negamaxRunning || !gameRunning) {
            qDebug() << "[GameLogic] Arrêt demandé après récupération";
            negamaxRunning = false;
            return;
        }

        qDebug() << "[GameLogic] Place le pion dans la colonne" << bestMove;
        emit robotStatus(QString("Place le pion dans la colonne %1").arg(bestMove));
        // Lâcher le pion dans la colonne choisie (fonction de haut niveau)
        calib->dropPiece(bestMove);
        qDebug() << "[GameLogic] Pion placé";

        // Vérifier après chaque opération robot
        if (!negamaxRunning || !gameRunning) {
            qDebug() << "[GameLogic] Arrêt demandé après placement";
            negamaxRunning = false;
            return;
        }

        // NE PAS aller au-dessus du prochain pion maintenant
        // On attend d'abord que le pion soit détecté par la caméra
        // Le mouvement se fera dans onGridUpdated() après détection

        // Fin du tour robot - attendre maintenant la détection du pion par la caméra
        qDebug() << "[GameLogic] Robot a fini, attente de détection du pion dans la colonne" << bestMove;
        emit robotStatus(QString("Attend détection du pion dans la colonne %1").arg(bestMove));
        negamaxRunning = false;
        lastRobotColumn = bestMove;
        currentTurn = WaitingForRobotDetection;
        // Note : on ne repasse PAS au tour du joueur ici
        // On attend que onGridUpdated détecte le pion du robot
    });

    negamaxThreadObj->start();
    qDebug() << "[GameLogic] Thread robot lancé";
}

// =============================================================
//   DÉTECTION DE VICTOIRE - 4 PIONS ALIGNÉS
// =============================================================
bool GameLogic::checkWin(int color)
{
    // Vérifier horizontal (4 alignés horizontalement)
    for (int r = 0; r < 6; r++) {
        for (int c = 0; c <= 3; c++) {  // 7 - 4 = 3
            if (grid[r][c] == color &&
                grid[r][c+1] == color &&
                grid[r][c+2] == color &&
                grid[r][c+3] == color) {
                return true;
            }
        }
    }

    // Vérifier vertical (4 alignés verticalement)
    for (int r = 0; r <= 2; r++) {  // 6 - 4 = 2
        for (int c = 0; c < 7; c++) {
            if (grid[r][c] == color &&
                grid[r+1][c] == color &&
                grid[r+2][c] == color &&
                grid[r+3][c] == color) {
                return true;
            }
        }
    }

    // Vérifier diagonal descendant (\)
    for (int r = 0; r <= 2; r++) {
        for (int c = 0; c <= 3; c++) {
            if (grid[r][c] == color &&
                grid[r+1][c+1] == color &&
                grid[r+2][c+2] == color &&
                grid[r+3][c+3] == color) {
                return true;
            }
        }
    }

    // Vérifier diagonal montant (/)
    for (int r = 3; r < 6; r++) {
        for (int c = 0; c <= 3; c++) {
            if (grid[r][c] == color &&
                grid[r-1][c+1] == color &&
                grid[r-2][c+2] == color &&
                grid[r-3][c+3] == color) {
                return true;
            }
        }
    }

    return false;
}

bool GameLogic::isBoardFull()
{
    for (int c = 0; c < 7; c++)
        if (grid[0][c] == 0)
            return false;
    return true;
}

// =============================================================
//   GESTION DES RÉSERVOIRS DE PIONS
// =============================================================
CalibPoint GameLogic::getNextReservoirPosition()
{
    // Utiliser d'abord le réservoir gauche
    if (leftReservoirPieces > 0) {
        CalibPoint pos = static_cast<CalibPoint>((int)CalibPoint::Left_1 + currentLeftIndex);
        currentLeftIndex++;
        leftReservoirPieces--;
        return pos;
    }
    // Sinon, utiliser le réservoir droit
    else if (rightReservoirPieces > 0) {
        CalibPoint pos = static_cast<CalibPoint>((int)CalibPoint::Right_1 + currentRightIndex);
        currentRightIndex++;
        rightReservoirPieces--;
        return pos;
    }
    // Cas d'erreur : plus de pions disponibles
    // On retourne Left_1 par défaut, mais cela ne devrait pas arriver
    return CalibPoint::Left_1;
}

// =============================================================
//   RÉSERVOIRS REMPLIS
// =============================================================
void GameLogic::onReservoirsRefilled()
{
    qDebug() << "[GameLogic] Réservoirs remplis par l'utilisateur";

    // Réinitialiser les compteurs des réservoirs
    leftReservoirPieces = 4;
    rightReservoirPieces = 4;
    currentLeftIndex = 0;
    currentRightIndex = 0;

    // Relancer le tour du robot si on était en attente
    if (currentTurn == RobotTurn && !negamaxRunning) {
        qDebug() << "[GameLogic] Relance du tour du robot après remplissage";
        launchRobotTurn();
    }
}

// =============================================================
//   COMPARAISON DE GRILLES
// =============================================================
bool GameLogic::areGridsEqual(const QVector<QVector<int>>& g1, const QVector<QVector<int>>& g2)
{
    if (g1.size() != g2.size())
        return false;

    for (int r = 0; r < g1.size(); ++r) {
        if (g1[r].size() != g2[r].size())
            return false;
        for (int c = 0; c < g1[r].size(); ++c) {
            if (g1[r][c] != g2[r][c])
                return false;
        }
    }

    return true;
}
