#include "TranspositionTable.hpp"
/*
TranspositionTable::TranspositionTable()
{
	transpositionTable.resize(size);
}

TranspositionTable::~TranspositionTable()
{
		transpositionTable.clear();
}

void TranspositionTable::put(Board board, uint8_t value)
{
	uint64_t key = getKey(board);
	unsigned int i = index(key);
	transpositionTable[i].key = key;
	transpositionTable[i].value = value;
}

int TranspositionTable::get(Board board)
{
	uint64_t key = getKey(board);
	unsigned int i = index(key);
	if (transpositionTable[i].key == key)
	{
		return transpositionTable[i].value;
	}
	else
	{
		return 0;
	}
}

bool TranspositionTable::contains(Board board)
{
	uint64_t key = getKey(board);
	unsigned int i = index(key);
	if (transpositionTable[i].key == key)
	{
		return true;
	}
	return false;
}

unsigned int TranspositionTable::index(uint64_t key)
{
	return key % transpositionTable.size();
}

uint64_t TranspositionTable::getKey(Board board)
{
	if (board.getMoveNumber() % 2 == 0)
	{
		return board.getRobotBitboard();
	}
	else
	{
		return board.getPlayerBitboard();
	}
}
*/

