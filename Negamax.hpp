#pragma once

#include <thread>
#include <iostream>
#include "TranspositionTable.hpp"

namespace Negamax
{
    /*

	/// <summary>
	/// Get the best move for the current player, using the negamax algorithm.
	/// </summary>
	/// <param name="board">Board to evaluate</param>
	/// <param name="transpositionTable">Transposition table to use to store already evaluated boards</param>
	/// <param name="depth">Depth of the search</param>
	/// <returns>The index of the column to play the best move</returns>
	int GetBestMove(Board board, TranspositionTable* transpositionTable, unsigned int depth);


	int GetBestMove_noThreads(Board board, TranspositionTable* transpositionTable, unsigned int depth);

	/// <summary>
	/// Evaluate a Terminal board by giving it a score corresponding to the number of pieces needed to win for the current player.
	/// </summary>
	/// <param name="board">Board to evaluate</param>
	/// <returns>Score of the board</returns>
	int Evaluate(Board terminalBoard);

	/// <summary>
	/// Recursive function to find the best move for the current player.
	/// </summary>
	/// <param name="board">Board to evaluate</param>
	/// <param name="alpha">Alpha-beta pruning parameter</param>
	/// <param name="beta">Alpha-beta pruning parameter</param>
	/// <param name="transpositionTable">Transposition table to use to store already evaluated boards</param>
	/// <param name="depth">Depth of the search</param>
	/// <returns>The score of the board</returns>
	int Negamax(Board board, int alpha, int beta, TranspositionTable* transpositionTable, unsigned int depth);

	/// <summary>
	/// Launche the threads running the negamax function. One thread is created for each column
	/// </summary>
	/// <param name="board">Board to evaluate</param>
	/// <param name="result">Int array to store each column result from each thread</param>
	/// <param name="transpositionTable"></param>
	/// <param name="depth"></param>
	void NegamaxThread(Board board, int* result, TranspositionTable* transpositionTable, unsigned int depth);

	int compareColumnOrder(int a, int b);

	int GetBestMoveEarlyGame(Board board);
*/
}
