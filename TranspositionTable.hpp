#pragma once

#include <vector>
#include "Board.hpp"

class TranspositionTable
{
	/// <summary>
	/// Transposition table element, storing the board in a 56 bit array, and the evaluated value of the board
	/// </summary>
	struct Entry
	{
		uint64_t key: 56;
		uint8_t value;
	};

public:
	/// <summary>
	/// Allocate the memory for storing the values
	/// </summary>
	TranspositionTable();

	/// <summary>
	/// Free the memory allocated
	/// </summary>
	~TranspositionTable();

	/// <summary>
	/// Store a board and its value in the trasposition table
	/// </summary>
	/// <param name="board">Board evaluated</param>
	/// <param name="value">Value of the board</param>
	void put(Board board, uint8_t value);

	/// <summary>
	/// Get the value associated with a board, stored in the transposition table
	/// </summary>
	/// <param name="board">Board to evaluate</param>
	/// <returns>Value of the given board</returns>
	int get(Board board);

	unsigned long long size = 40000; // 80000000 is the maximum size
	
	/// <summary>
	/// Check if a board is already in the transposition table
	/// </summary>
	/// <param name="board">Board to evaluate</param>
	/// <returns>True if the board is in the transposition table, False otherwise</returns>
	bool contains(Board board);

private:
	std::vector<Entry> transpositionTable;

	/// <summary>
	/// Get the index of a given key. The key is a board stored in bit array form
	/// </summary>
	/// <param name="key"></param>
	/// <returns>The index of the given key</returns>
	unsigned int index(uint64_t key);

	/// <summary>
	/// Get the corresponding key according to a board. If it is the player turn, give the player bitboard,
	/// if it is the robot turn, give the robot bitboard
	/// </summary>
	/// <param name="board">Board get the key</param>
	/// <returns>Bitboard key corresponding to a Board object</returns>
	uint64_t getKey(Board board);
};

