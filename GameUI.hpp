#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QImage>
#include <QTimer>
#include <QPainter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include <opencv2/opencv.hpp>
#include "StateMachine.hpp"
#include "Camera.hpp"
#include "Board.hpp"
#include "Robot.hpp"

class GameUI : public QWidget {
    Q_OBJECT

public:
    explicit GameUI(Robot* robot, QWidget *parent = nullptr);
    ~GameUI();

    void updateBoard(const Board& board);
    void playerVictory();
    void playerDefeat();
    void resetBoard();

public slots:
    void updateCameraFrame(const QImage &image);

signals:
    void backClicked();
    void restartClicked();
    void refillClicked();

private slots:
    void onBackButton();
    void onRestartButton();
    void onRefillButton();

    void usePiece();
    int getRemainingPieces() const { return remainingPieces; }

private:
    int remainingPieces = 8;

    QLabel *cameraLabel;
    QLabel *victoryLabel;
    QLabel *logoPolytech;

    QPushButton *backButton;
    QPushButton *restartButton;
    QPushButton *refillButton;

    QPixmap redTexture, yellowTexture, logoTexture;

    Robot* robot;
    Board currentBoard;

    void paintEvent(QPaintEvent *event) override;
};
