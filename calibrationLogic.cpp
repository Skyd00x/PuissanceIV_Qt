#include "CalibrationLogic.hpp"
#include <thread>
#include <chrono>
#include <QMetaObject>

CalibrationLogic::CalibrationLogic(Robot* robot, QObject* parent)
    : QObject(parent), robot(robot), connected(false), stepIndex(0) {}

bool CalibrationLogic::connectToRobot() {
    if (!robot) return false;

    emit messageUpdated("Connexion au robot en cours...");
    connected = robot->connect();

    if (connected) {
        emit messageUpdated("Retour en position initiale du robot...");
        std::thread([this]() {
            robot->Home();
            waitForRobotStable();
            QMetaObject::invokeMethod(this, [this]() {
                emit connectionFinished(true);
                emit messageUpdated("Robot prêt. Cliquez sur 'Commencer la calibration'.");
            }, Qt::QueuedConnection);
        }).detach();
    } else {
        emit connectionFinished(false);
        emit messageUpdated("Échec de la connexion au robot.");
    }

    return connected;
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
    emit messageUpdated("Début de la calibration...");
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
}

void CalibrationLogic::testCalibration() {
    if (!connected || calibrationData.empty()) return;

    emit messageUpdated("Test des positions calibrées en cours...");
    int total = static_cast<int>(calibrationData.size());

    for (int i = 0; i < total; ++i) {
        robot->goTo(calibrationData[i].pose);
        emit progressChanged(static_cast<int>((float(i + 1) / total) * 100));
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
    emit messageUpdated("Test terminé !");
}

void CalibrationLogic::resetCalibration() {
    calibrationData.clear();
    stepIndex = 0;
    emit messageUpdated("Calibration réinitialisée.");
    emit progressChanged(0);
}

void CalibrationLogic::saveCalibration(const QString& path)
{
    QJsonArray arr;  // maintenant défini

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
        emit messageUpdated("Calibration sauvegardée dans " + path);
    } else {
        emit messageUpdated("Erreur : impossible d'écrire le fichier " + path);
    }
}

void CalibrationLogic::loadCalibration(const QString& path) {
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        emit messageUpdated("Aucune calibration enregistrée trouvée.");
        return;
    }

    QByteArray data = f.readAll();
    f.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit messageUpdated("Erreur : fichier de calibration invalide.");
        return;
    }

    QJsonObject root = doc.object();
    QJsonArray arr = root["steps"].toArray();  // ✅ ici aussi : QJsonArray

    calibrationData.clear();
    for (const QJsonValue& v : arr) {  // ✅ pas besoin d’auto ici
        QJsonObject o = v.toObject();
        CalibrationStepData d;
        d.name = o["name"].toString();
        d.pose.x = o["x"].toDouble();
        d.pose.y = o["y"].toDouble();
        d.pose.z = o["z"].toDouble();
        d.pose.r = o["r"].toDouble();
        calibrationData.push_back(d);
    }

    emit messageUpdated(QString("Calibration chargée (%1 positions)").arg(calibrationData.size()));
}
