#include "Negamax.hpp"
#include <algorithm>

namespace SimpleAI
{
// ---------------------------------------------------------
// Vérifie si la colonne est jouable
// ---------------------------------------------------------
bool isValidMove(const Grid& grid, int col)
{
    if (col < 0 || col >= 7) return false;
    return (grid[0][col] == 0);
}

// ---------------------------------------------------------
// Joue un coup dans une copie de la grille
// ---------------------------------------------------------
Grid playMove(const Grid& grid, int col, int player)
{
    Grid g = grid;
    for (int r = 5; r >= 0; r--)
    {
        if (g[r][col] == 0)
        {
            g[r][col] = player;
            break;
        }
    }
    return g;
}

// ---------------------------------------------------------
// Détection alignement de 4
// ---------------------------------------------------------
bool isWinningMove(const Grid& g, int player)
{
    // Horizontal
    for (int r = 0; r < 6; r++)
    {
        for (int c = 0; c < 4; c++)
        {
            if (g[r][c] == player &&
                g[r][c + 1] == player &&
                g[r][c + 2] == player &&
                g[r][c + 3] == player)
                return true;
        }
    }

    // Vertical
    for (int c = 0; c < 7; c++)
    {
        for (int r = 0; r < 3; r++)
        {
            if (g[r][c] == player &&
                g[r + 1][c] == player &&
                g[r + 2][c] == player &&
                g[r + 3][c] == player)
                return true;
        }
    }

    // Diagonale /
    for (int r = 3; r < 6; r++)
    {
        for (int c = 0; c < 4; c++)
        {
            if (g[r][c] == player &&
                g[r - 1][c + 1] == player &&
                g[r - 2][c + 2] == player &&
                g[r - 3][c + 3] == player)
                return true;
        }
    }

    // Diagonale
    for (int r = 3; r < 6; r++)
    {
        for (int c = 3; c < 7; c++)
        {
            if (g[r][c] == player &&
                g[r - 1][c - 1] == player &&
                g[r - 2][c - 2] == player &&
                g[r - 3][c - 3] == player)
                return true;
        }
    }

    return false;
}

// ---------------------------------------------------------
// Évaluation simple (plus rapide que la tienne)
// ---------------------------------------------------------
int evaluate(const Grid& grid)
{
    if (isWinningMove(grid, 2)) return +10000;  // robot gagne
    if (isWinningMove(grid, 1)) return -10000;  // joueur gagne
    return 0;
}

// ---------------------------------------------------------
// Negamax récursif
// player = 1 (humain) ou 2 (robot)
// ---------------------------------------------------------
int negamax(const Grid& grid, int depth, int alpha, int beta, int player)
{
    int eval = evaluate(grid);
    if (depth == 0 || eval != 0)
        return (player == 2 ? eval : -eval);

    int best = -999999;

    for (int col = 0; col < 7; col++)
    {
        if (!isValidMove(grid, col))
            continue;

        Grid child = playMove(grid, col, player);

        int score = -negamax(child, depth - 1, -beta, -alpha, (player == 2 ? 1 : 2));

        best = std::max(best, score);
        alpha = std::max(alpha, score);

        if (alpha >= beta)
            break;
    }

    return best;
}

// ---------------------------------------------------------
// Recherche du meilleur coup
// ---------------------------------------------------------
int getBestMove(const Grid& grid, int depth)
{
    int bestCol = 3;      // centre par défaut
    int bestVal = -999999;

    for (int col = 0; col < 7; col++)
    {
        if (!isValidMove(grid, col))
            continue;

        Grid child = playMove(grid, col, 2); // robot = 2
        int val = -negamax(child, depth - 1, -100000, 100000, 1);

        if (val > bestVal)
        {
            bestVal = val;
            bestCol = col;
        }
    }

    return bestCol;
}
}
