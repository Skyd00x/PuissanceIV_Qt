cmake_minimum_required(VERSION 3.16)
project(PuissanceIV_QT VERSION 0.1 LANGUAGES CXX)

# ============================================================
# === PARAMÈTRES GÉNÉRAUX
# ============================================================
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ============================================================
# === Qt
# ============================================================
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets LinguistTools)

set(TS_FILES PuissanceIV_QT_fr_FR.ts)

# ============================================================
# === SOURCES DU PROJET
# ============================================================
set(PROJECT_SOURCES
    main.cpp
    MainWindow.cpp MainWindow.hpp MainWindow.ui
    MainMenu.cpp MainMenu.hpp
    GameScreen.cpp GameScreen.hpp



    CameraAi.cpp CameraAi.hpp
    Robot.cpp Robot.hpp
    StateMachine.cpp StateMachine.hpp
    Negamax.cpp Negamax.hpp


    TranspositionTable.cpp TranspositionTable.hpp
    IntroScreen.cpp IntroScreen.hpp
    CheckDevicesScreen.cpp CheckDevicesScreen.hpp
    CalibrationScreen.cpp CalibrationScreen.hpp
    ExplanationScreen.cpp ExplanationScreen.hpp
    CalibrationLogic.cpp CalibrationLogic.hpp
    ${TS_FILES}
)

# ============================================================
# === COPIE DES RESSOURCES
# ============================================================
file(COPY ${CMAKE_SOURCE_DIR}/Ressources DESTINATION ${CMAKE_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/Model DESTINATION ${CMAKE_BINARY_DIR})

# ============================================================
# === EXÉCUTABLE
# ============================================================
if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(PuissanceIV_QT
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        GameLogic.hpp GameLogic.cpp
    )
    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    add_executable(PuissanceIV_QT ${PROJECT_SOURCES})
    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

# ============================================================
# === OpenCV
# ============================================================
set(OpenCV_DIR "${CMAKE_SOURCE_DIR}/opencv")
include_directories("${OpenCV_DIR}/include")
link_directories("${OpenCV_DIR}/x64/vc16/lib")

file(GLOB OPENCV_LIBS "${OpenCV_DIR}/x64/vc16/lib/*.lib")
message(STATUS "✅ OpenCV libs trouvées : ${OPENCV_LIBS}")

file(GLOB OPENCV_DLLS "${OpenCV_DIR}/x64/vc16/bin/opencv_world*.dll")
if (OPENCV_DLLS)
    add_custom_command(TARGET PuissanceIV_QT POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${OPENCV_DLLS}
            $<TARGET_FILE_DIR:PuissanceIV_QT>
    )
endif()

# ============================================================
# === Dobot SDK
# ============================================================
set(DOBOT_DIR "${CMAKE_SOURCE_DIR}/Dobot")
include_directories("${DOBOT_DIR}")
link_directories("${DOBOT_DIR}")

add_custom_command(TARGET PuissanceIV_QT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${DOBOT_DIR}/DobotDll.dll"
        $<TARGET_FILE_DIR:PuissanceIV_QT>
)

# ============================================================
# === LIBTORCH (PyTorch C++ API)
# ============================================================
# 👉 La libtorch est placée à la racine du projet : ./libtorch
set(Torch_DIR "${CMAKE_SOURCE_DIR}/libtorch/share/cmake/Torch")
find_package(Torch REQUIRED)

include_directories("${CMAKE_SOURCE_DIR}/libtorch/include")
include_directories("${CMAKE_SOURCE_DIR}/libtorch/include/torch/csrc/api/include")
link_directories("${CMAKE_SOURCE_DIR}/libtorch/lib")

message(STATUS "✅ LibTorch trouvé : ${TORCH_LIBRARIES}")

# ============================================================
# === LIENS DES LIBRAIRIES
# ============================================================
target_link_libraries(PuissanceIV_QT
    PRIVATE
        Qt${QT_VERSION_MAJOR}::Widgets
        ${OPENCV_LIBS}
        "${DOBOT_DIR}/DobotDll.lib"
        ${TORCH_LIBRARIES}
)

# ============================================================
# === COPIE AUTOMATIQUE DES DLLs LIBTORCH
# ============================================================
add_custom_command(TARGET PuissanceIV_QT POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_SOURCE_DIR}/libtorch/lib"
        $<TARGET_FILE_DIR:PuissanceIV_QT>
)

# ============================================================
# === PROPRIÉTÉS DE L’EXÉCUTABLE
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

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(PuissanceIV_QT)
endif()
