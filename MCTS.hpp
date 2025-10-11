#pragma once
#include <iostream>
#include "Board.hpp"
#include <vector>
#include <memory>
#include <cmath>
#include <algorithm>
#include <future>
#include <numeric> 
using namespace std;



namespace MCTS {

    /// <summary>
    /// Represents a node in the Monte Carlo Tree Search (MCTS) structure.
    /// Each node stores information about the game state, possible moves, and statistics for evaluating the best move.
    /// </summary>
    class Node
    {
    public:
        shared_ptr<Node> parent;                // Pointer to the parent node in the tree.
        vector<shared_ptr<Node>> children;      // Vector containing pointers to all child nodes.
        int N;                                  // The number of times this node has been visited.
        double Q;                               // The total score accumulated in simulations from this node.
        Board board;                            // The game board state associated with this node.
        vector<int> untried_actions;            // Vector containing columns that have not been tried yet.
        int column;         // The column index representing the move that led to this node.
        bool SelfTurn;      // Indicates whether it is the robot's turn in this node.

        /// <summary>
        /// Initializes a new instance of the <see cref="Node"/> class.
        /// </summary>
        /// <param name="board">The game board state associated with this node.</param>
        /// <param name="parent">The parent node of this node.</param>
        Node(Board board, shared_ptr<Node> parent);

        /// <summary>
        /// Checks if all possible child nodes have been expanded.
        /// </summary>
        /// <returns>True if all possible moves have been explored, otherwise false.</returns>
        bool is_all_expand();

        /// <summary>
        /// Determines whether this node represents a terminal state in the game.
        /// </summary>
        /// <returns>True if the game has ended at this node, otherwise false.</returns>
        bool is_terminal_node();

        /// <summary>
        /// Adds a child node to the list of children.
        /// </summary>
        /// <param name="child_node">The child node to be added.</param>
        void add_child(std::shared_ptr<Node> child_node);
    };

    /// <summary>
    /// Get the best move for the current player, using the Monte Carlo Tree Search algorithm.
    /// </summary>
    /// <param name="board"> Board to evaluate </param>
    /// <param name="times"> Times of Iterations </param>
    /// <param name="NbSimu"> Times of Simulation to get the score of this node </param>
    /// <param name="ucb_constant"> Hyperparameters ucb constant </param>
    /// <returns> The index of the column to play the best move </returns>
    int GetBestMove(Board board, int times, int NbSimu, float ucb_constant);

    /// <summary>
    /// Selection
    /// </summary>
    /// <param name="v"> Current Node </param>
    /// <param name="ucb_constant"> Hyperparameters ucb constant </param>
    /// <returns> Selected Node </returns>
    shared_ptr<Node> TreePolicy(std::shared_ptr<Node> v, float ucb_constant);

    /// <summary>
    /// Expansion
    /// </summary>
    /// <param name="v"> Current Node </param>
    /// <returns> Expanded Node </returns>
    shared_ptr<Node> Expand(std::shared_ptr<Node> v);

    /// <summary>
    /// It's part of selection. For choice the best child of current node according UCB
    /// </summary>
    /// <param name="v"> Current Node </param>
    /// <param name="ucb_constant"> Hyperparameters ucb constant </param>
    /// <returns> The best child Node of Current Node </returns>
    shared_ptr<Node> BestChild(std::shared_ptr<Node> v, float ucb_constant);

    /// <summary>
    /// Simulation
    /// </summary>
    /// <param name="board"> Board to evaluate </param>
    /// <param name="NbSimu"> Times of Simulation to get the score of this node </param>
    /// <returns> reward of this node </returns>
    double DefaultPolicy(Board board, int NbSimu);
    double DefaultPolicy_WithThread(Board board, int NbSimu);

    /// <summary>
    /// Back Propagation
    /// </summary>
    /// <param name="v"> Current Node </param>
    /// <param name="delta"> reward of this node </param>
    void BackUp(std::shared_ptr<Node> v, double delta);

    /// <summary>
    /// Caculate the value of UTC
    /// </summary>
    /// <param name="N"> The parcour times of parent node </param>
    /// <param name="n"> The parcour times of current node </param>
    /// <param name="Q"> The score of current node </param>
    /// <param name="ucb_constant"> Hyperparameters ucb constant </param>
    /// <returns> the value of UTC </returns>
    double CaculatUCT(int N, int n, double Q, float ucb_constant);

} // namespace MCTS
