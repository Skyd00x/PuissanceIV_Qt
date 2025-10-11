#include "StateMachine.hpp"

StateMachine::StateMachine()
{
	state = State::MainMenu;
	difficulty = Difficulty::Medium;
}

void StateMachine::ChangeState(State newState)
{
	state = newState;
	std::cout << "State changed to " << state << std::endl;
}

void StateMachine::setDifficulty(Difficulty newDifficulty, float P1, float P2, float P3)
{
	difficulty = newDifficulty;
	std::cout << "difficulty changed to " << difficulty << std::endl;
	Param1 = P1;
	Param2 = P2;
	Param3 = P3;
}

float StateMachine::getParam1()
{
	return Param1;
}

float StateMachine::getParam2()
{
	return Param2;
}

float StateMachine::getParam3()
{
	return Param3;
}