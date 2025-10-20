#include "CalibrationLogic.hpp"
#include <thread>
#include <chrono>
#include <QMetaObject>

CalibrationLogic::CalibrationLogic(Robot* robot, QObject* parent)
    : QObject(parent), robot(robot), connected(false), stepIndex(0), gripperOpen(false)
{
    // === Initialisation des étapes UI ===
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

        QMetaObject::invokeMethod(this, [this]() {
            emit robotReady();
        }, Qt::QueuedConnection);
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

void CalibrationLogic::startCalibration() {
    if (!connected) return;
    stepIndex = 0;
    calibrationData.clear();
    emit progressChanged(0);

    // 🔹 Envoie la première étape à l’UI
    if (!steps.empty())
        emit stepChanged(steps[0], 0);
}

void CalibrationLogic::recordStep(int index) {
    if (!connected || !robot) return;
    Pose p;
    GetPose(&p);
    CalibrationStepData data;
    data.name = QString("Step %1").arg(index + 1);
    data.pose = p;

    if (index >= static_cast<int>(calibrationData.size()))
        calibrationData.resize(index + 1);

    calibrationData[index] = data;
    emit progressChanged(index + 1);

    // 🔹 Étape suivante
    stepIndex = index + 1;
    if (stepIndex < static_cast<int>(steps.size()))
        emit stepChanged(steps[stepIndex], stepIndex);

    // ✅ Si on vient de finir la dernière étape, on signale la fin
    if (stepIndex >= static_cast<int>(steps.size()) - 1) {
        emit calibrationFinished();  // on émet un signal
    }
}

void CalibrationLogic::previousStep() {
    if (!connected) return;
    if (stepIndex <= 0) {
        stepIndex = 0;
        // Rester sur la première étape
        if (!steps.empty())
            emit stepChanged(steps[0], 0);
        emit progressChanged(0);
        return;
    }

    // Reculer d'une étape
    stepIndex--;

    // Mettre à jour la progression (barre 0..7)
    emit progressChanged(stepIndex);

    // Pousser l'étape correspondante à l'UI
    if (stepIndex < static_cast<int>(steps.size()))
        emit stepChanged(steps[stepIndex], stepIndex);
}

void CalibrationLogic::testCalibration() {
    if (!connected || calibrationData.empty()) return;

    std::thread([this]() {
        int total = static_cast<int>(calibrationData.size());
        if (total == 0) return;

        for (int i = 0; i < total; ++i) {
            // Aller à la position enregistrée
            robot->goTo(calibrationData[i].pose);

            // Attendre que le robot ait fini de bouger
            while (robot->isMoving()) {
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }

            // Mise à jour de la progression (0–100%)
            int progress = static_cast<int>((float(i + 1) / total) * 100);
            QMetaObject::invokeMethod(this, [this, progress]() {
                emit progressChanged(progress);
            }, Qt::QueuedConnection);

            // Petite pause visuelle entre chaque position
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
        }

        // ✅ Fin du test
        QMetaObject::invokeMethod(this, [this]() {
            emit calibrationTestFinished();
        }, Qt::QueuedConnection);
    }).detach();
}

void CalibrationLogic::resetCalibration() {
    calibrationData.clear();
    stepIndex = 0;
    emit progressChanged(0);

    // 🔹 Retour à la première étape
    if (!steps.empty())
        emit stepChanged(steps[0], 0);
}

// === Manipulations robot ===
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
