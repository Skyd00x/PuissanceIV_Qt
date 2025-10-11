#pragma once
#include <iostream>
#include "Board.hpp"
using namespace std;


namespace MonteCarlo
{
	/// <summary>
	/// Get the best move for the current player, using the MonteCarlo Simulation algorithm.
	/// </summary>
	/// <param name="board"> Board to evaluate </param>
	/// <param name="times"> Times of simulation </param>
	/// <returns> The index of the column to play the best move </returns>
	int GetBestMove(Board board, int times);

	/// <summary>
	/// Get the best move for the current player, using the MonteCarlo Simulation algorithm with thread.
	/// </summary>
	/// <param name="board"> Board to evaluate </param>
	/// <param name="times"> Times of simulation </param>
	/// <returns> The index of the column to play the best move </returns>
	int GetBestMove_WithThread(Board board, int times);

	/// <summary>
	/// Single simulation, until the game is over
	/// </summary>
	/// <param name="board"> Board to evaluate </param>
	/// <returns> the score of simulation </returns>
	float SimulateRandomPlay(Board board);
};

