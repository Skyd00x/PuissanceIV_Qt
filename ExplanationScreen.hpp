#pragma once
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>

class ExplanationScreen : public QWidget
{
    Q_OBJECT
public:
    explicit ExplanationScreen(QWidget *parent = nullptr);

signals:
    void backToMenu();
};
