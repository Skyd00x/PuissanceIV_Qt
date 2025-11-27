#pragma once
#include <iostream>

class StateMachine
{
public:
    enum State {
        Intro,
        CheckDevices,
        MainMenu,
        Game,
        Calibration,
        Explanation,
        Options,
        Quit
    };

    enum Difficulty {
        Easy,
        Medium,
        Hard,
        Impossible
    };

    enum PlayerColor {
        Red = 1,    // Le joueur joue avec les pions rouges
        Yellow = 2  // Le joueur joue avec les pions jaunes
    };

    StateMachine();

    void ChangeState(State newState);
    void setDifficulty(Difficulty newDifficulty);
    void setPlayerColor(PlayerColor newColor);

    State getState() const { return state; }
    Difficulty getDifficulty() const { return difficulty; }
    PlayerColor getPlayerColor() const { return playerColor; }
    int getPlayerColorValue() const { return static_cast<int>(playerColor); }
    int getRobotColorValue() const { return (playerColor == PlayerColor::Red) ? 2 : 1; }

    bool isState(State stateToCompare) const { return state == stateToCompare; }

    float getParam1() const;
    float getParam2() const;
    float getParam3() const;

private:
    State state;
    Difficulty difficulty;
    PlayerColor playerColor;

    // Paramètres internes (par ex. profondeur, itérations, etc.)
    float Param1 = 0;
    float Param2 = 0;
    float Param3 = 0;
};
