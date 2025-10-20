#include "CalibrationLogic.hpp"
#include <thread>
#include <chrono>
#include <QMetaObject>

CalibrationLogic::CalibrationLogic(Robot* robot, QObject* parent)
    : QObject(parent), robot(robot), connected(false), stepIndex(0), gripperOpen(false) {}

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

    int total = static_cast<int>(calibrationData.size());
    for (int i = 0; i < total; ++i) {
        robot->goTo(calibrationData[i].pose);
        emit progressChanged(static_cast<int>((float(i + 1) / total) * 100));
        std::this_thread::sleep_for(std::chrono::milliseconds(400));
    }
}

void CalibrationLogic::resetCalibration() {
    calibrationData.clear();
    stepIndex = 0;
    emit progressChanged(0);
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
