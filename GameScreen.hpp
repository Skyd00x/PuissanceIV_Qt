#pragma once

#include <QWidget>
#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGraphicsDropShadowEffect>
#include <QStackedWidget>
#include <QMovie>

class GameScreen : public QWidget
{
    Q_OBJECT

public:
    explicit GameScreen(QWidget *parent = nullptr);

    void startGame();  // Lance le countdown et prépare le jeu
    void resetGame();  // Réinitialise complètement l'écran de jeu

    // Slots pour GameLogic
public slots:
    void updateCameraFrame(const QImage &img);
    void setTurnPlayer();
    void setTurnRobot();
    void setRobotStatus(const QString &status);
    void setDifficultyText(const QString &txt);
    void showEndOfGame(const QString &winnerText, int totalSeconds);
    void showGridIncompleteWarning(int detectedCount);
    void showRobotInitializing();      // Message "Mise en position initiale"
    void hideRobotInitializing();      // Cacher le message
    void showCheatDetected(const QString &reason);  // Message de triche avec bouton quitter
    void showReservoirEmpty();         // Message réservoirs vides avec bouton "C'est fait"
    void showGameResult(const QString &winner, const QString &difficulty, int totalSeconds);  // Afficher le résultat de la partie
    void resetAllOverlays();           // Cacher tous les overlays
    bool isReservoirOverlayVisible() const;  // Vérifier si l'overlay de réservoirs est visible
    void startCountdownWhenReady();    // Démarrer le countdown quand le robot est prêt
    void showConnectionError();        // Afficher l'overlay d'erreur de connexion robot

signals:
    void quitRequested();          // L'utilisateur veut quitter la partie
    void prepareGame();            // Avant le countdown → préparer le robot
    void countdownFinished();      // Fin du compte à rebours → GameLogic démarre
    void reservoirsRefilled();     // L'utilisateur a rempli les réservoirs

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void updateCountdown();
    void updateChronometer();
    void onQuitButtonClicked();
    void showConfirmationScreen();
    void returnToGame();

private:
    void startCountdown();
    void createGameWidget();
    void createConfirmWidget();
    void createInitializingWidget();

private:
    QStackedWidget *stack;

    // Widgets
    QWidget *initializingWidget;   // Widget d'initialisation (avant le jeu)
    QWidget *gameWidget;
    QWidget *confirmWidget;

    // Éléments du jeu
    QPushButton *quitButton;

    QLabel *titleLabel;          // Partie en mode X
    QLabel *turnLabel;           // Au tour du joueur/robot
    QLabel *cameraLabel;         // Affichage de la caméra

    QLabel *timerLabel;          // Chronomètre
    QTimer chronometer;
    int elapsedSeconds = 0;

    QLabel *countdownTextLabel;  // Texte "Lancement de la partie dans"
    QLabel *countdownLabel;      // Compte à rebours (3,2,1)
    QTimer countdownTimer;
    int countdownValue = 3;

    QLabel *warningLabel;        // Message d'avertissement (grille incomplète)
    QWidget *warningOverlay;     // Widget overlay pour le message
    QPushButton *warningQuitButton; // Bouton pour quitter quand grille incomplète

    // Éléments du widget d'initialisation
    QLabel *initializingLabel;          // Label de statut (ex: "Mise en position initiale...")
    QLabel *initializingLoadingLabel;   // Label pour le GIF de chargement
    QMovie *initializingLoadingMovie;   // Animation de chargement

    // Overlay pour triche détectée
    QWidget *cheatOverlay;
    QLabel *cheatLabel;
    QPushButton *cheatQuitButton;

    // Overlay pour réservoirs vides
    QWidget *reservoirOverlay;
    QLabel *reservoirLabel;
    QPushButton *reservoirRefillButton;

    // Overlay pour résultat de la partie (victoire/égalité)
    QWidget *resultOverlay;
    QLabel *resultLabel;
    QPushButton *resultQuitButton;

    // Overlay pour erreur de connexion au robot
    QWidget *connectionErrorOverlay;
    QLabel *connectionErrorLabel;
    QPushButton *retryConnectionButton;
    QPushButton *quitFromConnectionErrorButton;

    // Timer pour éviter d'afficher l'overlay de grille incomplète trop tôt
    QTimer gridWarningDelayTimer;
    bool allowGridWarning = false;
};
