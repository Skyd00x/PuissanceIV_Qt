#include "CalibrationTestScreen.hpp"
#include <QPalette>
#include <QFont>
#include <QDebug>
#include <thread>
#include <chrono>

CalibrationTestScreen::CalibrationTestScreen(Robot *robot, CalibrationLogic* calibLogic, QWidget *parent)
    : QWidget(parent), robot(robot), calib(calibLogic), testRunning(false), shouldStop(false)
{
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, Qt::white);
    setPalette(pal);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(60, 40, 60, 40);
    mainLayout->setSpacing(30);

    // === TITRE ===
    titleLabel = new QLabel("Test de calibration", this);
    titleLabel->setAlignment(Qt::AlignCenter);
    titleLabel->setStyleSheet("font-size: 48px; font-weight: bold; color: #1B3B5F;");

    // === INSTRUCTIONS ===
    instructionsLabel = new QLabel(
        "<b>Avant de lancer le test, assurez-vous que :</b><br>"
        "• Les réservoirs gauche et droit sont remplis de pions (4 pions chacun)<br>"
        "• La grille est vide<br>"
        "• Le robot est en position de sécurité<br>"
        "<br>"
        "Le test va récupérer les 8 pions des réservoirs et les déposer dans la grille.",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 20px;");
    instructionsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    instructionsLabel->setMinimumWidth(900);

    // === STATUT ===
    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 25px; line-height: 1.5;");
    statusLabel->setMinimumHeight(120);  // Hauteur minimale encore augmentée pour éviter que le texte soit coupé
    statusLabel->setMaximumHeight(150);  // Hauteur maximale pour contrôler l'expansion
    statusLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    statusLabel->hide();

    // === LOADING ===
    loadingLabel = new QLabel(this);
    loadingMovie = new QMovie("./Ressources/image/Gifs/simple_loading.gif");
    loadingLabel->setMovie(loadingMovie);
    loadingLabel->setFixedSize(85, 85);
    loadingLabel->setScaledContents(true);
    loadingLabel->setAlignment(Qt::AlignCenter);
    loadingLabel->hide();

    // === BARRE DE PROGRESSION ===
    progressBar = new QProgressBar(this);
    progressBar->setRange(0, 8);  // 8 pions à tester
    progressBar->setValue(0);
    progressBar->setFixedSize(900, 30);
    progressBar->setTextVisible(true);
    progressBar->setFormat("%v / %m pions testés");
    progressBar->setStyleSheet(
        "QProgressBar { background-color: #E0E0E0; border-radius: 15px; color: #1B3B5F; "
        "font-size: 18px; font-weight: bold; text-align: center; }"
        "QProgressBar::chunk { background-color: #4F8ED8; border-radius: 15px; }"
    );
    progressBar->hide();

    // === BOUTONS ===
    startButton = new QPushButton("Lancer le test");
    stopButton = new QPushButton("Arrêter le test");
    backButton = new QPushButton("← Retour au menu");

    stopButton->hide();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(30);
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);

    // Style du bouton retour (différent des autres)
    backButton->setFixedSize(200, 60);
    backButton->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 30px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
    );
    backButton->setCursor(Qt::PointingHandCursor);

    // === LAYOUT ===
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(instructionsLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(statusLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(loadingLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(progressBar, 0, Qt::AlignCenter);
    mainLayout->addSpacing(40);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addStretch();
    mainLayout->addWidget(backButton, 0, Qt::AlignLeft);

    // === STYLE DES BOUTONS ===
    styleButton(startButton, "#2ECC71", "#27AE60");
    styleButton(stopButton, "#E74C3C", "#C0392B");

    startButton->setFixedHeight(70);
    stopButton->setFixedHeight(70);

    // === CONNEXIONS ===
    connect(startButton, &QPushButton::clicked, this, &CalibrationTestScreen::onStartTestClicked);
    connect(stopButton, &QPushButton::clicked, this, &CalibrationTestScreen::onStopTestClicked);
    connect(backButton, &QPushButton::clicked, this, &CalibrationTestScreen::onBackToMenuClicked);

    setLayout(mainLayout);
}

void CalibrationTestScreen::resetScreen() {
    shouldStop = false;
    testRunning = false;

    instructionsLabel->show();
    statusLabel->hide();
    statusLabel->setText("");
    loadingLabel->hide();
    loadingMovie->stop();
    progressBar->hide();
    progressBar->setValue(0);

    startButton->show();
    stopButton->hide();
    backButton->setEnabled(true);
}

void CalibrationTestScreen::onStartTestClicked() {
    if (testRunning) {
        qDebug() << "[CalibrationTestScreen] Test déjà en cours";
        return;
    }

    qDebug() << "[CalibrationTestScreen] Démarrage du test";

    // Masquer les instructions et afficher le statut
    instructionsLabel->hide();
    statusLabel->setText("Connexion au robot...");
    statusLabel->show();
    loadingLabel->show();
    loadingMovie->start();

    // Masquer le bouton start, afficher le bouton stop
    startButton->hide();
    stopButton->show();

    // Désactiver le bouton retour pendant le test
    backButton->setEnabled(false);

    // Lancer le test dans un thread séparé
    testRunning = true;
    shouldStop = false;

    std::thread([this]() {
        runTest();
    }).detach();
}

void CalibrationTestScreen::onStopTestClicked() {
    qDebug() << "[CalibrationTestScreen] Arrêt demandé par l'utilisateur";
    shouldStop = true;

    // Mettre à jour le statut (garder la couleur par défaut)
    QMetaObject::invokeMethod(this, [this]() {
        statusLabel->setText("Arrêt du test en cours...");
        statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");
    }, Qt::QueuedConnection);
}

void CalibrationTestScreen::onBackToMenuClicked() {
    qDebug() << "[CalibrationTestScreen] Retour au menu demandé";

    // S'assurer que le test est arrêté
    if (testRunning) {
        shouldStop = true;
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    emit backToMenuRequested();
}

void CalibrationTestScreen::runTest() {
    qDebug() << "[CalibrationTestScreen] Thread de test démarré";

    // IMPORTANT: Recharger la calibration depuis le fichier avant le test
    // (car elle peut avoir été réinitialisée si l'utilisateur a quitté l'écran de calibration)
    qDebug() << "[CalibrationTestScreen] Rechargement de la calibration depuis le fichier...";
    calib->loadCalibration("./calibration.json");

    // Connexion au robot
    if (!calib->connectToRobot()) {
        qWarning() << "[CalibrationTestScreen] ERREUR: Impossible de se connecter au robot!";
        QMetaObject::invokeMethod(this, [this]() {
            statusLabel->setText("ERREUR: Impossible de se connecter au robot");
            statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");
            loadingMovie->stop();
            loadingLabel->hide();
            stopButton->hide();
            startButton->show();
            backButton->setEnabled(true);
        }, Qt::QueuedConnection);
        testRunning = false;
        return;
    }
    qDebug() << "[CalibrationTestScreen] Robot connecté avec succès";

    // Afficher la progression
    QMetaObject::invokeMethod(this, [this]() {
        statusLabel->setText("Remise en position initiale...");
        statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");
    }, Qt::QueuedConnection);

    // Remise à la position d'origine
    robot->Home();
    std::this_thread::sleep_for(std::chrono::seconds(2));

    if (shouldStop) {
        qDebug() << "[CalibrationTestScreen] Arrêt demandé pendant Home()";
        calib->disconnectToRobot();
        QMetaObject::invokeMethod(this, [this]() { resetScreen(); }, Qt::QueuedConnection);
        testRunning = false;
        return;
    }

    // Afficher la barre de progression
    QMetaObject::invokeMethod(this, [this]() {
        progressBar->show();
        loadingMovie->stop();
        loadingLabel->hide();
    }, Qt::QueuedConnection);

    // Test des mouvements : épuise réservoir gauche (4 pions) puis réservoir droit (4 pions)
    for (int i = 0; i < 8; i++) {
        // Vérification AVANT de commencer un nouveau cycle
        if (shouldStop) {
            qDebug() << "[CalibrationTestScreen] Arrêt demandé avant le test" << (i+1);
            break;
        }

        qDebug() << QString("[CalibrationTestScreen] === Test %1/8 ===").arg(i+1);

        // Mettre à jour le statut
        QMetaObject::invokeMethod(this, [this, i]() {
            statusLabel->setText(QString("Test %1/8 : Récupération du pion...").arg(i+1));
        }, Qt::QueuedConnection);

        // Déterminer la position de réservoir
        CalibPoint pickPoint;
        if (i < 4) {
            // Réservoir gauche (Left_1 à Left_4)
            pickPoint = static_cast<CalibPoint>((int)CalibPoint::Left_1 + i);
            qDebug() << QString("[CalibrationTestScreen] Prendre pion à la position Left_%1").arg(i + 1);
        } else {
            // Réservoir droit (Right_1 à Right_4)
            pickPoint = static_cast<CalibPoint>((int)CalibPoint::Right_1 + (i - 4));
            qDebug() << QString("[CalibrationTestScreen] Prendre pion à la position Right_%1").arg((i - 4) + 1);
        }

        // Vérification AVANT de prendre le pion
        if (shouldStop) {
            qDebug() << "[CalibrationTestScreen] Arrêt demandé avant pickPiece()";
            break;
        }

        // 1. Prendre le pion (utilise la nouvelle fonction de haut niveau)
        calib->pickPiece(pickPoint);

        // Vérification APRÈS avoir pris le pion
        if (shouldStop) {
            qDebug() << "[CalibrationTestScreen] Arrêt demandé après pickPiece()";
            // Lâcher le pion avant d'arrêter (sécurité)
            robot->openGripper();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            robot->turnOffGripper();
            break;
        }

        // Mettre à jour le statut
        QMetaObject::invokeMethod(this, [this, i]() {
            statusLabel->setText(QString("Test %1/8 : Dépôt du pion...").arg(i+1));
        }, Qt::QueuedConnection);

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // 2. Déposer dans une colonne (on fait le tour des colonnes 0-6, puis revient à 0)
        int column = i % 7;
        qDebug() << QString("[CalibrationTestScreen] Déposer pion dans colonne %1").arg(column + 1);

        // Vérification AVANT de déposer le pion
        if (shouldStop) {
            qDebug() << "[CalibrationTestScreen] Arrêt demandé avant dropPiece()";
            // Lâcher le pion avant d'arrêter (sécurité)
            robot->openGripper();
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            robot->turnOffGripper();
            break;
        }

        calib->dropPiece(column);

        // Vérification APRÈS avoir déposé le pion
        if (shouldStop) {
            qDebug() << "[CalibrationTestScreen] Arrêt demandé après dropPiece()";
            break;
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(200));

        // Mettre à jour la progression
        QMetaObject::invokeMethod(this, [this, i]() {
            progressBar->setValue(i + 1);
        }, Qt::QueuedConnection);

        qDebug() << QString("[CalibrationTestScreen] Test %1/8 terminé").arg(i+1);
    }

    // Retour à la position d'origine
    if (!shouldStop) {
        qDebug() << "[CalibrationTestScreen] Retour à la position d'origine...";
        QMetaObject::invokeMethod(this, [this]() {
            statusLabel->setText("Retour à la position initiale...");
            loadingLabel->show();
            loadingMovie->start();
        }, Qt::QueuedConnection);

        robot->Home();
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }

    // Déconnexion
    qDebug() << "[CalibrationTestScreen] Déconnexion du robot";
    calib->disconnectToRobot();

    // Mettre à jour l'interface
    QMetaObject::invokeMethod(this, [this]() {
        loadingMovie->stop();
        loadingLabel->hide();

        if (shouldStop) {
            statusLabel->setText("Test arrêté par l'utilisateur");
            statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");
        } else {
            statusLabel->setText("✅ Test terminé avec succès !");
            statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");
        }

        stopButton->hide();
        startButton->show();
        backButton->setEnabled(true);
    }, Qt::QueuedConnection);

    testRunning = false;
    qDebug() << "[CalibrationTestScreen] Thread de test terminé";
}

void CalibrationTestScreen::styleButton(QPushButton *button, const QString &c1, const QString &c2) {
    button->setStyleSheet(QString(
                              "QPushButton {"
                              "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 %1, stop:1 %2);"
                              "   color: white;"
                              "   font-size: 22px;"
                              "   font-weight: bold;"
                              "   border: none;"
                              "   border-radius: 35px;"
                              "   padding: 15px 30px;"
                              "   min-width: 280px;"
                              "   min-height: 70px;"
                              "   text-align: center;"
                              "}"
                              "QPushButton:hover {"
                              "   background: qlineargradient(x1:0, y1:0, x2:0, y2:1, stop:0 #66B0FF, stop:1 #347AD1);"
                              "}"
                              "QPushButton:pressed {"
                              "   background-color: %2;"
                              "}"
                              ).arg(c1, c2));

    button->setFont(QFont("Segoe UI", 14, QFont::Bold));
    button->setCursor(Qt::PointingHandCursor);

    auto *shadow = new QGraphicsDropShadowEffect(button);
    shadow->setBlurRadius(25);
    shadow->setOffset(3, 5);
    shadow->setColor(QColor(0, 0, 0, 120));
    button->setGraphicsEffect(shadow);
}
