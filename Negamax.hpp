#pragma once

#include <QVector>
#include "CameraAi.hpp"

namespace SimpleAI
{
using Grid = CameraAI::Grid;

// Retourne la meilleure colonne à jouer
int getBestMove(const Grid& grid, int depth);

// Optionnel : évalue une grille
int evaluate(const Grid& grid);

// Vérifie si un joueur gagne
bool isWinningMove(const Grid& grid, int player);

// Vérifie si un coup est jouable
bool isValidMove(const Grid& grid, int col);

// Joue un coup dans une copie de la grille
Grid playMove(const Grid& grid, int col, int player);

// Negamax avec élagage alpha-beta
int negamax(const Grid& grid, int depth, int alpha, int beta, int player);
}
