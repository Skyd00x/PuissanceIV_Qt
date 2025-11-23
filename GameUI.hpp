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
#include "Robot.hpp"

class GameUI : public QWidget {
    Q_OBJECT

public:
    explicit GameUI();
    ~GameUI();
};
