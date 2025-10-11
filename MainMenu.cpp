#include "MainMenu.hpp"
#include <QSpacerItem>
#include <QFontDatabase>
#include <QDebug>

MainMenu::MainMenu(QFont *fontButton, QFont *fontTextbox, QWidget *parent)
    : QWidget(parent)
{
    setStyleSheet("background-color: #BEE7FB;");

    // === Polices ===
    if (fontButton) buttonFont = *fontButton;
    else buttonFont = QFont("Arial", 20);

    if (fontTextbox) textboxFont = *fontTextbox;
    else textboxFont = QFont("Arial", 14);

    // === Bouton Jouer ===
    playButton = new QPushButton("Jouer");
    playButton->setFont(buttonFont);
    playButton->setStyleSheet("background-color: #6895BE; color: white; font-size: 32px; padding: 12px; border-radius: 15px;");

    // === Boutons de difficulté ===
    easyButton = new QPushButton("Facile");
    mediumButton = new QPushButton("Moyen");
    hardButton = new QPushButton("Difficile");

    QList<QPushButton*> diffButtons = {easyButton, mediumButton, hardButton};
    QList<QString> colors = {"#A0C882", "#F09C42", "#EC776D"};
    for (int i = 0; i < diffButtons.size(); ++i) {
        diffButtons[i]->setFont(buttonFont);
        diffButtons[i]->setStyleSheet(QString(
                                          "background-color: %1; color: white; font-size: 20px; border-radius: 10px; padding: 6px;"
                                          ).arg(colors[i]));
    }

    // === Zones de paramètre ===
    easyParamBox = new QLineEdit("8");
    mediumParamBox = new QLineEdit("30000");
    hardParamBox1 = new QLineEdit("1000");
    hardParamBox2 = new QLineEdit("50");
    hardParamBox3 = new QLineEdit("2.5");

    QList<QLineEdit*> edits = {easyParamBox, mediumParamBox, hardParamBox1, hardParamBox2, hardParamBox3};
    for (auto *box : edits) {
        box->setFont(textboxFont);
        box->setMaximumWidth(120);
        box->setAlignment(Qt::AlignCenter);
    }

    // === Labels de paramètres ===
    QLabel *easyLabel = new QLabel("profondeur [6 ~ 9] (int)");
    QLabel *mediumLabel = new QLabel("itérations [100 ~ 100000] (int)");
    hardLabels[0] = new QLabel("itérations [100 ~ 10000] (int)");
    hardLabels[1] = new QLabel("simulations [10 ~ 200] (int)");
    hardLabels[2] = new QLabel("ucb constant [0.9 ~ 4.0] (float)");

    QList<QLabel*> labels = {easyLabel, mediumLabel, hardLabels[0], hardLabels[1], hardLabels[2]};
    for (auto *lab : labels) {
        lab->setFont(textboxFont);
        lab->setStyleSheet("color: #333;");
    }

    // === Logo Polytech ===
    logoLabel = new QLabel;
    QPixmap logo(".\\Ressources\\image\\Logo_PolytechTours.png");
    if (!logo.isNull()) {
        logoLabel->setPixmap(logo.scaled(200, 200, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // === Layout des difficultés ===
    QVBoxLayout *diffLayout = new QVBoxLayout;

    auto addDiffRow = [&](QPushButton *button, QWidget *paramWidget, QWidget *labelWidget) {
        QHBoxLayout *row = new QHBoxLayout;
        row->addWidget(button);
        row->addWidget(paramWidget);
        row->addWidget(labelWidget);
        row->addStretch();
        diffLayout->addLayout(row);
    };

    addDiffRow(easyButton, easyParamBox, easyLabel);
    addDiffRow(mediumButton, mediumParamBox, mediumLabel);

    QVBoxLayout *hardParams = new QVBoxLayout;
    hardParams->addWidget(hardParamBox1);
    hardParams->addWidget(hardParamBox2);
    hardParams->addWidget(hardParamBox3);
    QVBoxLayout *hardLabelsLayout = new QVBoxLayout;
    hardLabelsLayout->addWidget(hardLabels[0]);
    hardLabelsLayout->addWidget(hardLabels[1]);
    hardLabelsLayout->addWidget(hardLabels[2]);
    QHBoxLayout *hardRow = new QHBoxLayout;
    hardRow->addWidget(hardButton);
    hardRow->addLayout(hardParams);
    hardRow->addLayout(hardLabelsLayout);
    diffLayout->addLayout(hardRow);

    // === Layout principal ===
    QVBoxLayout *mainLayout = new QVBoxLayout;
    mainLayout->addWidget(logoLabel, 0, Qt::AlignRight);
    mainLayout->addStretch();
    mainLayout->addWidget(playButton, 0, Qt::AlignCenter);
    mainLayout->addSpacing(20);
    mainLayout->addLayout(diffLayout);
    mainLayout->addStretch();
    setLayout(mainLayout);

    // === Connexions ===
    connect(playButton, &QPushButton::clicked, this, &MainMenu::onPlay);
    connect(easyButton, &QPushButton::clicked, this, &MainMenu::onEasy);
    connect(mediumButton, &QPushButton::clicked, this, &MainMenu::onMedium);
    connect(hardButton, &QPushButton::clicked, this, &MainMenu::onHard);
}

void MainMenu::resetDifficultyColors()
{
    easyButton->setStyleSheet("background-color: #A0C882; color: white; border-radius: 10px;");
    mediumButton->setStyleSheet("background-color: #F09C42; color: white; border-radius: 10px;");
    hardButton->setStyleSheet("background-color: #EC776D; color: white; border-radius: 10px;");
}

void MainMenu::onPlay()
{
    emit playClicked();
}

void MainMenu::onEasy()
{
    resetDifficultyColors();
    easyButton->setStyleSheet("background-color: #89B46F; color: white;");
    bool ok = false;
    float p1 = easyParamBox->text().toFloat(&ok);
    if (ok) emit difficultyChanged(StateMachine::Difficulty::Easy, p1, 0, 0);
}

void MainMenu::onMedium()
{
    resetDifficultyColors();
    mediumButton->setStyleSheet("background-color: #D88938; color: white;");
    bool ok = false;
    float p1 = mediumParamBox->text().toFloat(&ok);
    if (ok) emit difficultyChanged(StateMachine::Difficulty::Medium, p1, 0, 0);
}

void MainMenu::onHard()
{
    resetDifficultyColors();
    hardButton->setStyleSheet("background-color: #C55D53; color: white;");
    bool ok1, ok2, ok3;
    float p1 = hardParamBox1->text().toFloat(&ok1);
    float p2 = hardParamBox2->text().toFloat(&ok2);
    float p3 = hardParamBox3->text().toFloat(&ok3);
    if (ok1 && ok2 && ok3)
        emit difficultyChanged(StateMachine::Difficulty::Hard, p1, p2, p3);
}
