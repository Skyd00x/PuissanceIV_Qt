#include "MCTS.hpp"
#include "MonteCarlo.hpp"

namespace MCTS {

    // Node Constructor
    Node::Node(Board board, std::shared_ptr<Node> parent) : board(board), column(-1), parent(parent), N(0), Q(0), SelfTurn(parent ? !parent->SelfTurn : true), untried_actions{ 0, 1, 2, 3, 4, 5, 6 } {}

    // Determine whether it is fully expanded
    bool Node::is_all_expand()
    {
        if (untried_actions.empty())
            return true;

        std::vector<int> to_remove;  // Storing invalid columns
        for (int column : untried_actions)
        {
            if (board.isValidMove(column))
            {
                // 如果当前列是有效移动，删除所有记录的无效列
                // If the current column is valid move, delete all records with invalid columns
                for (int remove_column : to_remove)
                {
                    untried_actions.erase(
                        std::remove(untried_actions.begin(), untried_actions.end(), remove_column),
                        untried_actions.end()
                    );
                }
                return false;  // Find a valid move and return false directly. 找到有效移动，直接返回 false
            }
            // 将无效的列记录下来
            //Record the invalid columns
            to_remove.push_back(column);
        }

        // 删除所有无效的列
        // Remove all invalid columns
        for (int remove_column : to_remove)
        {
            untried_actions.erase(
                std::remove(untried_actions.begin(), untried_actions.end(), remove_column),
                untried_actions.end()
            );
        }

        // if anyone isn't valid, all expand
        return true;

    }

    // 判断是否是终结节点
    // Determine whether it is a terminal node
    bool Node::is_terminal_node()
    {
        return board.isTerminal();
    }

    // 添加子节点
    // Adding child nodes
    void Node::add_child(std::shared_ptr<Node> child_node)
    {
        children.push_back(child_node);
    }

    // GetBestMove (UCTsearch) 
    int GetBestMove(Board board, int times, int NbSimu, float ucb_constant)
    {
        auto root = make_shared<Node>(board, nullptr);// Create root node v0 based on initial state s0. 基于初态 s0 创建根节点 v0
        for (int i = 0; i < times; i++)
        {
            auto vl = TreePolicy(root, ucb_constant);   //selection
            //double delta = DefaultPolicy(vl->board, NbSimu);    // rollout
            double delta = DefaultPolicy_WithThread(vl->board, NbSimu);    // rollout
            BackUp(vl, delta);  // backup
        }
        auto best = BestChild(root, ucb_constant);  // return this node. 返回该节点

        //子节点
        for (int i = 0; i < root->children.size(); i++)
        {
            cout << root->children[i]->column << "\t";
        }
        cout << endl;

        ////子节点分数
        //for (int i = 0; i < root->children.size(); i++)
        //{
        //    cout << root->children[i]->Q << "\t";
        //}
        //cout << endl;

        ////次数
        //for (int i = 0; i < root->children.size(); i++)
        //{
        //    cout << root->children[i]->N << "\t";
        //}
        //cout << endl;

        ////Q/N
        //for (int i = 0; i < root->children.size(); i++)
        //{
        //    cout << (root->children[i]->Q) / (root->children[i]->N) << " ";
        //}
        //cout << endl;

        //UTC
        for (int i = 0; i < root->children.size(); i++)
        {
            cout << CaculatUCT(root->N, root->children[i]->N, root->children[i]->Q, ucb_constant) << " ";
        }
        cout << endl;

        int c = best->column;
        return c;
    }

    // Selection (TreePolicy)
    shared_ptr<Node> TreePolicy(shared_ptr<Node> v, float ucb_constant)
    {
        while (!v->is_terminal_node())
        {
            if (!v->is_all_expand())
            {
                return Expand(v);
            }
            else
            {
                v = BestChild(v, ucb_constant);
            }
        }
        return v;
    }

    // Expansion
    shared_ptr<Node> Expand(shared_ptr<Node> v)
    {
        if (v->untried_actions.empty())
        {
            return nullptr;  // Return directly when there is no scalable action. 无可扩展动作时直接返回
        }

        Board new_board = v->board; // Copy the current state
        int randomCol = -1;
        do
        {
            // randomCol = rand() % 7; // choice col random
            int randomIndex = rand() % v->untried_actions.size(); // Random Index. 随机索引
            randomCol = v->untried_actions[randomIndex];          // Get a random column. 获取随机列

        } while (!new_board.isValidMove(randomCol));    //until find valid move
        //this movement is valid and not be add in the children node yet

        new_board.Play(randomCol); // Play the move random
        auto v1 = make_shared<Node>(new_board, v);
        v1->column = randomCol;
        //v->untried_actions.push_back(randomCol);
        v->untried_actions.erase(remove(v->untried_actions.begin(), v->untried_actions.end(), randomCol), v->untried_actions.end());
        v->add_child(v1);
        return v1;
    }

    // Caculat UCT
    double CaculatUCT(int N, int n, double Q, float ucb_constant)
    {
        return (Q / n) + ucb_constant * sqrt(2 * log(N) / n);
    }

    // find BestChild
    shared_ptr<Node> BestChild(std::shared_ptr<Node> v, float ucb_constant) {
        return *max_element(v->children.begin(), v->children.end(),
            [ucb_constant, v](const shared_ptr<Node>& v1, const shared_ptr<Node>& v2)
            {
                double uct1 = CaculatUCT(v1->parent->N, v1->N, v1->Q, ucb_constant);
        double uct2 = CaculatUCT(v1->parent->N, v2->N, v2->Q, ucb_constant);
        return uct1 < uct2;
            });
    }

    // Simulation (DefaultPolicy)
    double DefaultPolicy(Board board, int NbSimu)
    {
        float score = 0;
        for (int i = 0; i < NbSimu; i++)
        {
            score += MonteCarlo::SimulateRandomPlay(board);
        }
        return  score;
    }

    double DefaultPolicy_WithThread(Board board, int NbSimu)
    {
        const int numThreads = 8;
        int simsPerThread = NbSimu / numThreads;
        int remainingSims = NbSimu % numThreads;  // There may be remaining simulation tasks when allocating. 分配时可能有剩余的模拟任务
        std::vector<std::future<double>> futures;

        // 创建8个线程，每个线程完成 simsPerThread 次模拟任务
        // Create 8 threads, each thread completes simsPerThread simulation tasks
        for (int i = 0; i < numThreads; i++)
        {
            // 如果有剩余的任务，分配到前几个线程
            // If there are remaining tasks, assign them to the first few threads
            int currentSimCount = simsPerThread + (i < remainingSims ? 1 : 0);

            futures.push_back(std::async(std::launch::async, [currentSimCount, &board]() {
                double local_score = 0;
            for (int j = 0; j < currentSimCount; j++)
            {
                local_score += MonteCarlo::SimulateRandomPlay(board);
            }
            return local_score;
                }));
        }

        // 汇总所有线程返回的分数
        // Sum the scores returned by all threads
        double totalScore = 0;
        for (auto& f : futures)
        {
            totalScore += f.get();
        }

        return totalScore;
    }

    // BackUp
    void BackUp(std::shared_ptr<Node> v, double delta)
    {
        if (v->SelfTurn)
            delta = -delta;

        while (v != nullptr)
        {
            v->N += 1;
            v->Q += delta;
            v = v->parent;
            delta = -delta;
        }
    }

} // namespace MCTS

