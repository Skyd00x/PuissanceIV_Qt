cmake_minimum_required(VERSION 3.16)

project(PuissanceIV_QT VERSION 0.1 LANGUAGES CXX)

# === Paramètres généraux ===
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# === Qt ===
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES PuissanceIV_QT_fr_FR.ts)

# === Fichiers sources du projet ===
set(PROJECT_SOURCES
    main.cpp
    MainWindow.cpp MainWindow.hpp MainWindow.ui
    MainMenu.cpp MainMenu.hpp
    GameUI.cpp GameUI.hpp
    Board.cpp Board.hpp
    BoardDetector.cpp BoardDetector.hpp
    Camera.cpp Camera.hpp
    Robot.cpp Robot.hpp
    StateMachine.cpp StateMachine.hpp
    Negamax.cpp Negamax.hpp
    MonteCarlo.cpp MonteCarlo.hpp
    MCTS.cpp MCTS.hpp
    TranspositionTable.cpp TranspositionTable.hpp
    ${TS_FILES}
)

file(COPY ${CMAKE_SOURCE_DIR}/Ressources DESTINATION ${CMAKE_BINARY_DIR})

# === Création de l'exécutable Qt ===
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PuissanceIV_QT
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        IntroScreen.hpp IntroScreen.cpp
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(PuissanceIV_QT ${PROJECT_SOURCES})
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# ============================================================
# === OpenCV (local dans le projet) ===
# ============================================================
set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv")
include_directories("${OpenCV_DIR}/include")
link_directories("${OpenCV_DIR}/x64/vc16/lib")

# Récupère toutes les bibliothèques OpenCV .lib (Release & Debug)
file(GLOB OPENCV_LIBS "${OpenCV_DIR}/x64/vc16/lib/*.lib")
message(STATUS "✅ OpenCV libs trouvées : ${OPENCV_LIBS}")

# Copie automatique de toutes les DLL OpenCV présentes
file(GLOB OPENCV_DLLS "${OpenCV_DIR}/x64/vc16/bin/opencv_world*.dll")
if (OPENCV_DLLS)
    add_custom_command(TARGET PuissanceIV_QT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENCV_DLLS}
            $<TARGET_FILE_DIR:PuissanceIV_QT>
    )
else()
    message(WARNING "⚠️ Aucune DLL OpenCV trouvée dans ${OpenCV_DIR}/x64/vc16/bin/")
endif()

# ============================================================
# === Dobot SDK (local) ===
# ============================================================
set(DOBOT_DIR "${CMAKE_SOURCE_DIR}/Dobot")
include_directories("${DOBOT_DIR}")
link_directories("${DOBOT_DIR}")

if(NOT EXISTS "${DOBOT_DIR}/DobotDll.lib")
    message(FATAL_ERROR "❌ Fichier DobotDll.lib introuvable dans ${DOBOT_DIR}")
endif()

# Copie automatique de la DLL Dobot après build
add_custom_command(TARGET PuissanceIV_QT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DOBOT_DIR}/DobotDll.dll"
        $<TARGET_FILE_DIR:PuissanceIV_QT>
)

# ============================================================
# === Liaisons des librairies ===
# ============================================================
target_link_libraries(PuissanceIV_QT
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${OPENCV_LIBS}
        "${DOBOT_DIR}/DobotDll.lib"
)

# ============================================================
# === Propriétés de l'exécutable ===
# ============================================================
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.PuissanceIV_QT)
endif()

set_target_properties(PuissanceIV_QT PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

# ============================================================
# === Installation (facultatif) ===
# ============================================================
include(GNUInstallDirs)
install(TARGETS PuissanceIV_QT
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PuissanceIV_QT)
endif()
