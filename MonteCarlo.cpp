#include "MonteCarlo.hpp"
#include <vector>
#include <limits>
#include <cstdlib> // include rand and srand
#include <ctime>   // include time
#include <thread>
#include <limits>
#include <mutex>

mutex mtx;  // 互斥锁用于保护共享资源
const int ROWS = 6;
const int COLS = 7;

int MonteCarlo::GetBestMove(Board board, int times)
{
    std::vector<int> scores(COLS, 0);
    std::vector<float> wins(COLS, 0); // store win times of every col

    for (int col = 0; col < COLS; col++)
    {
        if (board.isValidMove(col))
        {
            for (int i = 0; i < times; i++)
            {
                Board boardCopy = board; // copy coard
                boardCopy.Play(col); // move

                // simulation
                int result = MonteCarlo::SimulateRandomPlay(boardCopy);
                if (result == 1) // if robot win
                {
                    wins[col]++;
                }
                scores[col] += result; // add result
            }
        }
    }

    // find the best move
    int bestMove = -1;
    int maxScore = std::numeric_limits<int>::min(); // initialisation at min value
    for (int col = 0; col < COLS; col++)
    {
        if (board.isValidMove(col) && scores[col] > maxScore)
        {
            maxScore = scores[col];
            bestMove = col;
        }
    }

    for (size_t i = 0; i < wins.size(); ++i) {
        cout << wins[i] << " ";
    }
    cout << endl;
    for (size_t i = 0; i < wins.size(); ++i) {
        cout << wins[i] / times * 100 << "% ";
    }
    cout << endl;


    return bestMove;
}

int MonteCarlo::GetBestMove_WithThread(Board board, int times)
{
    std::vector<int> scores(COLS, 0);
    std::vector<float> wins(COLS, 0); // 存储每列的胜利次数

    // 创建线程数组
    std::vector<std::thread> threads(COLS);

    // 为每列计算得分和胜利次数
    for (int col = 0; col < COLS; col++)
    {
        if (board.isValidMove(col))
        {
            // 创建线程，使用lambda表达式捕获变量
            threads[col] = std::thread([&, col]()
                {
                    srand(static_cast<unsigned int>(time(0)));
            for (int i = 0; i < times; i++)
            {
                Board boardCopy = board; // 复制棋盘
                boardCopy.Play(col); // 下棋

                // 模拟
                int result = MonteCarlo::SimulateRandomPlay(boardCopy);

                // 保护共享资源
                std::lock_guard<std::mutex> lock(mtx);
                if (result == 1) // 如果机器人获胜
                {
                    wins[col]++;
                }
                scores[col] += result; // 添加结果
            }
                });
        }
    }

    // 等待所有线程完成
    for (int col = 0; col < COLS; col++)
    {
        if (threads[col].joinable())
        {
            threads[col].join();
        }
    }

    // 查找最佳移动
    int bestMove = -1;
    int maxScore = numeric_limits<int>::min(); // 初始化为最小值
    for (int col = 0; col < COLS; col++)
    {
        if (board.isValidMove(col) && scores[col] > maxScore)
        {
            maxScore = scores[col];
            bestMove = col;
        }
    }

    // 打印胜利次数和胜率
    for (size_t i = 0; i < wins.size(); ++i) {
        std::cout << wins[i] << " ";
    }
    std::cout << std::endl;

    for (size_t i = 0; i < wins.size(); ++i) {
        std::cout << (wins[i] / times) * 100 << "% ";
    }
    std::cout << std::endl;

    return bestMove;
}

float MonteCarlo::SimulateRandomPlay(Board board)
{
    float score = 0;

    while (!board.isTerminal())
    {

        int randomCol = rand() % COLS; // choice col random
        if (board.isValidMove(randomCol))
        {
            board.Play(randomCol); // move random
        }

    }

    // judge result
    if (board.robotWins())
    {
        return 1 + score; // robot win
    }
    else if (board.playerWins())
    {
        return -1 + score; // player win
    }
    else if (board.draw())
    {
        return 0 + score; // draw
    }

    throw std::exception("Not Terminal");
    return 0;
}

