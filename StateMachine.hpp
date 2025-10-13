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

    StateMachine();

    void ChangeState(State newState);
    void setDifficulty(Difficulty newDifficulty);

    State getState() const { return state; }
    Difficulty getDifficulty() const { return difficulty; }

    bool isState(State stateToCompare) const { return state == stateToCompare; }

    float getParam1() const;
    float getParam2() const;
    float getParam3() const;

private:
    State state;
    Difficulty difficulty;

    // Paramètres internes (par ex. profondeur, itérations, etc.)
    float Param1 = 0;
    float Param2 = 0;
    float Param3 = 0;
};
