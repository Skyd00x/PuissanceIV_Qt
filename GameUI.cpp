#include "GameUI.hpp"
#include <QFile>
#include <QDebug>
#include <QPainter>
#include <QStyleOption>

GameUI::GameUI(Robot* robot, QWidget *parent)
    : QWidget(parent), robot(robot)
{
    setStyleSheet("background-color: #FEEEC8;"); // Couleur de fond (équiv. SFML backGroundColor)

    // === Zone caméra ===
    cameraLabel = new QLabel("Chargement de la caméra...");
    cameraLabel->setAlignment(Qt::AlignCenter);
    cameraLabel->setMinimumSize(640, 360);
    cameraLabel->setStyleSheet("background-color: #333; color: white;");

    // === Boutons ===
    backButton = new QPushButton("Retour");
    restartButton = new QPushButton("Rejouer");
    refillButton = new QPushButton("Recharger");

    QString buttonStyle = R"(
        QPushButton {
            background-color: #92F1F8;
            color: white;
            font-size: 24px;
            border-radius: 15px;
            padding: 8px 16px;
        }
        QPushButton:hover {
            background-color: #5DC9D4;
        }
    )";
    backButton->setStyleSheet(buttonStyle);
    restartButton->setStyleSheet(buttonStyle);
    refillButton->setStyleSheet(buttonStyle);

    // === Logo Polytech ===
    logoPolytech = new QLabel;
    if (logoTexture.load(".\\Ressources\\image\\Logo_PolytechTours.png")) {
        logoPolytech->setPixmap(logoTexture.scaled(150, 150, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    }

    // === Label de victoire ===
    victoryLabel = new QLabel;
    victoryLabel->setAlignment(Qt::AlignCenter);
    victoryLabel->setStyleSheet("color: green; font-size: 40px; font-weight: bold;");
    victoryLabel->hide();

    // === Layout principal ===
    QVBoxLayout *mainLayout = new QVBoxLayout;
    QHBoxLayout *topBar = new QHBoxLayout;
    QVBoxLayout *buttonLayout = new QVBoxLayout;

    buttonLayout->addWidget(backButton);
    buttonLayout->addWidget(restartButton);
    buttonLayout->addWidget(refillButton);
    buttonLayout->addStretch();

    topBar->addLayout(buttonLayout);
    topBar->addWidget(cameraLabel, 1);
    topBar->addWidget(logoPolytech);

    mainLayout->addLayout(topBar);
    mainLayout->addWidget(victoryLabel);
    setLayout(mainLayout);

    // === Connexions ===
    connect(backButton, &QPushButton::clicked, this, &GameUI::onBackButton);
    connect(restartButton, &QPushButton::clicked, this, &GameUI::onRestartButton);
    connect(refillButton, &QPushButton::clicked, this, &GameUI::onRefillButton);
}

GameUI::~GameUI() {}

void GameUI::onBackButton() { emit backClicked(); }
void GameUI::onRestartButton() { emit restartClicked(); victoryLabel->hide(); }
void GameUI::onRefillButton() {
    if (robot) robot->Refill();
    emit refillClicked();
}

void GameUI::updateCameraFrame(const cv::Mat& frame) {
    if (frame.empty()) return;

    QImage image(frame.data, frame.cols, frame.rows, frame.step, QImage::Format_BGR888);
    cameraLabel->setPixmap(QPixmap::fromImage(image).scaled(
        cameraLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

void GameUI::updateBoard(const Board& board) {
    currentBoard = board;
    update(); // redessine (paintEvent)
}

void GameUI::playerVictory() {
    victoryLabel->setText("Victoire du joueur !");
    victoryLabel->show();
}

void GameUI::playerDefeat() {
    victoryLabel->setText("Défaite du joueur !");
    victoryLabel->show();
}

void GameUI::resetBoard() {
    currentBoard = Board();
    victoryLabel->hide();
    update();
}

void GameUI::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter painter(this);
    QStyleOption opt;
    opt.initFrom(this);
    style()->drawPrimitive(QStyle::PE_Widget, &opt, &painter, this);

    // Exemple d’affichage du plateau (simplifié)
    const int cols = 7;
    const int rows = 6;
    int gridW = width() / 2;
    int gridH = height() / 2;
    int startX = width() / 4;
    int startY = height() / 2 - gridH / 2;

    painter.setBrush(QColor("#028ECC"));
    painter.drawRect(startX, startY, gridW, gridH);

    float cellW = gridW / (float)cols;
    float cellH = gridH / (float)rows;

    for (int i = 0; i < cols; ++i) {
        for (int j = 0; j < rows; ++j) {
            int piece = currentBoard.getPiece(i, rows - 1 - j);
            QColor color = QColor("#BFF0FF"); // empty
            if (piece == 1) color = Qt::red;
            else if (piece == 2) color = Qt::yellow;

            painter.setBrush(color);
            painter.setPen(Qt::NoPen);
            painter.drawEllipse(QRectF(startX + i * cellW + 5, startY + j * cellH + 5, cellW - 10, cellH - 10));
        }
    }
}
