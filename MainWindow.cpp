#include "MainWindow.hpp"
#include <QCloseEvent>
#include <QDebug>
#include <QTimer>
#include <thread>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Puissance IV Robotisé");
    setMinimumSize(1280, 720);

    // === STACK PRINCIPAL ===
    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    // === OBJETS MOTEUR ===
    robot     = new Robot(this);
    cameraAI  = new CameraAI();  // Pas de parent pour permettre moveToThread()

    // === ÉCRANS UI ===
    introScreen       = new IntroScreen(this);
    checkScreen       = new CheckDevicesScreen(this);
    mainMenu          = new MainMenu(this);

    // ❗ Correction : on passe robot, pas "this"
    calibrationScreen = new CalibrationScreen(robot, this);

    // Écran de test de calibration (nécessite robot et CalibrationLogic)
    calibrationTestScreen = new CalibrationTestScreen(robot, calibrationScreen->getCalibrationLogic(), this);

    explanationScreen = new ExplanationScreen(this);
    gameScreen        = new GameScreen(this);

    // === AJOUT AU STACK ===
    stack->addWidget(introScreen);
    stack->addWidget(checkScreen);
    stack->addWidget(mainMenu);
    stack->addWidget(calibrationScreen);
    stack->addWidget(calibrationTestScreen);
    stack->addWidget(explanationScreen);
    stack->addWidget(gameScreen);

    // === CRÉATION GAMELOGIC ===
    gameLogic = new GameLogic(
        cameraAI,
        robot,
        calibrationScreen->getCalibrationLogic(), // getter ajouté
        &stateMachine,
        this
        );

    // === CONNEXIONS GAME SCREEN ↔ GAME LOGIC ===
    connect(gameScreen, &GameScreen::quitRequested, this, [this]() {
        qDebug() << "[MainWindow] Quit demandé depuis GameScreen";
        gameScreen->resetGame();  // Reset complet de l'écran de jeu

        // CRITIQUE : S'assurer que TOUT est arrêté et déconnecté avant de retourner au menu
        ensureFullyDisconnected();

        // Réinitialiser l'état de connexion pour refaire Home() à la prochaine partie
        gameLogic->resetRobotConnection();

        showMenu();
    });

    connect(gameScreen, &GameScreen::prepareGame,
            gameLogic, &GameLogic::prepareGame);

    connect(gameScreen, &GameScreen::countdownFinished,
            gameLogic, &GameLogic::startGame);

    connect(gameScreen, &GameScreen::emergencyStopRequested, this, [this]() {
        qDebug() << "[MainWindow] ⚠️ ARRÊT D'URGENCE demandé depuis GameScreen";

        // 1. Afficher l'overlay immédiatement pour donner un feedback visuel
        gameScreen->showEmergencyStopOverlay();

        // 2. Arrêter le robot ET couper le compresseur IMMÉDIATEMENT (sans bloquer)
        qDebug() << "[MainWindow] Déconnexion d'urgence du robot (arrêt + compresseur)...";
        robot->emergencyDisconnect();

        // 3. Activer le flag pour indiquer qu'un arrêt d'urgence est en cours
        emergencyStopInProgress = true;

        // 4. Lancer le nettoyage du jeu dans un thread séparé pour ne pas bloquer l'interface
        qDebug() << "[MainWindow] Lancement du nettoyage du jeu dans un thread séparé...";
        std::thread([this]() {
            gameLogic->emergencyStopGame();
            qDebug() << "[MainWindow] Nettoyage du jeu terminé";
            // Désactiver le flag une fois le nettoyage terminé
            emergencyStopInProgress = false;
            qDebug() << "[MainWindow] Flag emergencyStopInProgress désactivé";
        }).detach();
    });

    connect(gameScreen, &GameScreen::emergencyStopQuitRequested, this, [this]() {
        qDebug() << "[MainWindow] Retour au menu après arrêt d'urgence demandé";

        // CRITIQUE : Attendre que le thread d'arrêt d'urgence soit terminé SANS bloquer l'interface
        // Lancer l'attente dans un thread séparé pour ne pas bloquer l'UI
        qDebug() << "[MainWindow] Lancement de l'attente du thread d'arrêt d'urgence (asynchrone)...";
        std::thread([this]() {
            qDebug() << "[MainWindow] Attente de la fin du thread d'arrêt d'urgence...";
            int waitCount = 0;
            while (emergencyStopInProgress && waitCount < 50) {  // Max 5 secondes (50 * 100ms)
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                waitCount++;
            }

            if (emergencyStopInProgress) {
                qWarning() << "[MainWindow] TIMEOUT : Le thread d'arrêt d'urgence n'a pas terminé après 5 secondes !";
                qWarning() << "[MainWindow] On continue quand même vers le menu...";
            } else {
                qDebug() << "[MainWindow] Thread d'arrêt d'urgence terminé après" << (waitCount * 100) << "ms";
            }

            // Une fois terminé, retourner au menu dans le thread principal
            QMetaObject::invokeMethod(this, [this]() {
                qDebug() << "[MainWindow] Retour au menu principal - Reset + show menu";

                // Reset de l'écran de jeu AVANT d'afficher le menu (évite les double-clics)
                gameScreen->resetGame();

                // Tout est déjà arrêté par emergencyStopGame() et emergencyDisconnect()
                // Réinitialiser l'état de connexion pour refaire Home() à la prochaine partie
                gameLogic->resetRobotConnection();

                qDebug() << "[MainWindow] Affichage du menu";
                showMenu();
            }, Qt::QueuedConnection);
        }).detach();
    });

    connect(gameLogic, &GameLogic::turnPlayer,
            gameScreen, &GameScreen::setTurnPlayer);

    connect(gameLogic, &GameLogic::turnRobot,
            gameScreen, &GameScreen::setTurnRobot);

    connect(gameLogic, &GameLogic::robotStatus,
            gameScreen, &GameScreen::setRobotStatus);

    connect(gameLogic, &GameLogic::difficultyText,
            gameScreen, &GameScreen::setDifficultyText);

    connect(gameLogic, &GameLogic::sendFrameToScreen,
            gameScreen, &GameScreen::updateCameraFrame);

    connect(gameLogic, &GameLogic::endOfGame,
            gameScreen, &GameScreen::showEndOfGame);

    connect(cameraAI, &CameraAI::gridIncomplete, this, [this](int detectedCount) {
        // Ne pas afficher l'overlay de grille incomplète si on attend le remplissage des réservoirs
        if (!gameScreen->isReservoirOverlayVisible()) {
            gameScreen->showGridIncompleteWarning(detectedCount);
        }
    }, Qt::QueuedConnection);

    connect(cameraAI, &CameraAI::gridComplete, this, [this]() {
        gameScreen->resetAllOverlays();
    }, Qt::QueuedConnection);

    connect(gameLogic, &GameLogic::robotInitializing,
            gameScreen, &GameScreen::showRobotInitializing);

    connect(gameLogic, &GameLogic::robotInitialized,
            gameScreen, &GameScreen::hideRobotInitializing);

    connect(gameLogic, &GameLogic::robotInitialized,
            gameScreen, &GameScreen::startCountdownWhenReady);

    connect(gameLogic, &GameLogic::cheatDetected,
            gameScreen, &GameScreen::showCheatDetected);

    connect(gameLogic, &GameLogic::reservoirEmpty,
            gameScreen, &GameScreen::showReservoirEmpty);

    connect(gameScreen, &GameScreen::reservoirsRefilled,
            gameLogic, &GameLogic::onReservoirsRefilled);

    connect(gameLogic, &GameLogic::gameResult,
            gameScreen, &GameScreen::showGameResult);

    connect(gameLogic, &GameLogic::connectionFailed,
            gameScreen, &GameScreen::showConnectionError);

    // === CONNEXIONS DU MENU ===
    connect(mainMenu, &MainMenu::startGame,
            [&](StateMachine::Difficulty diff, StateMachine::PlayerColor color){
                stateMachine.setDifficulty(diff);
                stateMachine.setPlayerColor(color);
                showGame();
            });

    connect(mainMenu, &MainMenu::startCalibration,
            this, &MainWindow::showCalibration);

    connect(mainMenu, &MainMenu::startCalibrationTest,
            this, &MainWindow::showCalibrationTest);

    connect(mainMenu, &MainMenu::openExplanation,
            this, &MainWindow::showExplanation);

    connect(mainMenu, &MainMenu::quitGame,
            this, &MainWindow::close);

    // === CONNEXIONS CALIBRATION ===
    connect(calibrationScreen, &CalibrationScreen::backToMenuRequested, this, [this]() {
        qDebug() << "[MainWindow] Signal backToMenuRequested reçu depuis CalibrationScreen";

        // CRITIQUE : S'assurer que TOUT est arrêté et déconnecté avant de retourner au menu
        // (robot->disconnect() vérifie si connecté avant de déconnecter, donc pas de double déconnexion)
        ensureFullyDisconnected();

        // Réinitialiser l'état de connexion pour refaire Home() à la prochaine partie
        gameLogic->resetRobotConnection();
        qDebug() << "[MainWindow] État de connexion réinitialisé dans GameLogic";

        showMenu();
        qDebug() << "[MainWindow] Affichage du menu principal effectué";

        // Reset UI après avoir changé d'écran (en asynchrone)
        QTimer::singleShot(100, [this]() {
            qDebug() << "[MainWindow] Début du reset de la calibration";
            calibrationScreen->resetCalibration();
            qDebug() << "[MainWindow] Reset de la calibration terminé";
        });
    });

    // === CONNEXIONS CALIBRATION TEST ===
    connect(calibrationTestScreen, &CalibrationTestScreen::backToMenuRequested, this, [this]() {
        qDebug() << "[MainWindow] Retour au menu depuis le test de calibration";

        // Informer GameLogic que le robot peut avoir été déconnecté
        gameLogic->resetRobotConnection();

        // Animation de retour (vers la droite)
        animateTransition(calibrationTestScreen, mainMenu, false);

        // Reset de l'écran de test après transition
        QTimer::singleShot(500, [this]() {
            calibrationTestScreen->resetScreen();
        });
    });

    // === CONNEXIONS EXPLANATION ===
    connect(explanationScreen, &ExplanationScreen::backToMenu, this, [this]() {
        // Animation verticale vers le bas pour retourner au menu
        animateVerticalTransition(explanationScreen, mainMenu, false);
    });

    // === CONNEXIONS INTRO → CHECK → MENU ===
    connect(introScreen, &IntroScreen::introFinished, this, [this]() {
        showCheck();
        checkScreen->startChecking();
    });

    connect(checkScreen, &CheckDevicesScreen::readyToContinue, this, [this]() {
        // Normalement pas nécessaire ici (robot pas connecté), mais par sécurité
        ensureFullyDisconnected();
        showMenu();
    });

    // === DÉMARRAGE PAR L'INTRO ===
    // Géré dans main.cpp pour respecter les modes debug
}

MainWindow::~MainWindow()
{
    // Nettoyage de CameraAI qui n'a pas de parent
    if (cameraAI) {
        cameraAI->stop();
        delete cameraAI;
        cameraAI = nullptr;
    }
}

void MainWindow::setDebugMode(bool enabled)
{
    debugMode = enabled;
}

// =====================================================
//        MÉTHODES D'AFFICHAGE DES ÉCRANS
// =====================================================

void MainWindow::showIntro()
{
    stack->setCurrentWidget(introScreen);
    introScreen->start();
}

void MainWindow::showCheck()
{
    stack->setCurrentWidget(checkScreen);
}

// ============================================================================
//  ENSURE FULLY DISCONNECTED - S'assurer que TOUT est arrêté avant showMenu()
// ============================================================================
void MainWindow::ensureFullyDisconnected()
{
    qDebug() << "[MainWindow] ========================================";
    qDebug() << "[MainWindow] ENSURE FULLY DISCONNECTED - Vérification complète avant retour menu";
    qDebug() << "[MainWindow] ========================================";

    // 1. Arrêter la caméra (au cas où elle tourne encore)
    qDebug() << "[MainWindow] Arrêt de la caméra...";
    if (cameraAI) {
        cameraAI->stop();
    }

    // 2. Arrêter tous les threads de gameLogic (negamax, préparation, etc.)
    qDebug() << "[MainWindow] Arrêt de tous les threads de gameLogic...";
    if (gameLogic) {
        gameLogic->stopGame();  // Arrête tous les threads et déconnecte le robot proprement
    }

    // 3. S'assurer que le robot est bien déconnecté (sécurité finale)
    //    Utilise CalibrationLogic qui vérifie si déjà déconnecté (évite double déconnexion)
    qDebug() << "[MainWindow] Vérification finale de la déconnexion du robot...";
    if (calibrationScreen && calibrationScreen->getCalibrationLogic()) {
        calibrationScreen->getCalibrationLogic()->disconnectToRobot();
    }

    // 4. Petit délai pour laisser le temps à tout de se terminer proprement
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    qDebug() << "[MainWindow] ========================================";
    qDebug() << "[MainWindow] TOUT EST ARRÊTÉ ET DÉCONNECTÉ - Retour au menu sécurisé";
    qDebug() << "[MainWindow] ========================================";
}

void MainWindow::showMenu()
{
    mainMenu->resetToMainMenu();  // Reset le menu au principal
    stack->setCurrentWidget(mainMenu);
}

void MainWindow::showCalibration()
{
    stack->setCurrentWidget(calibrationScreen);
}

void MainWindow::showCalibrationTest()
{
    calibrationTestScreen->resetScreen();
    animateTransition(stack->currentWidget(), calibrationTestScreen, true);
}

void MainWindow::showExplanation()
{
    // Animation verticale vers le haut (comme pour showCalibrationTest mais vertical)
    animateVerticalTransition(stack->currentWidget(), explanationScreen, true);
}

void MainWindow::showGame()
{
    // Définir la couleur du joueur dans GameScreen pour l'affichage des tours
    gameScreen->setPlayerColor(stateMachine.getPlayerColorValue());

    stack->setCurrentWidget(gameScreen);
    gameScreen->startGame();
}

// =====================================================
//      ANIMATION DE TRANSITION
// =====================================================
void MainWindow::animateTransition(QWidget *from, QWidget *to, bool forward)
{
    if (!from || !to) return;

    int w = stack->width();
    int h = stack->height();

    // Position selon la direction
    // forward=true : vient de la gauche (x=-w au départ)
    // forward=false : repart vers la droite (x=w à la fin)
    int startXTo = forward ? -w : w;
    int endXTo   = 0;
    int endXFrom = forward ? w : -w;

    to->setGeometry(startXTo, 0, w, h);
    to->show();

    auto *slideOut = new QPropertyAnimation(from, "geometry");
    slideOut->setDuration(400);
    slideOut->setEasingCurve(QEasingCurve::InOutQuad);
    slideOut->setStartValue(QRect(0, 0, w, h));
    slideOut->setEndValue(QRect(endXFrom, 0, w, h));

    auto *slideIn = new QPropertyAnimation(to, "geometry");
    slideIn->setDuration(400);
    slideIn->setEasingCurve(QEasingCurve::InOutQuad);
    slideIn->setStartValue(QRect(startXTo, 0, w, h));
    slideIn->setEndValue(QRect(endXTo, 0, w, h));

    connect(slideOut, &QPropertyAnimation::finished, [this, to]() {
        stack->setCurrentWidget(to);
    });

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(slideOut);
    group->addAnimation(slideIn);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

void MainWindow::animateVerticalTransition(QWidget *from, QWidget *to, bool upward)
{
    if (!from || !to) return;

    int w = stack->width();
    int h = stack->height();

    // Position selon la direction
    // upward=true : le nouveau vient du bas (y=h au départ) et monte, l'ancien sort par le haut (y=-h)
    // upward=false : le nouveau vient du haut (y=-h au départ) et descend, l'ancien sort par le bas (y=h)
    int startYTo = upward ? h : -h;
    int endYTo   = 0;
    int endYFrom = upward ? -h : h;

    to->setGeometry(0, startYTo, w, h);
    to->show();

    auto *slideOut = new QPropertyAnimation(from, "geometry");
    slideOut->setDuration(400);
    slideOut->setEasingCurve(QEasingCurve::InOutQuad);
    slideOut->setStartValue(QRect(0, 0, w, h));
    slideOut->setEndValue(QRect(0, endYFrom, w, h));

    auto *slideIn = new QPropertyAnimation(to, "geometry");
    slideIn->setDuration(400);
    slideIn->setEasingCurve(QEasingCurve::InOutQuad);
    slideIn->setStartValue(QRect(0, startYTo, w, h));
    slideIn->setEndValue(QRect(0, endYTo, w, h));

    connect(slideOut, &QPropertyAnimation::finished, [this, to]() {
        stack->setCurrentWidget(to);
    });

    auto *group = new QParallelAnimationGroup(this);
    group->addAnimation(slideOut);
    group->addAnimation(slideIn);
    group->start(QAbstractAnimation::DeleteWhenStopped);
}

// =====================================================
//      FERMETURE DE LA FENÊTRE
// =====================================================

void MainWindow::closeEvent(QCloseEvent *event)
{
    qDebug() << "[MainWindow] Fermeture de l'application demandée";

    // CRITIQUE : S'assurer que TOUT est arrêté et déconnecté avant de fermer
    ensureFullyDisconnected();

    qDebug() << "[MainWindow] Fermeture propre effectuée";
    event->accept();
}
