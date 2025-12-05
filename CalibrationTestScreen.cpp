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
    titleLabel->setStyleSheet("font-size: 60px; font-weight: bold; color: #1B3B5F;");

    // === INSTRUCTIONS ===
    instructionsLabel = new QLabel(
        "<div style='text-align: center;'>"
        "<b>Avant de lancer le test, assurez-vous que :</b><br><br>"
        "• Les réservoirs sont remplis de pions,<br>"
        "• La grille est vide.<br>"
        "<br>"
        "Le test va récupérer les 8 pions des réservoirs et les déposer dans la grille."
        "</div>",
        this
    );
    instructionsLabel->setAlignment(Qt::AlignCenter);  // Centre le QLabel lui-même
    instructionsLabel->setWordWrap(true);
    instructionsLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 20px;");
    instructionsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    instructionsLabel->setMinimumHeight(200);  // Hauteur minimale pour afficher tout le texte
    instructionsLabel->setMinimumWidth(900);  // Largeur minimale pour éviter la coupure

    // === STATUT ===
    statusLabel = new QLabel("", this);
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setWordWrap(true);
    statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 25px; line-height: 1.5;");
    statusLabel->setMinimumHeight(200);  // Hauteur minimale augmentée pour afficher les messages d'urgence
    statusLabel->setMaximumHeight(250);  // Hauteur maximale pour contrôler l'expansion
    statusLabel->setMinimumWidth(700);  // Largeur minimale pour afficher les messages longs
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
    backButton = new QPushButton("Retour");
    emergencyStopButton = new QPushButton("ARRÊT D'URGENCE");

    stopButton->hide();
    emergencyStopButton->hide();

    QHBoxLayout* buttonLayout = new QHBoxLayout();
    buttonLayout->setAlignment(Qt::AlignCenter);
    buttonLayout->setSpacing(30);
    buttonLayout->addWidget(startButton);
    buttonLayout->addWidget(stopButton);

    // Style du bouton retour (même style que dans MainMenu)
    backButton->setFixedSize(160, 55);
    backButton->setStyleSheet(
        "QPushButton { background-color: #E0E0E0; color: #1B3B5F;"
        " font-size: 22px; font-weight: bold; border-radius: 27px; }"
        "QPushButton:hover { background-color: #D0D0D0; }"
        "QPushButton:pressed { background-color: #A8A8A8; }"
    );
    backButton->setCursor(Qt::PointingHandCursor);

    // === LAYOUT ===
    mainLayout->addWidget(backButton, 0, Qt::AlignLeft);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(titleLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(instructionsLabel, 0, Qt::AlignCenter);
    mainLayout->addSpacing(20);
    mainLayout->addWidget(statusLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(loadingLabel, 0, Qt::AlignCenter);
    mainLayout->addWidget(progressBar, 0, Qt::AlignCenter);
    mainLayout->addSpacing(40);
    mainLayout->addLayout(buttonLayout);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(emergencyStopButton, 0, Qt::AlignCenter);
    mainLayout->addStretch();

    // === STYLE DES BOUTONS ===
    styleButton(startButton, "#2ECC71", "#27AE60");
    styleButton(stopButton, "#E74C3C", "#C0392B");
    styleButton(emergencyStopButton, "#FF0000", "#CC0000");  // Rouge vif pour arrêt d'urgence

    startButton->setFixedHeight(70);
    stopButton->setFixedHeight(70);
    emergencyStopButton->setFixedHeight(60);
    emergencyStopButton->setMinimumWidth(320);

    // === CONNEXIONS ===
    connect(startButton, &QPushButton::clicked, this, &CalibrationTestScreen::onStartTestClicked);
    connect(stopButton, &QPushButton::clicked, this, &CalibrationTestScreen::onStopTestClicked);
    connect(backButton, &QPushButton::clicked, this, &CalibrationTestScreen::onBackToMenuClicked);
    connect(emergencyStopButton, &QPushButton::clicked, this, [this]() {
        qDebug() << "[CalibrationTestScreen] ARRÊT D'URGENCE activé !";

        // Arrêt immédiat du robot
        this->robot->emergencyStop();
        this->shouldStop = true;

        // Arrêter le test et mettre à jour l'interface
        QMetaObject::invokeMethod(this, [this]() {
            // Arrêter les animations et la progression
            loadingMovie->stop();
            loadingLabel->hide();
            progressBar->hide();

            // Cacher tous les boutons sauf le bouton retour
            startButton->hide();
            stopButton->hide();
            emergencyStopButton->hide();

            // Afficher un message d'urgence
            statusLabel->setText("ARRÊT D'URGENCE ACTIVÉ️<br><br>"
                                "Le robot a été arrêté immédiatement.<br>");
            statusLabel->setStyleSheet("font-size: 24px; color: #FF0000; font-weight: bold; padding: 15px;");
            statusLabel->show();

            // Cacher les instructions
            instructionsLabel->hide();

            // Afficher le bouton retour
            backButton->show();

            testRunning = false;
        }, Qt::QueuedConnection);
    });

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
    emergencyStopButton->hide();
    backButton->show();
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

    // Masquer le bouton start, afficher le bouton stop et arrêt d'urgence
    startButton->hide();
    stopButton->show();
    emergencyStopButton->show();

    // Masquer le bouton retour pendant le test
    backButton->hide();

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
            emergencyStopButton->hide();
            startButton->show();
            backButton->show();
        }, Qt::QueuedConnection);
        testRunning = false;
        return;
    }
    qDebug() << "[CalibrationTestScreen] Robot connecté avec succès";

    // Afficher la progression (sauf si arrêt d'urgence)
    QMetaObject::invokeMethod(this, [this]() {
        if (!shouldStop) {
            statusLabel->setText("Remise en position initiale...");
            statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");
        }
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

    // Afficher la barre de progression (sauf si arrêt d'urgence)
    QMetaObject::invokeMethod(this, [this]() {
        if (!shouldStop) {
            progressBar->show();
            loadingMovie->stop();
            loadingLabel->hide();
        }
    }, Qt::QueuedConnection);

    // Test des mouvements : épuise réservoir gauche (4 pions) puis réservoir droit (4 pions)
    for (int i = 0; i < 8; i++) {
        // Vérification AVANT de commencer un nouveau cycle
        if (shouldStop) {
            qDebug() << "[CalibrationTestScreen] Arrêt demandé avant le test" << (i+1);
            break;
        }

        qDebug() << QString("[CalibrationTestScreen] === Test %1/8 ===").arg(i+1);

        // Mettre à jour le statut (sauf si arrêt d'urgence)
        QMetaObject::invokeMethod(this, [this, i]() {
            if (!shouldStop) {
                statusLabel->setText(QString("Test %1/8 : Récupération du pion...").arg(i+1));
            }
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

        // Mettre à jour le statut (sauf si arrêt d'urgence)
        QMetaObject::invokeMethod(this, [this, i]() {
            if (!shouldStop) {
                statusLabel->setText(QString("Test %1/8 : Dépôt du pion...").arg(i+1));
            }
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

        // Mettre à jour la progression (sauf si arrêt d'urgence)
        QMetaObject::invokeMethod(this, [this, i]() {
            if (!shouldStop) {
                progressBar->setValue(i + 1);
            }
        }, Qt::QueuedConnection);

        qDebug() << QString("[CalibrationTestScreen] Test %1/8 terminé").arg(i+1);
    }

    // Positionnement au-dessus du réservoir gauche avant de déconnecter
    qDebug() << "[CalibrationTestScreen] Déplacement au-dessus du réservoir gauche...";
    calib->goToLeftReservoirArea();
    qDebug() << "[CalibrationTestScreen] Positionné au-dessus du réservoir gauche";

    // Déconnexion
    qDebug() << "[CalibrationTestScreen] Déconnexion du robot";
    calib->disconnectToRobot();

    // Mettre à jour l'interface SEULEMENT si ce n'est pas un arrêt d'urgence
    // (l'arrêt d'urgence a déjà mis à jour l'UI avec son message rouge)
    if (!shouldStop) {
        QMetaObject::invokeMethod(this, [this]() {
            loadingMovie->stop();
            loadingLabel->hide();

            statusLabel->setText("Test terminé avec succès !<br>Utilisez le bouton Retour pour revenir au menu principal.");
            statusLabel->setStyleSheet("font-size: 24px; color: #1B3B5F; font-weight: bold; padding: 15px;");

            stopButton->hide();
            emergencyStopButton->hide();
            // Ne pas réafficher le bouton "Lancer le test" - l'utilisateur doit retourner au menu
            startButton->hide();
            backButton->show();
        }, Qt::QueuedConnection);
    }

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
