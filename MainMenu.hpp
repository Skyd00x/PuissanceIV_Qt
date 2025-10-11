#pragma once

#include <QWidget>
#include <QPushButton>
#include <QLineEdit>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPixmap>
#include "StateMachine.hpp"

/// <summary>
/// Menu principal du jeu (Qt version)
/// </summary>
class MainMenu : public QWidget
{
    Q_OBJECT

public:
    explicit MainMenu(QFont *fontButton = nullptr, QFont *fontTextbox = nullptr, QWidget *parent = nullptr);

signals:
    void playClicked();
    void difficultyChanged(StateMachine::Difficulty difficulty,
                           float param1, float param2, float param3);

private slots:
    void onPlay();
    void onEasy();
    void onMedium();
    void onHard();

private:
    QPushButton *playButton;
    QPushButton *easyButton;
    QPushButton *mediumButton;
    QPushButton *hardButton;

    QLineEdit *easyParamBox;
    QLineEdit *mediumParamBox;
    QLineEdit *hardParamBox1;
    QLineEdit *hardParamBox2;
    QLineEdit *hardParamBox3;

    QLabel *logoLabel;
    QLabel *hardLabels[3];

    QFont buttonFont;
    QFont textboxFont;

    void resetDifficultyColors();
};
