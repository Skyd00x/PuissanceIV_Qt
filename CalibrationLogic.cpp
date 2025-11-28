#include "CalibrationLogic.hpp"
#include <thread>
#include <chrono>
#include <QMetaObject>
#include <QDebug>
#include <cmath>

CalibrationLogic::CalibrationLogic(Robot* robot, QObject* parent)
    : QObject(parent), robot(robot), connected(false), stepIndex(0), gripperOpen(false), shouldStop_(false)
{
    steps = {
        // Étape 0 : Instructions initiales
        { "Videz les réservoirs, puis placez un pion dans le réservoir de gauche à l'emplacement 1.",
         "./Ressources/image/Calibration/Etape1.png", true, false, false, false, false, false, false },

        // Étapes 1-4 : Réservoir gauche
        { "Positionnez le robot à l'emplacement 1 du réservoir de gauche.",
         "./Ressources/image/Calibration/Etape2.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement 2 du réservoir de gauche.",
         "./Ressources/image/Calibration/Etape2.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement 3 du réservoir de gauche.",
         "./Ressources/image/Calibration/Etape2.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement 4 du réservoir de gauche.",
         "./Ressources/image/Calibration/Etape3.png", true, true, true, true, false, false, false },

        // Étapes 5-8 : Réservoir droit
        { "Positionnez le robot à l'emplacement 1 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape4.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement 2 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape4.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement 3 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape4.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à l'emplacement 4 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape5.png", true, true, true, true, false, false, false },

        // Étapes 9-15 : Grille colonnes 1-7
        { "Positionnez le robot à la colonne 1 de la grille (tout à gauche).",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à la colonne 2 de la grille.",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à la colonne 3 de la grille.",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à la colonne 4 de la grille (centre).",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à la colonne 5 de la grille.",
         "./Ressources/image/Calibration/Etape7.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à la colonne 6 de la grille.",
         "./Ressources/image/Calibration/Etape7.png", true, true, true, true, false, false, false },
        { "Positionnez le robot à la colonne 7 de la grille (tout à droite).",
         "./Ressources/image/Calibration/Etape7.png", true, true, true, true, false, false, false },

        // Étape 16 : Fin
        { "Calibration terminée.<br>Vous pouvez maintenant tester les positions, recommencer ou quitter.",
         "./Ressources/image/welcome_calibration.png", false, false, false, false, true, true, true }
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

void CalibrationLogic::disconnectToRobot() {
    // Si déjà déconnecté, ne rien faire (évite le crash de double déconnexion)
    if (!connected) {
        qDebug() << "[CalibrationLogic] Déjà déconnecté, rien à faire";
        return;
    }

    qDebug() << "[CalibrationLogic] Déconnexion du robot...";

    // Arrêter tous les threads en cours
    shouldStop_ = true;

    // Attendre un peu pour laisser les threads se terminer proprement
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    if (robot) {
        robot->turnOffGripper();
        robot->disconnect();
    }
    connected = false;
    gripperOpen = false;

    qDebug() << "[CalibrationLogic] Robot déconnecté";
}

void CalibrationLogic::homeRobot() {
    if (!connected || !robot) return;

    std::thread([this]() {
        robot->Home();

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
        if (stepIndex < (int)steps.size())
            emit stepChanged(steps[stepIndex], stepIndex);
        return;
    }

    // Récupérer la pose actuelle du robot
    Pose p;
    GetPose(&p);

    // Enregistrer directement dans calibratedPoints
    // index 1-15 correspondent aux CalibPoint 0-14
    int calibPointIndex = index - 1;
    if (calibPointIndex >= 0 && calibPointIndex < (int)CalibPoint::Count) {
        calibratedPoints[calibPointIndex] = p;
        qDebug() << "[CalibrationLogic] Position" << calibPointIndex << "enregistrée : x=" << p.x << "y=" << p.y << "z=" << p.z << "r=" << p.r;
    }

    emit progressChanged(index);

    // Dernière étape (index 15 = Grid_7) = fin de calibration
    if (index == 15) {
        saveCalibration("./calibration.json");
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
//   Génération des 15 points calibrés
//   NOTE: Cette fonction n'est plus utilisée depuis la nouvelle calibration
//         qui enregistre directement tous les 15 points sans interpolation.
//         Conservée pour compatibilité avec anciennes versions.
// ================================================
void CalibrationLogic::computeAllPositions() {
    // Cette fonction n'est plus utilisée avec la nouvelle méthode de calibration
    return;

    Pose left1  = calibrationData[1].pose;
    Pose left4  = calibrationData[2].pose;
    Pose right1 = calibrationData[3].pose;
    Pose right4 = calibrationData[4].pose;
    Pose grid1  = calibrationData[5].pose;
    Pose grid7  = calibrationData[6].pose;

    // 1️⃣ Réserve gauche
    auto L = interpolatePoints(left1, left4, 4);
    for (int i = 0; i < 4; i++)
        calibratedPoints[(int)CalibPoint::Left_1 + i] = L[i];

    // 2️⃣ Réserve droite
    auto R = interpolatePoints(right1, right4, 4);
    for (int i = 0; i < 4; i++)
        calibratedPoints[(int)CalibPoint::Right_1 + i] = R[i];

    // 3️⃣ Colonnes de grille (7)
    auto G = interpolatePoints(grid1, grid7, 7);
    for (int i = 0; i < 7; i++)
        calibratedPoints[(int)CalibPoint::Grid_1 + i] = G[i];

    saveCalibration("./calibration.json");
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

            robot->goToSecurized(calibratedPoints[i], safeZ);

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

// === Déplacements automatiques vers les zones de calibration ===
void CalibrationLogic::goToLeftReservoirArea() {
    if (!connected || !robot) return;

    Pose target;

    // Si la calibration existe déjà, utiliser la moyenne entre Left_1 et Left_4
    if (calibratedPoints[(int)CalibPoint::Left_1].x != 0 || calibratedPoints[(int)CalibPoint::Left_4].x != 0) {
        Pose left1 = calibratedPoints[(int)CalibPoint::Left_1];
        Pose left4 = calibratedPoints[(int)CalibPoint::Left_4];

        target.x = (left1.x + left4.x) / 2.0f;
        target.y = (left1.y + left4.y) / 2.0f;
        target.z = (left1.z + left4.z) / 2.0f;
        target.r = (left1.r + left4.r) / 2.0f;
    } else {
        // Valeurs par défaut pour le réservoir gauche (votre configuration)
        target.x = 29.5f;
        target.y = -220.3f;
        target.z = -70.0f;
        target.r = -80.0f;
    }

    qDebug() << "[CalibrationLogic] Déplacement vers réservoir gauche : x=" << target.x << "y=" << target.y << "z=" << target.z;
    robot->goToSecurized(target, getSafeHeight());
}

void CalibrationLogic::goToRightReservoirArea() {
    if (!connected || !robot) return;

    Pose target;

    // Si la calibration existe déjà, utiliser la moyenne entre Right_1 et Right_4
    if (calibratedPoints[(int)CalibPoint::Right_1].x != 0 || calibratedPoints[(int)CalibPoint::Right_4].x != 0) {
        Pose right1 = calibratedPoints[(int)CalibPoint::Right_1];
        Pose right4 = calibratedPoints[(int)CalibPoint::Right_4];

        target.x = (right1.x + right4.x) / 2.0f;
        target.y = (right1.y + right4.y) / 2.0f;
        target.z = (right1.z + right4.z) / 2.0f;
        target.r = (right1.r + right4.r) / 2.0f;
    } else {
        // Valeurs par défaut pour le réservoir droit (votre configuration)
        target.x = 16.5f;
        target.y = 222.0f;
        target.z = -70.0f;
        target.r = 95.2f;
    }

    qDebug() << "[CalibrationLogic] Déplacement vers réservoir droit : x=" << target.x << "y=" << target.y << "z=" << target.z;
    robot->goToSecurized(target, getSafeHeight());
}

void CalibrationLogic::goToGridArea() {
    if (!connected || !robot) return;

    Pose target;

    // Si la calibration existe déjà, utiliser la moyenne entre Grid_1 et Grid_7
    if (calibratedPoints[(int)CalibPoint::Grid_1].x != 0 || calibratedPoints[(int)CalibPoint::Grid_7].x != 0) {
        Pose grid1 = calibratedPoints[(int)CalibPoint::Grid_1];
        Pose grid7 = calibratedPoints[(int)CalibPoint::Grid_7];

        target.x = (grid1.x + grid7.x) / 2.0f;
        target.y = (grid1.y + grid7.y) / 2.0f;
        target.z = (grid1.z + grid7.z) / 2.0f;
        target.r = (grid1.r + grid7.r) / 2.0f;
    } else {
        // Valeurs par défaut pour la grille (votre configuration)
        target.x = 241.0f;
        target.y = 6.0f;
        target.z = 104.0f;
        target.r = 3.2f;
    }

    qDebug() << "[CalibrationLogic] Déplacement vers grille : x=" << target.x << "y=" << target.y << "z=" << target.z;
    robot->goToSecurized(target, getSafeHeight());
}

// === Sauvegarde ===
void CalibrationLogic::saveCalibration(const QString& path) {
    QJsonArray arr;

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
    }
}

// === Chargement ===
void CalibrationLogic::loadCalibration(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QByteArray data = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;

    QJsonObject root = doc.object();

    if (root.contains("points")) {
        QJsonArray arr = root["points"].toArray();
        for (const auto& v : arr) {
            QJsonObject o = v.toObject();
            int i = o["index"].toInt();
            if (i >= 0 && i < (int)CalibPoint::Count) {
                calibratedPoints[i].x = o["x"].toDouble();
                calibratedPoints[i].y = o["y"].toDouble();
                calibratedPoints[i].z = o["z"].toDouble();
                calibratedPoints[i].r = o["r"].toDouble();
            }
        }
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
    // Parcourir tous les points calibrés pour trouver le z maximum
    float maxZ = -1000.0f;  // Valeur très basse pour commencer

    for (int i = 0; i < (int)CalibPoint::Count; i++) {
        if (calibratedPoints[i].z > maxZ) {
            maxZ = calibratedPoints[i].z;
        }
    }

    // Si aucun point n'a été trouvé (calibration vide), utiliser une valeur par défaut
    if (maxZ < -999.0f) {
        return 150.0f;
    }

    // Retourner le z max + 30 pour la sécurité
    return maxZ + 30.0f;
}

// =====================================================
//   Fonctions de haut niveau pour manipuler les pions
// =====================================================

void CalibrationLogic::pickPiece(CalibPoint reservoirPosition) {
    if (!connected || !robot) return;

    // Vérifier que la position est bien dans un réservoir
    int posIndex = (int)reservoirPosition;
    bool isLeftReservoir = (posIndex >= (int)CalibPoint::Left_1 && posIndex <= (int)CalibPoint::Left_4);
    bool isRightReservoir = (posIndex >= (int)CalibPoint::Right_1 && posIndex <= (int)CalibPoint::Right_4);

    if (!isLeftReservoir && !isRightReservoir) {
        qDebug() << "[CalibrationLogic] ERREUR: pickPiece() requiert une position de réservoir (Left_1..4 ou Right_1..4)";
        return;
    }

    // IMPORTANT: Ouvrir la pince AVANT de descendre sur le pion
    robot->openGripper();
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Récupérer la position calibrée
    Pose pickPose = calibratedPoints[posIndex];

    // Aller chercher le pion de manière sécurisée avec hauteur calculée
    float safeZ = getSafeHeight();
    robot->goToSecurized(pickPose, safeZ);

    // Fermer la pince pour attraper le pion
    robot->closeGripper();

    // Petite pause pour s'assurer que le pion est bien attrapé
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
}

void CalibrationLogic::dropPiece(int column) {
    if (!connected || !robot) return;

    // Vérifier que la colonne est valide (0..6)
    if (column < 0 || column > 6) {
        qDebug() << "[CalibrationLogic] ERREUR: dropPiece() requiert une colonne entre 0 et 6";
        return;
    }

    // Récupérer la position de la colonne
    Pose dropPose = getPoseForColumn(column);

    // Aller à la colonne de manière sécurisée avec hauteur calculée
    float safeZ = getSafeHeight();
    robot->goToSecurized(dropPose, safeZ);

    // Ouvrir la pince pour lâcher le pion
    robot->openGripper();

    // Petite pause pour s'assurer que le pion est bien lâché
    std::this_thread::sleep_for(std::chrono::milliseconds(300));

    // Désactiver la pince (économie d'énergie et sécurité)
    robot->turnOffGripper();
}
