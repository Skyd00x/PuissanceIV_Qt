#pragma once

#include <QString>
#include <QCoreApplication>
#include <QDir>
#include <QDebug>

/**
 * @brief Retourne le chemin absolu vers un fichier de ressource
 *
 * Cette fonction garantit que les ressources sont toujours trouvées,
 * quel que soit le répertoire de travail au lancement de l'application.
 *
 * Utilise QCoreApplication::applicationDirPath() pour construire un chemin
 * absolu basé sur l'emplacement de l'exécutable.
 *
 * @param relativePath Chemin relatif depuis le dossier Ressources
 *                     Exemple: "image/Logo_PolytechTours.png"
 * @return QString Chemin absolu vers la ressource
 *
 * @example
 * QString logoPath = getResourcePath("image/Logo_PolytechTours.png");
 * // Retourne: "C:/Program Files/MonApp/Ressources/image/Logo_PolytechTours.png"
 */
inline QString getResourcePath(const QString& relativePath) {
    // Obtenir le répertoire de l'exécutable
    QString appDir = QCoreApplication::applicationDirPath();

    // Construire le chemin complet vers la ressource
    QString fullPath = QDir(appDir).filePath("Ressources/" + relativePath);

    // Normaliser le chemin (convertir les \ en / et supprimer les ./)
    fullPath = QDir::cleanPath(fullPath);

    // Afficher un warning si la ressource n'existe pas (utile pour le debug)
    if (!QFile::exists(fullPath)) {
        qWarning() << "[ResourcesPath] ⚠️ Ressource non trouvée:" << fullPath;
    }

    return fullPath;
}
