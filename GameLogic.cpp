#include "GameLogic.hpp"
#include "Negamax.hpp"      // → version SimpleAI que nous venons de créer
#include <vector>
#include <cstdlib>          // Pour rand()
#include <ctime>            // Pour srand()
#include <QRandomGenerator> // Pour génération aléatoire améliorée
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
    referenceGrid.resize(6);
    stableCandidate.resize(6);
    for (int r = 0; r < 6; r++) {
        grid[r].resize(7);
        prevGrid[r].resize(7);
        candidateGrid[r].resize(7);
        referenceGrid[r].resize(7);
        stableCandidate[r].resize(7);
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

        // Lancer la préparation dans un thread séparé pour ne pas bloquer l'UI
        std::thread([this]() {
            qDebug() << "[GameLogic] Thread de préparation démarré";

            // Connexion au robot
            qDebug() << "[GameLogic] Connexion au robot...";
            QMetaObject::invokeMethod(this, [this]() { emit robotInitializing(); }, Qt::QueuedConnection);

            if (!calib->connectToRobot()) {
                qWarning() << "[GameLogic] ERREUR : Impossible de se connecter au robot !";
                QMetaObject::invokeMethod(this, [this]() { emit connectionFailed(); }, Qt::QueuedConnection);
                preparationRunning = false;
                robotConnected = false;
                return;
            }

            // Vérifier si l'arrêt d'urgence a été activé
            if (!preparationRunning) {
                qDebug() << "[GameLogic] Arrêt d'urgence détecté après connexion, abandon de la préparation";
                return;
            }
            qDebug() << "[GameLogic] Robot connecté avec succès";

            // Remise en position initiale DIRECTEMENT via robot->Home()
            qDebug() << "[GameLogic] === REMISE EN POSITION INITIALE ===";
            qDebug() << "[GameLogic] ⚠️ APPEL UNIQUE À robot->Home() depuis GameLogic::prepareGame() thread";
            if (!robot->Home()) {
                qWarning() << "[GameLogic] ❌ ERREUR : Échec de Home() - le robot n'a pas pu retourner à sa position initiale";
                QMetaObject::invokeMethod(this, [this]() {
                    emit robotStatus("ERREUR : Échec du retour à la position initiale");
                }, Qt::QueuedConnection);
                preparationRunning = false;
                return;
            }
            qDebug() << "[GameLogic] ✅ robot->Home() TERMINÉ depuis GameLogic::prepareGame() thread";

            // Vérifier si l'arrêt d'urgence a été activé pendant le Home
            if (!preparationRunning) {
                qDebug() << "[GameLogic] Arrêt d'urgence détecté après Home(), abandon de la préparation";
                return;
            }
            qDebug() << "[GameLogic] Remise en position initiale terminée";

            // Fermer la pince pour être prêt à jouer
            qDebug() << "[GameLogic] Fermeture de la pince...";
            if (!robot->closeGripper()) {
                qWarning() << "[GameLogic] ❌ ERREUR : Échec de closeGripper()";
                QMetaObject::invokeMethod(this, [this]() {
                    emit robotStatus("ERREUR : Impossible de fermer la pince");
                }, Qt::QueuedConnection);
                preparationRunning = false;
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            if (!robot->turnOffGripper()) {
                qWarning() << "[GameLogic] ❌ ERREUR : Échec de turnOffGripper()";
                // On continue quand même car ce n'est pas critique
            }

            // Vérifier si l'arrêt d'urgence a été activé
            if (!preparationRunning) {
                qDebug() << "[GameLogic] Arrêt d'urgence détecté après fermeture pince, abandon de la préparation";
                return;
            }
            qDebug() << "[GameLogic] Pince fermée";

            // Se positionner au-dessus du réservoir gauche pour être prêt à jouer
            qDebug() << "[GameLogic] Déplacement au-dessus du réservoir gauche...";
            calib->goToLeftReservoirArea();

            // Vérifier si l'arrêt d'urgence a été activé
            if (!preparationRunning) {
                qDebug() << "[GameLogic] Arrêt d'urgence détecté après déplacement réservoir, abandon de la préparation";
                return;
            }
            qDebug() << "[GameLogic] Positionné au-dessus du réservoir gauche";

            // Marquer comme prêt
            robotConnected = true;
            preparationRunning = false;

            qDebug() << "[GameLogic] Robot prêt à jouer";
            QMetaObject::invokeMethod(this, [this]() { emit robotInitialized(); }, Qt::QueuedConnection);

            qDebug() << "[GameLogic] Thread de préparation terminé";
        }).detach();
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
    stabilityConfirmCount = 0;
    waitingForStableGrid = false;
    candidateGrid.clear();
    referenceGrid.clear();
    stableCandidate.clear();
    currentTurn = PlayerTurn;
    lastRobotColumn = -1;

    // Le thread de Home() de CalibrationLogic s'arrêtera automatiquement
    // via disconnectToRobot() plus bas

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
        if (!robot->openGripper()) {
            qWarning() << "[GameLogic] ⚠️ Échec d'ouverture de la pince lors de l'arrêt";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (!robot->closeGripper()) {
            qWarning() << "[GameLogic] ⚠️ Échec de fermeture de la pince lors de l'arrêt";
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(300));
        if (!robot->turnOffGripper()) {
            qWarning() << "[GameLogic] ⚠️ Échec de coupure du compresseur lors de l'arrêt";
        }
    }

    // Déconnecter le robot pour permettre une nouvelle connexion propre à la prochaine partie
    if (robotConnected && calib) {
        qDebug() << "[GameLogic] Déconnexion du robot...";
        calib->disconnectToRobot();
        robotConnected = false;
        qDebug() << "[GameLogic] Robot déconnecté";
    }

    qDebug() << "[GameLogic] === PARTIE ARRÊTÉE ===";
}

// =============================================================
//   EMERGENCY STOP GAME — arrêt d'urgence
// =============================================================
void GameLogic::emergencyStopGame()
{
    qDebug() << "[GameLogic] === ARRÊT D'URGENCE DE LA PARTIE ===";
    qDebug() << "[GameLogic] NOTE : Le robot a déjà été déconnecté par MainWindow::emergencyDisconnect()";

    gameRunning = false;
    negamaxRunning = false;
    preparationRunning = false;

    camera->stop();

    // Réinitialiser tous les compteurs et états
    gridConfirmCount = 0;
    stabilityConfirmCount = 0;
    waitingForStableGrid = false;
    candidateGrid.clear();
    referenceGrid.clear();
    stableCandidate.clear();
    currentTurn = PlayerTurn;
    lastRobotColumn = -1;

    // Attendre proprement la fin du thread du robot (timeout court : 2 secondes max)
    if (negamaxThreadObj) {
        if (negamaxThreadObj->isRunning()) {
            qDebug() << "[GameLogic] Attente de la fin du thread robot (timeout 2s)...";
            negamaxThreadObj->quit();

            // Timeout court car le thread devrait se terminer rapidement grâce au flag emergencyStopFlag
            if (!negamaxThreadObj->wait(2000)) {
                qWarning() << "[GameLogic] Le thread robot ne s'est pas terminé en 2s, abandon (fuite mémoire)";
                negamaxThreadObj = nullptr;  // Perdre la référence, le thread finira tout seul
            } else {
                qDebug() << "[GameLogic] Thread robot terminé proprement";
                delete negamaxThreadObj;
                negamaxThreadObj = nullptr;
            }
        } else {
            delete negamaxThreadObj;
            negamaxThreadObj = nullptr;
        }
    }

    // NE PAS déconnecter le robot ici - c'est déjà fait par MainWindow::emergencyDisconnect()
    // NE PAS toucher à la pince - le robot est déjà déconnecté
    // Juste mettre à jour les flags
    robotConnected = false;
    if (calib) {
        calib->setConnectedState(false);
    }

    qDebug() << "[GameLogic] === ARRÊT D'URGENCE TERMINÉ ===";
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

    // ===== PHASE 1 : DÉTECTION INITIALE (5 images identiques) =====
    if (!waitingForStableGrid) {
        if (candidateGrid.isEmpty() || !areGridsEqual(candidateGrid, g)) {
            // Nouvelle grille détectée, réinitialiser le compteur
            candidateGrid = g;
            gridConfirmCount = 1;
            return;
        }

        // La grille est identique à la précédente
        gridConfirmCount++;

        // Attendre GRID_CONFIRM_THRESHOLD détections identiques avant de passer à la validation de stabilité
        if (gridConfirmCount < GRID_CONFIRM_THRESHOLD)
            return;

        // Grille détectée sur 5 images, vérifier qu'elle est différente de la grille de référence
        // Si c'est la même grille, ignorer (aucun coup n'a été joué)
        if (gridReady && !referenceGrid.isEmpty() && areGridsEqual(g, referenceGrid)) {
            // La grille n'a pas changé depuis le dernier coup validé, ignorer
            gridConfirmCount = 0;  // Réinitialiser pour éviter de reboucler
            return;
        }

        // Grille détectée sur 5 images ET différente de la référence, passer à la phase de validation de stabilité
        qDebug() << "[GameLogic] Grille détectée sur 5 images (différente de la référence), validation de stabilité...";
        stableCandidate = g;
        stabilityConfirmCount = 1;
        waitingForStableGrid = true;
        gridConfirmCount = 0;
        return;
    }

    // ===== PHASE 2 : VALIDATION DE STABILITÉ (15 images) =====
    if (waitingForStableGrid) {
        if (!areGridsEqual(stableCandidate, g)) {
            // La grille a changé, retour à la phase de détection
            qDebug() << "[GameLogic] Grille instable, retour à la phase de détection";
            candidateGrid = g;
            gridConfirmCount = 1;
            stabilityConfirmCount = 0;
            waitingForStableGrid = false;
            return;
        }

        stabilityConfirmCount++;

        // Attendre STABILITY_THRESHOLD images identiques pour valider la stabilité
        if (stabilityConfirmCount < STABILITY_THRESHOLD)
            return;

        // Pion stable validé sur 15 images !
        qDebug() << "[GameLogic] Pion stable validé sur 15 images";
        waitingForStableGrid = false;
        stabilityConfirmCount = 0;

        // ===== PHASE 3 : ANTI-CHEAT (comparaison avec grille de référence) =====
        // Si c'est la première grille (début de partie), initialiser la référence
        if (referenceGrid.isEmpty() || !gridReady) {
            qDebug() << "[GameLogic] Initialisation de la grille de référence";
            referenceGrid = g;
            grid = g;
            gridReady = true;
            return;
        }

        // Comparer avec la grille de référence du tour précédent
        int newPiecesCount = 0;
        int newPlayerPieces = 0;
        int newRobotPieces = 0;

        for (int r = 0; r < 6; r++) {
            for (int c = 0; c < 7; c++) {
                // Un pion a disparu (case remplie -> case vide)
                if (referenceGrid[r][c] != 0 && g[r][c] == 0) {
                    camera->stop();
                    emit cheatDetected("TRICHE DÉTECTÉE\nUn pion a été retiré de la grille !");
                    gameRunning = false;
                    return;
                }
                // Un pion a changé de couleur
                else if (referenceGrid[r][c] != 0 && g[r][c] != 0 && referenceGrid[r][c] != g[r][c]) {
                    camera->stop();
                    emit cheatDetected("TRICHE DÉTECTÉE\nUn pion a changé de couleur !");
                    gameRunning = false;
                    return;
                }
                // Un nouveau pion est apparu (case vide -> case remplie)
                else if (referenceGrid[r][c] == 0 && g[r][c] != 0) {
                    newPiecesCount++;
                    if (g[r][c] == playerColor)
                        newPlayerPieces++;
                    else if (g[r][c] == robotColor)
                        newRobotPieces++;
                }
            }
        }

        // Vérifier qu'exactement un pion a été ajouté
        if (newPiecesCount != 1) {
            camera->stop();
            if (newPiecesCount == 0) {
                emit cheatDetected("ERREUR\nAucun pion détecté alors qu'un coup devrait avoir été joué !");
            } else {
                emit cheatDetected("TRICHE DÉTECTÉE\nPlusieurs pions ont été ajoutés en même temps !");
            }
            gameRunning = false;
            return;
        }

        // Vérifier que c'est la bonne couleur selon le tour actuel
        if (currentTurn == PlayerTurn && newPlayerPieces != 1) {
            camera->stop();
            emit cheatDetected("TRICHE DÉTECTÉE\nMauvaise couleur de pion !\nVous devez jouer avec les pions " +
                             QString(playerColor == 1 ? "rouges" : "jaunes"));
            gameRunning = false;
            return;
        }

        if (currentTurn == RobotTurn && newRobotPieces != 1) {
            camera->stop();
            emit cheatDetected("ERREUR SYSTÈME\nLe robot devrait avoir joué un pion " +
                             QString(robotColor == 1 ? "rouge" : "jaune"));
            gameRunning = false;
            return;
        }

        // ===== PHASE 4 : ENREGISTREMENT ET CONTINUATION =====
        qDebug() << "[GameLogic] Anti-cheat OK, enregistrement de la nouvelle grille de référence";
        prevGrid = referenceGrid;
        referenceGrid = g;
        grid = g;

        // Vérifier la victoire et l'égalité
        if (checkWin(playerColor)) {
            camera->stop();
            QString diffString;
            switch (sm->getDifficulty()) {
            case StateMachine::Easy: diffString = "Facile"; break;
            case StateMachine::Medium: diffString = "Normal"; break;
            case StateMachine::Hard: diffString = "Difficile"; break;
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
            }
            emit gameResult("Égalité", diffString, elapsedSeconds);
            gameRunning = false;
            return;
        }

        // Passage au tour suivant
        if (currentTurn == PlayerTurn) {
            int playedCol = -1;
            if (detectPlayerMove(prevGrid, grid, playedCol)) {
                qDebug() << "[GameLogic] Coup joueur validé dans colonne" << playedCol;
                currentTurn = RobotTurn;
                emit turnRobot();
                launchRobotTurn();
            }
        } else if (currentTurn == RobotTurn) {
            qDebug() << "[GameLogic] Coup robot validé, passage au tour du joueur";
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
    case StateMachine::Hard: depth = 6; break;
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
        int bestMove = -1;

        // Fonction helper pour vérifier si une colonne est pleine (6 lignes)
        auto isColumnFull = [this](int col) -> bool {
            if (col < 0 || col >= 7) return true;  // Colonne invalide = pleine
            // Une colonne est pleine si toutes les 6 lignes sont remplies (non-zéro)
            for (int row = 0; row < 6; row++) {
                if (grid[row][col] == 0) return false;  // Case vide trouvée
            }
            return true;  // Toutes les cases sont remplies
        };

        // Obtenir la liste des colonnes valides (non pleines)
        QVector<int> validColumns;
        for (int col = 0; col < 7; col++) {
            if (!isColumnFull(col)) {
                validColumns.append(col);
            }
        }

        if (validColumns.isEmpty()) {
            qWarning() << "[GameLogic] ERREUR : Aucune colonne disponible (grille pleine) !";
            emit endOfGame("Match nul !\nLa grille est pleine", 0);
            negamaxRunning = false;
            return;
        }

        if (sm->getDifficulty() == StateMachine::Easy) {
            // Mode facile : choix complètement aléatoire parmi les colonnes valides
            qDebug() << "[GameLogic] Mode facile : choix aléatoire parmi" << validColumns.size() << "colonnes valides";
            qDebug() << "[GameLogic] Colonnes disponibles :" << validColumns;
            emit robotStatus("Il réfléchit");

            // Utiliser QRandomGenerator pour un vrai aléatoire (meilleur que rand())
            int randomIndex = QRandomGenerator::global()->bounded(validColumns.size());
            bestMove = validColumns[randomIndex];
            qDebug() << "[GameLogic] *** MODE FACILE *** Colonne aléatoire choisie :" << bestMove << "(index" << randomIndex << "sur" << validColumns.size() << "colonnes)";
        } else {
            // Modes Normal et Difficile : utiliser Negamax
            qDebug() << "[GameLogic] IA réfléchit avec Negamax...";
            emit robotStatus("Il réfléchit");
            QVector<QVector<int>> current = grid;
            bestMove = SimpleAI::getBestMove(current, depth, robotColor);
            qDebug() << "[GameLogic] Negamax a choisi la colonne" << bestMove;

            // Vérifier que Negamax n'a pas choisi une colonne pleine (sécurité)
            if (isColumnFull(bestMove)) {
                qWarning() << "[GameLogic] ATTENTION : Negamax a choisi une colonne pleine (" << bestMove << "), fallback sur colonne aléatoire";
                int randomIndex = QRandomGenerator::global()->bounded(validColumns.size());
                bestMove = validColumns[randomIndex];
                qDebug() << "[GameLogic] Colonne de fallback :" << bestMove;
            }
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
        if (!calib->pickPiece(pickPos)) {
            qWarning() << "[GameLogic] ❌ ERREUR : Échec de pickPiece() - le robot n'a pas pu récupérer le pion";
            emit robotStatus("ERREUR : Impossible de récupérer le pion");
            negamaxRunning = false;
            return;
        }
        qDebug() << "[GameLogic] Pion récupéré";

        // Vérifier après chaque opération robot
        if (!negamaxRunning || !gameRunning) {
            qDebug() << "[GameLogic] Arrêt demandé après récupération";
            negamaxRunning = false;
            return;
        }

        qDebug() << "[GameLogic] Place le pion dans la colonne" << bestMove;
        emit robotStatus(QString("Place le pion dans la colonne %1").arg(bestMove+1));
        // Lâcher le pion dans la colonne choisie (fonction de haut niveau)
        if (!calib->dropPiece(bestMove)) {
            qWarning() << "[GameLogic] ❌ ERREUR : Échec de dropPiece() - le robot n'a pas pu placer le pion";
            emit robotStatus("ERREUR : Impossible de placer le pion");
            negamaxRunning = false;
            return;
        }
        qDebug() << "[GameLogic] Pion placé";

        // Vérifier après chaque opération robot
        if (!negamaxRunning || !gameRunning) {
            qDebug() << "[GameLogic] Arrêt demandé après placement";
            negamaxRunning = false;
            return;
        }

        // Fin du tour robot - marquer negamaxRunning à false AVANT le repositionnement
        // pour permettre au joueur de jouer rapidement
        qDebug() << "[GameLogic] Robot a terminé son action";
        negamaxRunning = false;
        lastRobotColumn = bestMove;

        // Repositionner le robot au-dessus du prochain réservoir (asynchrone)
        // TOUJOURS repositionner, même si les réservoirs sont vides, pour éviter l'ombre sur la grille
        // Si le joueur joue pendant ce repositionnement, le robot pourra jouer immédiatement
        // car les commandes Dobot sont en queue et gérées dans l'ordre
        bool reservoirsEmpty = (leftReservoirPieces <= 0 && rightReservoirPieces <= 0);
        bool isLeftReservoir = true;  // Par défaut, aller au réservoir gauche

        if (leftReservoirPieces > 0) {
            isLeftReservoir = true;
        } else if (rightReservoirPieces > 0) {
            isLeftReservoir = false;
        }
        // Sinon, on garde isLeftReservoir = true (réservoir gauche par défaut si vides)

        qDebug() << "[GameLogic] Lancement du repositionnement au réservoir" << (isLeftReservoir ? "gauche" : "droit");
        emit robotStatus(QString("Se repositionne au réservoir %1").arg(isLeftReservoir ? "gauche" : "droit"));

        // Lancer le repositionnement dans un thread séparé (asynchrone)
        QThread* repositionThread = QThread::create([this, isLeftReservoir, reservoirsEmpty]() {
            qDebug() << "[GameLogic] Thread de repositionnement démarré";

            if (isLeftReservoir) {
                qDebug() << "[GameLogic] Déplacement vers réservoir GAUCHE";
                calib->goToLeftReservoirArea();
            } else {
                qDebug() << "[GameLogic] Déplacement vers réservoir DROIT";
                calib->goToRightReservoirArea();
            }

            qDebug() << "[GameLogic] Repositionnement terminé";

            // Si les réservoirs sont vides, émettre le signal APRÈS le repositionnement
            if (reservoirsEmpty) {
                qDebug() << "[GameLogic] Réservoirs vides ! Demande de remplissage...";
                emit reservoirEmpty();
            }
        });

        connect(repositionThread, &QThread::finished, repositionThread, &QThread::deleteLater);
        repositionThread->start();

        // currentTurn reste à RobotTurn - onGridUpdated() va le changer à PlayerTurn après validation
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
