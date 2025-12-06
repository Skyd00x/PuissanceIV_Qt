#include "CalibrationLogic.hpp"
#include <thread>
#include <chrono>
#include <QMetaObject>
#include <QDebug>
#include <cmath>

CalibrationLogic::CalibrationLogic(Robot* robot, QObject* parent)
    : QObject(parent), robot(robot), connected(false), stepIndex(0), gripperOpen(false), shouldStop_(false),
      currentAxis('x'), currentDelta(0.0f), lastMoveCommandIndex(0)
{
    // Initialiser le timer pour les mouvements continus
    continuousMoveTimer = new QTimer(this);
    continuousMoveTimer->setInterval(50);  // 50ms entre chaque mouvement (20 Hz)
    connect(continuousMoveTimer, &QTimer::timeout, this, [this]() {
        if (this->connected && this->robot) {
            // Vérifier si la commande précédente est terminée avant d'envoyer la suivante
            if (this->lastMoveCommandIndex == 0 || this->robot->isCommandCompleted(this->lastMoveCommandIndex)) {
                // Envoyer une nouvelle commande et stocker son index
                this->lastMoveCommandIndex = this->robot->moveAxisContinuous(this->currentAxis, this->currentDelta);
            }
            // Sinon, on attend le prochain tick du timer
        }
    });

    steps = {
        // Étape 0 : Instructions initiales
        { "Remplissez les réservoirs gauche et droite avec le maximum de pions.<br>",
         "./Ressources/image/Calibration/Etape1.png", true, false, false, false, false, false, false },

        // Étapes 1-2 : Réservoir gauche (seulement 1 et 4)
        { "Positionnez le robot à l'emplacement <b>1</b> du réservoir de gauche, de façon à ce qu'il puisse prendre le pion correctement avec la pince.",
         "./Ressources/image/Calibration/Etape2.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement <b>4</b> du réservoir de gauche, de façon à ce qu'il puisse prendre le pion correctement avec la pince.",
         "./Ressources/image/Calibration/Etape3.png", true, true, true, true, false, false, false },

        // Étapes 3-4 : Réservoir droit (seulement 1 et 4)
        { "Positionnez le robot à l'emplacement <b>1</b> du réservoir de droite, de façon à ce qu'il puisse prendre le pion correctement avec la pince.",
         "./Ressources/image/Calibration/Etape4.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement <b>4</b> du réservoir de droite, de façon à ce qu'il puisse prendre le pion correctement avec la pince.",
         "./Ressources/image/Calibration/Etape5.png", true, true, true, true, false, false, false },

        // Étapes 5-7 : Grille (colonnes 1, 4 et 7 pour précision maximale)
        { "Avec la pince fermée tenant un pion, positionnez le à la <b>première colonne</b> de la grille, de façon à ce qu'il puisse lâcher le pion correctement.",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Avec la pince fermée tenant un pion, positionnez le à la <b>colonne centrale</b> de la grille, de façon à ce qu'il puisse lâcher le pion correctement.",
         "./Ressources/image/Calibration/Etape7.png", true, true, true, true, false, false, false },
        { "Avec la pince fermée tenant un pion, positionnez le à la <b>dernière colonne</b> de la grille, de façon à ce qu'il puisse lâcher le pion correctement.",
         "./Ressources/image/Calibration/Etape8.png", true, true, true, true, false, false, false },

        // Étape 8 : Fin
        { "Calibration terminée.<br>"
         "Toutes les positions ont été enregistrées.<br>"
         "Vous pouvez maintenant retourner au menu principal, ou recommencer si besoin.",
         "./Ressources/image/welcome_calibration.png", false, false, false, false, false, true, true }
    };

    // Charger les positions calibrées si elles existent
    loadCalibration("./calibration.json");
}

// === Connexion au robot ===
bool CalibrationLogic::connectToRobot() {
    if (!robot) return false;
    shouldStop_ = false;  // Réinitialiser le flag au moment de la connexion
    connected = robot->connect();
    emit connectionFinished(connected);
    return connected;
}

void CalibrationLogic::disconnectToRobot(bool emergencyMode) {
    // Si déjà déconnecté, ne rien faire (évite le crash de double déconnexion)
    if (!connected) {
        qDebug() << "[CalibrationLogic] Déjà déconnecté, rien à faire";
        return;
    }

    if (emergencyMode) {
        qDebug() << "[CalibrationLogic] Déconnexion d'urgence du robot (sans toucher au gripper)...";
    } else {
        qDebug() << "[CalibrationLogic] Déconnexion du robot...";
    }

    // Arrêter tous les threads en cours
    shouldStop_ = true;

    // Attendre un peu pour laisser les threads se terminer proprement
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (robot) {
        // En mode d'urgence, NE PAS appeler turnOffGripper() car le mutex peut être verrouillé
        if (!emergencyMode) {
            robot->turnOffGripper();
        } else {
            qDebug() << "[CalibrationLogic] Mode d'urgence : on saute turnOffGripper() pour éviter un deadlock";
        }
        robot->disconnect();
    }
    connected = false;
    gripperOpen = false;

    qDebug() << "[CalibrationLogic] Robot déconnecté";
}

void CalibrationLogic::setConnectedState(bool state) {
    connected = state;
    if (!state) {
        gripperOpen = false;
    }
}

void CalibrationLogic::homeRobot() {
    if (!connected || !robot) return;

    std::thread([this]() {
        if (!robot->Home()) {
            qWarning() << "[CalibrationLogic] ❌ ERREUR : Échec de Home()";
            return;
        }

        // Vérifier si on doit arrêter avant d'émettre le signal
        if (!shouldStop_) {
            QMetaObject::invokeMethod(this, [this]() { emit robotReady(); }, Qt::QueuedConnection);
        }
    }).detach();
}

// === Début de calibration ===
void CalibrationLogic::startCalibration() {
    if (!connected) return;

    stepIndex = 0;

    // Remettre à zéro tous les points calibrés
    for (int i = 0; i < (int)CalibPoint::Count; i++) {
        calibratedPoints[i] = Pose{0, 0, 0, 0};
    }

    emit progressChanged(0);

    if (!steps.empty())
        emit stepChanged(steps[0], 0);
}

// === Enregistrement intermédiaire ===
void CalibrationLogic::recordStep(int index) {
    if (!connected || !robot) return;

    // L'étape 0 est juste l'instruction initiale, pas d'enregistrement
    if (index == 0) {
        stepIndex = 1;
        // Compter l'étape d'instruction comme la première étape (1/8)
        emit progressChanged(1);
        if (stepIndex < (int)steps.size())
            emit stepChanged(steps[stepIndex], stepIndex);
        return;
    }

    // Récupérer la pose actuelle du robot
    Pose p;
    GetPose(&p);

    // Mapping des étapes vers les indices de calibratedPoints
    // Système optimisé : on calibre 7 points clés (2 par réservoir, 3 pour la grille)
    int calibPointIndex = -1;
    switch (index) {
        case 1: calibPointIndex = (int)CalibPoint::Left_1;   break;  // Étape 1 → Left_1
        case 2: calibPointIndex = (int)CalibPoint::Left_4;   break;  // Étape 2 → Left_4
        case 3: calibPointIndex = (int)CalibPoint::Right_1;  break;  // Étape 3 → Right_1
        case 4: calibPointIndex = (int)CalibPoint::Right_4;  break;  // Étape 4 → Right_4
        case 5: calibPointIndex = (int)CalibPoint::Grid_1;   break;  // Étape 5 → Grid_1
        case 6: calibPointIndex = (int)CalibPoint::Grid_4;   break;  // Étape 6 → Grid_4 (centre)
        case 7: calibPointIndex = (int)CalibPoint::Grid_7;   break;  // Étape 7 → Grid_7
        default: break;
    }

    if (calibPointIndex >= 0 && calibPointIndex < (int)CalibPoint::Count) {
        calibratedPoints[calibPointIndex] = p;

        // Afficher un message détaillé pour chaque point enregistré
        QString pointName;
        if (calibPointIndex >= (int)CalibPoint::Left_1 && calibPointIndex <= (int)CalibPoint::Left_4) {
            pointName = QString("Left_%1").arg(calibPointIndex - (int)CalibPoint::Left_1 + 1);
        } else if (calibPointIndex >= (int)CalibPoint::Right_1 && calibPointIndex <= (int)CalibPoint::Right_4) {
            pointName = QString("Right_%1").arg(calibPointIndex - (int)CalibPoint::Right_1 + 1);
        } else if (calibPointIndex >= (int)CalibPoint::Grid_1 && calibPointIndex <= (int)CalibPoint::Grid_7) {
            pointName = QString("Grid_%1").arg(calibPointIndex - (int)CalibPoint::Grid_1 + 1);
        }

        qDebug() << "[CalibrationLogic] 📍 Point clé" << pointName << "enregistré : x=" << p.x << " y=" << p.y << " z=" << p.z << " r=" << p.r;
    }

    // Progression : index + 1 pour que l'étape 1 affiche 2/8, ..., l'étape 7 affiche 8/8
    emit progressChanged(index + 1);

    // Dernière étape (index 7) = calculer les positions intermédiaires puis sauvegarder
    if (index == 7) {
        qDebug() << "[CalibrationLogic] 🔄 Étape finale : calcul des positions intermédiaires...";
        computeIntermediatePositions();
        qDebug() << "[CalibrationLogic] 💾 Sauvegarde de tous les points...";
        saveCalibration("./calibration.json");

        // La progression est déjà à 8/8 grâce à progressChanged(index + 1) ci-dessus
        emit calibrationFinished();
    }

    stepIndex = index + 1;
    if (stepIndex < (int)steps.size())
        emit stepChanged(steps[stepIndex], stepIndex);
}

void CalibrationLogic::previousStep() {
    if (!connected) return;

    if (stepIndex <= 0) {
        stepIndex = 0;
        emit stepChanged(steps[0], 0);
        emit progressChanged(0);
        return;
    }

    stepIndex--;
    emit progressChanged(stepIndex);
    emit stepChanged(steps[stepIndex], stepIndex);
}

void CalibrationLogic::resetCalibration() {
    // Remettre à zéro tous les points calibrés
    for (int i = 0; i < (int)CalibPoint::Count; i++) {
        calibratedPoints[i] = Pose{0, 0, 0, 0};
    }

    stepIndex = 0;
    emit progressChanged(0);

    if (!steps.empty())
        emit stepChanged(steps[0], 0);
}

// === Interpolation ===
std::vector<Pose> CalibrationLogic::interpolatePoints(const Pose& start, const Pose& end, int count) {
    std::vector<Pose> pts;
    if (count < 2) return pts;

    pts.reserve(count);
    for (int i = 0; i < count; ++i) {
        float t = float(i) / float(count - 1);
        Pose p;
        p.x = start.x + (end.x - start.x) * t;
        p.y = start.y + (end.y - start.y) * t;
        // Interpoler le Z aussi pour tenir compte de l'inclinaison réelle du réservoir/grille
        p.z = start.z + (end.z - start.z) * t;
        p.r = start.r + (end.r - start.r) * t;
        pts.push_back(p);
    }
    return pts;
}

// ================================================
//   Calcul des positions intermédiaires
//   Interpolation linéaire à partir des points clés calibrés
// ================================================
void CalibrationLogic::computeIntermediatePositions() {
    qDebug() << "[CalibrationLogic] Calcul des positions intermédiaires par interpolation";

    // Sauvegarder les points clés manuellement calibrés AVANT interpolation
    // pour garantir qu'ils ne soient pas modifiés par des erreurs d'arrondi
    Pose left1_manual  = calibratedPoints[(int)CalibPoint::Left_1];
    Pose left4_manual  = calibratedPoints[(int)CalibPoint::Left_4];
    Pose right1_manual = calibratedPoints[(int)CalibPoint::Right_1];
    Pose right4_manual = calibratedPoints[(int)CalibPoint::Right_4];
    Pose grid1_manual  = calibratedPoints[(int)CalibPoint::Grid_1];
    Pose grid4_manual  = calibratedPoints[(int)CalibPoint::Grid_4];
    Pose grid7_manual  = calibratedPoints[(int)CalibPoint::Grid_7];

    // 1️⃣ Réservoir gauche : interpoler Left_2 et Left_3 entre Left_1 et Left_4
    auto leftReservoir = interpolatePoints(left1_manual, left4_manual, 4);
    // Stocker UNIQUEMENT les points intermédiaires (2 et 3), pas les extrémités
    calibratedPoints[(int)CalibPoint::Left_2] = leftReservoir[1];
    calibratedPoints[(int)CalibPoint::Left_3] = leftReservoir[2];
    qDebug() << "  Left_1: x=" << left1_manual.x << " y=" << left1_manual.y << " z=" << left1_manual.z << " (manuel)";
    qDebug() << "  Left_2: x=" << leftReservoir[1].x << " y=" << leftReservoir[1].y << " z=" << leftReservoir[1].z << " (interpolé)";
    qDebug() << "  Left_3: x=" << leftReservoir[2].x << " y=" << leftReservoir[2].y << " z=" << leftReservoir[2].z << " (interpolé)";
    qDebug() << "  Left_4: x=" << left4_manual.x << " y=" << left4_manual.y << " z=" << left4_manual.z << " (manuel)";

    // 2️⃣ Réservoir droit : interpoler Right_2 et Right_3 entre Right_1 et Right_4
    auto rightReservoir = interpolatePoints(right1_manual, right4_manual, 4);
    calibratedPoints[(int)CalibPoint::Right_2] = rightReservoir[1];
    calibratedPoints[(int)CalibPoint::Right_3] = rightReservoir[2];
    qDebug() << "  Right_1: x=" << right1_manual.x << " y=" << right1_manual.y << " z=" << right1_manual.z << " (manuel)";
    qDebug() << "  Right_2: x=" << rightReservoir[1].x << " y=" << rightReservoir[1].y << " z=" << rightReservoir[1].z << " (interpolé)";
    qDebug() << "  Right_3: x=" << rightReservoir[2].x << " y=" << rightReservoir[2].y << " z=" << rightReservoir[2].z << " (interpolé)";
    qDebug() << "  Right_4: x=" << right4_manual.x << " y=" << right4_manual.y << " z=" << right4_manual.z << " (manuel)";

    // 3️⃣ Grille : interpoler en deux segments (1→4 et 4→7) pour précision maximale
    // Segment 1 : Grid_1 → Grid_4 (colonnes 1, 2, 3, 4)
    auto gridLeft = interpolatePoints(grid1_manual, grid4_manual, 4);
    calibratedPoints[(int)CalibPoint::Grid_2] = gridLeft[1];
    calibratedPoints[(int)CalibPoint::Grid_3] = gridLeft[2];
    qDebug() << "  Grid_1: x=" << grid1_manual.x << " y=" << grid1_manual.y << " z=" << grid1_manual.z << " (manuel)";
    qDebug() << "  Grid_2: x=" << gridLeft[1].x << " y=" << gridLeft[1].y << " z=" << gridLeft[1].z << " (interpolé 1→4)";
    qDebug() << "  Grid_3: x=" << gridLeft[2].x << " y=" << gridLeft[2].y << " z=" << gridLeft[2].z << " (interpolé 1→4)";
    qDebug() << "  Grid_4: x=" << grid4_manual.x << " y=" << grid4_manual.y << " z=" << grid4_manual.z << " (manuel)";

    // Segment 2 : Grid_4 → Grid_7 (colonnes 4, 5, 6, 7)
    auto gridRight = interpolatePoints(grid4_manual, grid7_manual, 4);
    calibratedPoints[(int)CalibPoint::Grid_5] = gridRight[1];
    calibratedPoints[(int)CalibPoint::Grid_6] = gridRight[2];
    qDebug() << "  Grid_5: x=" << gridRight[1].x << " y=" << gridRight[1].y << " z=" << gridRight[1].z << " (interpolé 4→7)";
    qDebug() << "  Grid_6: x=" << gridRight[2].x << " y=" << gridRight[2].y << " z=" << gridRight[2].z << " (interpolé 4→7)";
    qDebug() << "  Grid_7: x=" << grid7_manual.x << " y=" << grid7_manual.y << " z=" << grid7_manual.z << " (manuel)";

    qDebug() << "[CalibrationLogic] ✅ Toutes les positions intermédiaires calculées (7 points manuels + 8 interpolés = 15 points)";
}

// === Test des positions ===
void CalibrationLogic::testCalibration() {
    if (!connected) return;

    std::thread([this]() {
        int total = (int)CalibPoint::Count;
        float safeZ = getSafeHeight();

        for (int i = 0; i < total; i++) {
            // Vérifier si on doit arrêter
            if (shouldStop_) {
                qDebug() << "[CalibrationLogic] Test de calibration interrompu";
                return;
            }

            if (!robot->goToSecurized(calibratedPoints[i], safeZ)) {
                qWarning() << "[CalibrationLogic] ❌ ERREUR : Test de calibration échoué au point" << i;
                return;
            }

            int progress = (i + 1) * 100 / total;
            if (!shouldStop_) {
                QMetaObject::invokeMethod(this, [this, progress]() {
                    emit progressChanged(progress);
                }, Qt::QueuedConnection);
            }

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }

        // Émettre le signal de fin seulement si on n'a pas été interrompu
        if (!shouldStop_) {
            QMetaObject::invokeMethod(this, [this]() {
                emit calibrationTestFinished();
                emit progressChanged(100);
            }, Qt::QueuedConnection);
        }

    }).detach();
}

// === Manipulations manuelles ===
void CalibrationLogic::toggleGripper() {
    if (!connected || !robot) return;

    if (gripperOpen) {
        // Fermer la pince
        robot->closeGripper();
        gripperOpen = false;

        // Attendre que la pince soit bien fermée
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // IMPORTANT: Laisser le compresseur allumé pour maintenir la prise
        qDebug() << "[CalibrationLogic] Pince fermée - compresseur maintenu actif";
    }
    else {
        // Ouvrir la pince
        robot->openGripper();
        gripperOpen = true;

        // Attendre que la pince soit bien ouverte
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Couper le compresseur après ouverture
        robot->turnOffGripper();
        qDebug() << "[CalibrationLogic] Pince ouverte - compresseur coupé";
    }

    // Émettre le signal de changement d'état
    emit gripperStateChanged(gripperOpen);
}

void CalibrationLogic::rotateLeft()  { if (connected) robot->rotate(+5); }
void CalibrationLogic::rotateRight() { if (connected) robot->rotate(-5); }

// === Déplacements fins sur les axes (1mm par clic) ===
void CalibrationLogic::moveXPlus()  { if (connected && robot) robot->moveAxis('x', +1.0f); }
void CalibrationLogic::moveXMinus() { if (connected && robot) robot->moveAxis('x', -1.0f); }
void CalibrationLogic::moveYPlus()  { if (connected && robot) robot->moveAxis('y', +1.0f); }
void CalibrationLogic::moveYMinus() { if (connected && robot) robot->moveAxis('y', -1.0f); }
void CalibrationLogic::moveZPlus()  { if (connected && robot) robot->moveAxis('z', +1.0f); }
void CalibrationLogic::moveZMinus() { if (connected && robot) robot->moveAxis('z', -1.0f); }

// === Déplacements continus (mode "joystick") ===
void CalibrationLogic::startContinuousMove(char axis, float delta) {
    if (!connected || !robot) return;

    currentAxis = axis;
    currentDelta = delta;

    // Démarrer le timer qui va répéter le mouvement
    if (!continuousMoveTimer->isActive()) {
        continuousMoveTimer->start();
    }
}

void CalibrationLogic::stopContinuousMove() {
    // Arrêter le timer
    if (continuousMoveTimer->isActive()) {
        continuousMoveTimer->stop();
    }
    // Réinitialiser l'index de la dernière commande
    lastMoveCommandIndex = 0;
}

// === Helper pour obtenir le point générique de la grille ===
Pose CalibrationLogic::getGridGenericPoint() const {
    Pose gridGeneric;

    // Valeurs par défaut pour la grille (votre configuration)
    gridGeneric.x = 241.0f;
    gridGeneric.y = 6.0f;
    gridGeneric.z = 104.0f;
    gridGeneric.r = 3.2f;

    return gridGeneric;
}

// === Déplacements automatiques vers les zones de calibration ===
void CalibrationLogic::goToLeftReservoirArea() {
    if (!connected || !robot) return;

    Pose target;

    // Valeurs par défaut pour le réservoir gauche (votre configuration)
    target.x = 29.5f;
    target.y = -220.3f;
    target.z = -70.0f;
    target.r = -80.0f;

    // IMPORTANT : Utiliser le Z du point générique de la grille comme hauteur de sécurité
    Pose gridGeneric = getGridGenericPoint();
    float gridZ = gridGeneric.z;

    qDebug() << "[CalibrationLogic] Déplacement vers réservoir gauche avec hauteur grille z=" << gridZ;
    if (!robot->goToSecurized(target, gridZ)) {
        qWarning() << "[CalibrationLogic] ❌ ERREUR : Impossible d'atteindre le réservoir gauche";
    }
}

void CalibrationLogic::goToRightReservoirArea() {
    if (!connected || !robot) return;

    Pose target;

    // Valeurs par défaut pour le réservoir droit (votre configuration)
    target.x = 16.5f;
    target.y = 222.0f;
    target.z = -70.0f;
    target.r = 95.2f;

    // IMPORTANT : Utiliser le Z du point générique de la grille comme hauteur de sécurité
    Pose gridGeneric = getGridGenericPoint();
    float gridZ = gridGeneric.z;

    qDebug() << "[CalibrationLogic] Déplacement vers réservoir droit avec hauteur grille z=" << gridZ;
    if (!robot->goToSecurized(target, gridZ)) {
        qWarning() << "[CalibrationLogic] ❌ ERREUR : Impossible d'atteindre le réservoir droit";
    }
}

void CalibrationLogic::goToGridArea() {
    if (!connected || !robot) return;

    // Obtenir le point générique de la grille
    Pose target = getGridGenericPoint();

    qDebug() << "[CalibrationLogic] Déplacement vers grille : x=" << target.x << "y=" << target.y << "z=" << target.z;

    // Utiliser le Z du point générique de la grille comme hauteur de sécurité
    float gridZ = target.z;
    if (!robot->goToSecurized(target, gridZ)) {
        qWarning() << "[CalibrationLogic] ❌ ERREUR : Impossible d'atteindre la grille";
    }
}

// === Sauvegarde ===
void CalibrationLogic::saveCalibration(const QString& path) {
    QJsonArray arr;

    qDebug() << "[CalibrationLogic] 💾 Sauvegarde de la calibration dans" << path;

    for (int i = 0; i < (int)CalibPoint::Count; i++) {
        const Pose& p = calibratedPoints[i];
        QJsonObject o;
        o["index"] = i;
        o["x"] = p.x;
        o["y"] = p.y;
        o["z"] = p.z;
        o["r"] = p.r;
        arr.append(o);
    }

    QJsonObject root;
    root["points"] = arr;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson());
        f.close();
        qDebug() << "[CalibrationLogic] ✅ Calibration sauvegardée avec succès (" << (int)CalibPoint::Count << "points)";
    } else {
        qDebug() << "[CalibrationLogic] ❌ ERREUR : Impossible d'ouvrir le fichier" << path << "pour l'écriture";
    }
}

// === Chargement ===
void CalibrationLogic::loadCalibration(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qDebug() << "[CalibrationLogic] ⚠️ Fichier de calibration non trouvé:" << path << "(première utilisation ou calibration non effectuée)";
        return;
    }

    QByteArray data = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        qWarning() << "[CalibrationLogic] ❌ Fichier de calibration invalide (format JSON incorrect)";
        return;
    }

    QJsonObject root = doc.object();

    if (root.contains("points")) {
        QJsonArray arr = root["points"].toArray();
        int loadedCount = 0;
        for (const auto& v : arr) {
            QJsonObject o = v.toObject();
            int i = o["index"].toInt();
            if (i >= 0 && i < (int)CalibPoint::Count) {
                calibratedPoints[i].x = o["x"].toDouble();
                calibratedPoints[i].y = o["y"].toDouble();
                calibratedPoints[i].z = o["z"].toDouble();
                calibratedPoints[i].r = o["r"].toDouble();
                loadedCount++;
            }
        }
        qDebug() << "[CalibrationLogic] ✅ Calibration chargée avec succès:" << loadedCount << "points depuis" << path;
    } else {
        qWarning() << "[CalibrationLogic] ❌ Fichier de calibration ne contient pas de points";
    }
}

// =====================================================
//   Nouveaux helpers utilisés par GameLogic / Negamax
// =====================================================
Pose CalibrationLogic::getPosePick() const {
    // On utilise par défaut le premier emplacement de la réserve gauche
    // (Left_1). Tu pourras changer la logique si besoin (réserve droite, etc.).
    return calibratedPoints[(int)CalibPoint::Left_1];
}

Pose CalibrationLogic::getPoseForColumn(int col) const {
    // 7 colonnes : 0..6 → Grid_1..Grid_7
    if (col < 0) col = 0;
    if (col > 6) col = 6;
    return calibratedPoints[(int)CalibPoint::Grid_1 + col];
}

float CalibrationLogic::getSafeHeight() const {
    // IMPORTANT: La hauteur de sécurité doit TOUJOURS être basée sur les points de la grille,
    // car c'est le point le plus haut du parcours normal du robot.
    // Parcourir UNIQUEMENT les points de la grille (Grid_1 à Grid_7)
    float maxZ = -1000.0f;  // Valeur très basse pour commencer

    for (int i = (int)CalibPoint::Grid_1; i <= (int)CalibPoint::Grid_7; i++) {
        if (calibratedPoints[i].z > maxZ) {
            maxZ = calibratedPoints[i].z;
        }
    }

    // Si aucun point de grille n'a été trouvé (calibration vide), utiliser une valeur par défaut
    if (maxZ < -999.0f) {
        return 150.0f;
    }

    // Retourner le z max de la grille + 30 pour la sécurité
    return maxZ + 30.0f;
}

Pose CalibrationLogic::getReservoirGenericPoint(bool isLeftReservoir) const {
    Pose genericPoint;

    if (isLeftReservoir) {
        // Point générique du réservoir gauche : moyenne entre Left_1 et Left_4
        // EXACTEMENT comme dans goToLeftReservoirArea()
        if (calibratedPoints[(int)CalibPoint::Left_1].x != 0 || calibratedPoints[(int)CalibPoint::Left_4].x != 0) {
            Pose left1 = calibratedPoints[(int)CalibPoint::Left_1];
            Pose left4 = calibratedPoints[(int)CalibPoint::Left_4];

            genericPoint.x = (left1.x + left4.x) / 2.0f;
            genericPoint.y = (left1.y + left4.y) / 2.0f;
            genericPoint.z = (left1.z + left4.z) / 2.0f;
            genericPoint.r = (left1.r + left4.r) / 2.0f;
        } else {
            // Valeurs par défaut pour le réservoir gauche
            genericPoint.x = 29.5f;
            genericPoint.y = -220.3f;
            genericPoint.z = -70.0f;
            genericPoint.r = -80.0f;
        }
    } else {
        // Point générique du réservoir droit : moyenne entre Right_1 et Right_4
        // EXACTEMENT comme dans goToRightReservoirArea()
        if (calibratedPoints[(int)CalibPoint::Right_1].x != 0 || calibratedPoints[(int)CalibPoint::Right_4].x != 0) {
            Pose right1 = calibratedPoints[(int)CalibPoint::Right_1];
            Pose right4 = calibratedPoints[(int)CalibPoint::Right_4];

            genericPoint.x = (right1.x + right4.x) / 2.0f;
            genericPoint.y = (right1.y + right4.y) / 2.0f;
            genericPoint.z = (right1.z + right4.z) / 2.0f;
            genericPoint.r = (right1.r + right4.r) / 2.0f;
        } else {
            // Valeurs par défaut pour le réservoir droit
            genericPoint.x = 16.5f;
            genericPoint.y = 222.0f;
            genericPoint.z = -70.0f;
            genericPoint.r = 95.2f;
        }
    }

    return genericPoint;
}

// =====================================================
//   Fonctions de haut niveau pour manipuler les pions
// =====================================================

bool CalibrationLogic::pickPiece(CalibPoint reservoirPosition) {
    if (!connected || !robot) {
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : robot non connecté";
        return false;
    }

    // Vérifier que la position est bien dans un réservoir
    int posIndex = (int)reservoirPosition;
    bool isLeftReservoir = (posIndex >= (int)CalibPoint::Left_1 && posIndex <= (int)CalibPoint::Left_4);
    bool isRightReservoir = (posIndex >= (int)CalibPoint::Right_1 && posIndex <= (int)CalibPoint::Right_4);

    if (!isLeftReservoir && !isRightReservoir) {
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : position invalide (doit être Left_1..4 ou Right_1..4)";
        return false;
    }

    // IMPORTANT: Ouvrir la pince AVANT de descendre sur le pion
    if (!robot->openGripper()) {
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : impossible d'ouvrir la pince";
        return false;
    }
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Récupérer la position calibrée
    Pose pickPose = calibratedPoints[posIndex];

    // Aller chercher le pion de manière sécurisée avec hauteur calculée
    float safeZ = getSafeHeight();
    if (!robot->goToSecurized(pickPose, safeZ)) {
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : impossible d'atteindre la position du pion";
        return false;
    }

    // Fermer la pince pour attraper le pion
    if (!robot->closeGripper()) {
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : impossible de fermer la pince";
        return false;
    }

    // Petite pause pour s'assurer que le pion est bien attrapé
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // IMPORTANT: Remonter doucement de 30mm sur Z pour extraire le pion de l'emplacement
    Pose currentPose;
    int result = GetPose(&currentPose);
    if (result != DobotCommunicate_NoError) {
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : impossible de lire la position actuelle";
        return false;
    }
    currentPose.z += 30.0f;

    qDebug() << "[CalibrationLogic] Extraction du pion : remontée lente de 30mm à z=" << currentPose.z;
    if (!robot->goTo(currentPose, true)) {  // true = mouvement de précision (lent) pour l'extraction
        qWarning() << "[CalibrationLogic] ❌ pickPiece() ÉCHEC : impossible d'extraire le pion";
        return false;
    }

    qDebug() << "[CalibrationLogic] ✅ pickPiece() réussi";
    return true;
}

bool CalibrationLogic::dropPiece(int column) {
    if (!connected || !robot) {
        qWarning() << "[CalibrationLogic] ❌ dropPiece() ÉCHEC : robot non connecté";
        return false;
    }

    // Vérifier que la colonne est valide (0..6)
    if (column < 0 || column > 6) {
        qWarning() << "[CalibrationLogic] ❌ dropPiece() ÉCHEC : colonne invalide (doit être 0..6)";
        return false;
    }

    // Récupérer la position de la colonne
    Pose dropPose = getPoseForColumn(column);

    // Aller à la colonne de manière sécurisée avec hauteur calculée
    float safeZ = getSafeHeight();
    if (!robot->goToSecurized(dropPose, safeZ)) {
        qWarning() << "[CalibrationLogic] ❌ dropPiece() ÉCHEC : impossible d'atteindre la colonne";
        return false;
    }

    // Ouvrir la pince pour lâcher le pion
    if (!robot->openGripper()) {
        qWarning() << "[CalibrationLogic] ❌ dropPiece() ÉCHEC : impossible d'ouvrir la pince";
        return false;
    }

    // Petite pause pour s'assurer que le pion est bien lâché
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Désactiver la pince (économie d'énergie et sécurité)
    if (!robot->turnOffGripper()) {
        qWarning() << "[CalibrationLogic] ❌ dropPiece() ÉCHEC : impossible de couper la pince";
        return false;
    }

    qDebug() << "[CalibrationLogic] ✅ dropPiece() réussi";
    return true;
}
