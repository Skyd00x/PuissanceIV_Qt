#pragma once
#include <iostream>

class StateMachine
{
public:
	enum State {
		MainMenu,
		Game,
		Options,
		Quit
	};

	enum Difficulty {
		Easy,
		Medium,
		Hard
	};

	StateMachine();

	void ChangeState(State newState);
	void setDifficulty(Difficulty newDifficulty, float Param1, float Param2, float Param3);

	State getState() { return state; }
	Difficulty getDifficulty() const { return difficulty; }

	bool isState(State stateToCompare) { return state == stateToCompare; }

	float getParam1();
	float getParam2();
	float getParam3();

private:
	State state;
	Difficulty difficulty;

	// the param of algo
	float Param1 = 0;
	float Param2 = 0;
	float Param3 = 0;


};

