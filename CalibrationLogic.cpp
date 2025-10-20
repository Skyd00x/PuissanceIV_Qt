#include "CalibrationLogic.hpp"
#include <thread>
#include <chrono>
#include <QMetaObject>
#include <cmath>

CalibrationLogic::CalibrationLogic(Robot* robot, QObject* parent)
    : QObject(parent), robot(robot), connected(false), stepIndex(0), gripperOpen(false)
{
    // === Définition des étapes UI ===
    steps = {
        { "Videz les réservoirs, sauf un pion dans le réservoir de gauche à l'emplacement 1.",
         "./Ressources/image/Calibration/Etape1.png", true, false, false, false, false, false, false },
        { "Attrapez le pion du réservoir de gauche avec la pince du robot (elle doit être fermée).",
         "./Ressources/image/Calibration/Etape2.png", true, true, true, true, false, false, false },
        { "Amenez le pion à l'emplacement 4 du réservoir de gauche.",
         "./Ressources/image/Calibration/Etape3.png", true, true, true, true, false, false, false },
        { "Amenez le pion à l'emplacement 1 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape4.png", true, true, true, true, false, false, false },
        { "Amenez le pion à l'emplacement 4 du réservoir de droite.",
         "./Ressources/image/Calibration/Etape5.png", true, true, true, true, false, false, false },
        { "Amenez le pion à la colonne tout à gauche de la grille.",
         "./Ressources/image/Calibration/Etape6.png", true, true, true, true, false, false, false },
        { "Amenez le pion à la colonne tout à droite de la grille.",
         "./Ressources/image/Calibration/Etape7.png", true, true, true, true, false, false, false },
        { "Calibration terminée.<br>Vous pouvez maintenant tester les positions, recommencer ou quitter.",
         "./Ressources/image/welcome_calibration.png", false, false, false, false, true, true, true }
    };
}

// === Connexion / Initialisation ===
bool CalibrationLogic::connectToRobot() {
    if (!robot) return false;
    connected = robot->connect();
    emit connectionFinished(connected);
    return connected;
}

void CalibrationLogic::homeRobot() {
    if (!connected || !robot) return;

    std::thread([this]() {
        robot->Home();
        waitForRobotStable();
        QMetaObject::invokeMethod(this, [this]() { emit robotReady(); }, Qt::QueuedConnection);
    }).detach();
}

void CalibrationLogic::waitForRobotStable() {
    int stableCount = 0;
    while (true) {
        if (!robot->isMoving()) stableCount++;
        else stableCount = 0;
        if (stableCount >= 5) break;
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
    }
}

// === Calibration ===
void CalibrationLogic::startCalibration() {
    if (!connected) return;
    stepIndex = 0;
    calibrationData.clear();
    emit progressChanged(0);

    if (!steps.empty())
        emit stepChanged(steps[0], 0);
}

void CalibrationLogic::recordStep(int index) {
    if (!connected || !robot) return;

    Pose p;
    GetPose(&p);

    CalibrationStepData data;
    data.name = QString("Step_%1").arg(index + 1);
    data.pose = p;

    if (index >= static_cast<int>(calibrationData.size()))
        calibrationData.resize(index + 1);

    calibrationData[index] = data;
    emit progressChanged(index + 1);

    // 🔹 Si toutes les étapes manuelles sont enregistrées, calculer les autres
    if (index == 6) { // dernière étape manuelle
        computeAllPositions();
        emit calibrationFinished();
    }

    // Étape suivante (UI)
    stepIndex = index + 1;
    if (stepIndex < static_cast<int>(steps.size()))
        emit stepChanged(steps[stepIndex], stepIndex);
}

void CalibrationLogic::previousStep() {
    if (!connected) return;
    if (stepIndex <= 0) {
        stepIndex = 0;
        if (!steps.empty())
            emit stepChanged(steps[0], 0);
        emit progressChanged(0);
        return;
    }

    stepIndex--;
    emit progressChanged(stepIndex);
    if (stepIndex < static_cast<int>(steps.size()))
        emit stepChanged(steps[stepIndex], stepIndex);
}

void CalibrationLogic::resetCalibration() {
    calibrationData.clear();
    stepIndex = 0;
    emit progressChanged(0);
    if (!steps.empty())
        emit stepChanged(steps[0], 0);
}

// === Calculs automatiques ===
std::vector<Pose> CalibrationLogic::interpolatePoints(const Pose& start, const Pose& end, int count) {
    std::vector<Pose> points;
    if (count < 2) return points;

    for (int i = 0; i < count; ++i) {
        float t = static_cast<float>(i) / (count - 1);
        Pose p;
        p.x = start.x + (end.x - start.x) * t;
        p.y = start.y + (end.y - start.y) * t;
        p.z = start.z + (end.z - start.z) * t;
        p.r = start.r + (end.r - start.r) * t;
        points.push_back(p);
    }
    return points;
}

void CalibrationLogic::computeAllPositions() {
    if (calibrationData.size() < 7) return;

    // 🔹 Sauvegarder les points de référence avant tout
    Pose left1 = calibrationData[1].pose;
    Pose left4 = calibrationData[2].pose;
    Pose right1 = calibrationData[3].pose;
    Pose right4 = calibrationData[4].pose;
    Pose grid1 = calibrationData[5].pose;
    Pose grid7 = calibrationData[6].pose;

    // 🔹 Effacer et reconstruire proprement la liste complète
    calibrationData.clear();

    // 1️⃣ Réservoir gauche (2 points → 4)
    auto leftPoints = interpolatePoints(left1, left4, 4);
    for (int i = 0; i < 4; ++i) {
        CalibrationStepData d;
        d.name = QString("Left_%1").arg(i + 1);
        d.pose = leftPoints[i];
        calibrationData.push_back(d);
    }

    // 2️⃣ Réservoir droit (2 points → 4)
    auto rightPoints = interpolatePoints(right1, right4, 4);
    for (int i = 0; i < 4; ++i) {
        CalibrationStepData d;
        d.name = QString("Right_%1").arg(i + 1);
        d.pose = rightPoints[i];
        calibrationData.push_back(d);
    }

    // 3️⃣ Grille (2 points → 7)
    auto gridPoints = interpolatePoints(grid1, grid7, 7);
    for (int i = 0; i < 7; ++i) {
        CalibrationStepData d;
        d.name = QString("Grid_%1").arg(i + 1);
        d.pose = gridPoints[i];
        calibrationData.push_back(d);
    }

    // 🔹 Sauvegarde automatique de la calibration complète
    saveCalibration("./calibration.json");
}

// === Test des positions ===
void CalibrationLogic::testCalibration() {
    if (!connected || calibrationData.empty()) return;

    std::thread([this]() {
        int total = static_cast<int>(calibrationData.size());
        if (total == 0) return;

        qDebug() << "Nombre total de positions testées:" << total;

        for (int i = 0; i < total; ++i) {
            robot->goTo(calibrationData[i].pose);

            while (robot->isMoving()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(150));
            }

            // 🔹 Calcul précis du pourcentage (float → int)
            double ratio = double(i + 1) / double(total);
            int progress = std::clamp(static_cast<int>(ratio * 100.0), 0, 100);
            qDebug() << "Progression:" << progress;

            QMetaObject::invokeMethod(this, [this, progress]() {
                emit progressChanged(progress);
            }, Qt::QueuedConnection);

            std::this_thread::sleep_for(std::chrono::milliseconds(150));
        }


        // Fin du test
        QMetaObject::invokeMethod(this, [this]() {
            emit calibrationTestFinished();
            emit progressChanged(100);
        }, Qt::QueuedConnection);
    }).detach();
}


// === Manipulations manuelles ===
void CalibrationLogic::toggleGripper() {
    if (!connected || !robot) return;
    if (gripperOpen)
        robot->closeGripper();
    else
        robot->openGripper();
    gripperOpen = !gripperOpen;
}

void CalibrationLogic::rotateLeft() {
    if (!connected || !robot) return;
    robot->rotate(+5);
}

void CalibrationLogic::rotateRight() {
    if (!connected || !robot) return;
    robot->rotate(-5);
}

// === Sauvegarde / Chargement ===
void CalibrationLogic::saveCalibration(const QString& path) {
    QJsonArray arr;
    for (const auto& d : calibrationData) {
        QJsonObject o;
        o["name"] = d.name;
        o["x"] = d.pose.x;
        o["y"] = d.pose.y;
        o["z"] = d.pose.z;
        o["r"] = d.pose.r;
        arr.append(o);
    }

    QJsonObject root;
    root["steps"] = arr;

    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(QJsonDocument(root).toJson());
        f.close();
    }
}

void CalibrationLogic::loadCalibration(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly))
        return;

    QByteArray data = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject())
        return;

    QJsonArray arr = doc.object()["steps"].toArray();
    calibrationData.clear();
    for (const QJsonValue& v : arr) {
        QJsonObject o = v.toObject();
        CalibrationStepData d;
        d.name = o["name"].toString();
        d.pose.x = o["x"].toDouble();
        d.pose.y = o["y"].toDouble();
        d.pose.z = o["z"].toDouble();
        d.pose.r = o["r"].toDouble();
        calibrationData.push_back(d);
    }
}
