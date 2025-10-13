#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class CalibrationScreen : public QWidget
{
    Q_OBJECT
public:
    explicit CalibrationScreen(QWidget *parent = nullptr);

signals:
    void backToMenu();
};
