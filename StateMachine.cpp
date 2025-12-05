#include "StateMachine.hpp"
#include <iostream>

StateMachine::StateMachine()
{
    state = State::MainMenu;
    difficulty = Difficulty::Medium;
    playerColor = PlayerColor::Red;  // Par défaut, le joueur joue avec les rouges
}

void StateMachine::ChangeState(State newState)
{
    state = newState;
    std::cout << "State changed to " << state << std::endl;
}

void StateMachine::setDifficulty(Difficulty newDifficulty)
{
    difficulty = newDifficulty;

    // === Paramètres internes définis selon la difficulté ===
    switch (newDifficulty)
    {
    case Difficulty::Easy:
        // Profondeur réduite (exemple pour minimax)
        Param1 = 6;     // profondeur
        Param2 = 0;
        Param3 = 0;
        std::cout << "Difficulté : Facile (profondeur = " << Param1 << ")\n";
        break;

    case Difficulty::Medium:
        // Négamax avec itérations moyennes
        Param1 = 30000; // itérations
        Param2 = 0;
        Param3 = 0;
        std::cout << "Difficulté : Moyenne (itérations = " << Param1 << ")\n";
        break;

    case Difficulty::Hard:
        // MCTS avec paramètres classiques
        Param1 = 1000;  // itérations
        Param2 = 50;    // simulations
        Param3 = 2.5f;  // constante UCB
        std::cout << "Difficulté : Difficile (it=" << Param1 << ", sim=" << Param2 << ", ucb=" << Param3 << ")\n";
        break;
    }

    std::cout << "Difficulty changed to " << newDifficulty << std::endl;
}

void StateMachine::setPlayerColor(PlayerColor newColor)
{
    playerColor = newColor;
    std::cout << "Player color changed to " << (newColor == PlayerColor::Red ? "Red" : "Yellow") << std::endl;
}

float StateMachine::getParam1() const { return Param1; }
float StateMachine::getParam2() const { return Param2; }
float StateMachine::getParam3() const { return Param3; }
