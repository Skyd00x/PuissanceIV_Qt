#pragma once

#include "Dobot/DobotDll.h"
#include "Dobot/DobotType.h"
#include <iostream>

class Robot
{
public:
    Robot();
    ~Robot();

    bool connect();
    void Home();
    void Play(int column);
    void Refill();
    int getRemainingPieces();

private:
    int remainingPieces = 8;
    Pose columnCoordinates[7];
    Pose pieceCoordinates[8];

    void goTo(Pose position);
    void goTo(Pose position, float z);
    void openGripper();
    void closeGripper();
    void grabPiece();
    void gripper(bool enable, bool grip);
    void turnOffGripper();
    void wait(float seconds);
};
