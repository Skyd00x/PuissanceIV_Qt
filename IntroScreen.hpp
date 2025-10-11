#pragma once

#include <QWidget>
#include <QLabel>
#include <QTimer>
#include <QGraphicsOpacityEffect>
#include <QPropertyAnimation>
#include <QVBoxLayout>

class IntroScreen : public QWidget
{
    Q_OBJECT

public:
    explicit IntroScreen(QWidget *parent = nullptr);
    void start(); // Lance l’animation

signals:
    void introFinished(); // Signal émis à la fin du fondu

private:
    QLabel* titleLabel;
    QLabel* logoLabel;
    QLabel* copyrightLabel;
    QGraphicsOpacityEffect* opacityEffect;
    QPropertyAnimation* fadeInAnim;
    QPropertyAnimation* fadeOutAnim;
    QTimer* holdTimer;
};
