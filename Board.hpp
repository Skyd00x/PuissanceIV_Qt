#pragma once

#include <exception>
#include <iostream>

class Board
{
public:
	/// <summary>
	/// Default constructor
	/// </summary>
	Board();

	/// <summary>
	/// Copy a board to a new board object
	/// </summary>
	/// <returns>The new board object</returns>
	Board copy();

	/// <summary>
	/// Play a move in the corresponding column
	/// </summary>
	/// <param name="column">Column where the piece will be played</param>
	void Play(int column);

	/// <summary>
	/// Check if the game is over (Win or Draw)
	/// </summary>
	/// <returns>True if the game is over, false otherwise</returns>
	bool isTerminal();

	/// <summary>
	/// Check if the player wins
	/// </summary>
	/// <returns>True if the player wins, false otherwise</returns>
	bool playerWins();

	/// <summary>
	/// Check if the robot wins
	/// </summary>
	/// <returns>True if the robot wins, false otherwise</returns>
	bool robotWins();

	/// <summary>
	/// Check if the game is a draw
	/// </summary>
	/// <returns>True if the game is a draw, false otherwise</returns>
	bool draw();

	/// <summary>
	/// Check if a move is valid
	/// </summary>
	/// <param name="column">Column to check</param>
	/// <returns>True if the move is valid, false otherwise</returns>
	bool isValidMove(int column);

	/// <summary>
	/// Get the number of moves played on the board
	/// </summary>
	/// <returns>The number of moves played</returns>
	int getMoveNumber();

	/// <summary>
	/// Print the board in the console
	/// </summary>
	void printBoard();

	/// <summary>
	/// Set the player piece at the given position (empty the position if value is false)
	/// </summary>
	void setPlayerPiece(int column, int row, bool value);

	/// <summary>
	/// Set the robot piece at the given position (empty the position if value is false)
	/// </summary>
	void setRobotPiece(int column, int row, bool value);

	/// <summary>
	/// Check if the board is empty
	/// </summary>
	/// <returns>True if the board is empty, false otherwise</returns>
	bool isEmpty();

	/// <summary>
	/// Get the player bitboard as an unsigned __int64
	/// </summary>
	/// <returns>The player bitboard</returns>
	unsigned __int64 getPlayerBitboard();

	/// <summary>
	/// Get the robot bitboard as an unsigned __int64
	/// </summary>
	/// <returns>The robot bitboard</returns>
	unsigned __int64 getRobotBitboard();

	/// <summary>
	/// Check if the board is valid
	/// </summary>
	/// <returns>True if every piece has another piece below, false otherwise</returns>
	bool isValid();

	/// <summary>
	/// Get the piece at the given position
	/// </summary>
	/// <returns>1 if the player has a piece, 2 if the robot has a piece, 0 otherwise</returns>
	int getPiece(int column, int row);

private:
	unsigned __int64 playerBoard;
	unsigned __int64 robotBoard;
	unsigned int moveNumber;

	/// <summary>
	/// Check if a piece is present at the given position
	/// </summary>
	/// <param name="column">Index of the column</param>
	/// <param name="row">Index of the row</param>
	/// <param name="board">Bitboard to check</param>
	/// <returns>True if a piece is present, false otherwise</returns>
	bool getPiece(int column, int row, unsigned __int64 board);

	/// <summary>
	/// Set a piece at the given position
	/// </summary>
	/// <param name="column">Index of the column</param>
	/// <param name="row">Index of the row</param>
	/// <param name="board">Bitboard to modify</param>
	/// <param name="value">True to set a piece, false to empty the position</param>
	void setPiece(int column, int row, unsigned __int64* board, bool value);

	/// <summary>
	/// Check if 4 pieces are aligned
	/// </summary>
	/// <param name="board">Bitboard to check</param>
	/// <returns>True if 4 pieces are aligned, false otherwise</returns>
	bool checkWin(unsigned __int64 board);

public:

	/// <summary>
	/// A faster way to check if a 4 pieces are aligned, but not working for all cases
	/// Keep it to improve the speed of the game in the future
	/// </summary>
	/// <param name="board">Bitboard to check</param>
	/// <returns>True if 4 pieces are aligned, false otherwise</returns>
	bool checkWinFast(unsigned __int64 board);

	bool moveIsWinning(int column);
};

