cmake_minimum_required(VERSION 3.16)

project(RView VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --------------------------------------------------------------------
# Find libraries
find_package(
    QT
    NAMES
    Qt6 Qt5
    REQUIRED
    COMPONENTS Widgets LinguistTools OpenGL OpenGLWidgets
)

find_package(
    Qt${QT_VERSION_MAJOR}
    REQUIRED
    COMPONENTS Widgets LinguistTools OpenGL OpenGLWidgets
)

find_package(OpenCV REQUIRED)

# --------------------------------------------------------------------
# Set up the build environment
set(TS_FILES resource/RView_ja_JP.ts)

set(
    PROJECT_SOURCES
    src/main.cpp
    # -------------------------------------------------------
    # maincontrol
    include/maincontrol.h
    src/maincontrol.cpp
    # -------------------------------------------------------
    # mainwindow
    src/mainwindow.cpp
    src/mainwindow.ui
    include/mainwindow.h
    # -------------------------------------------------------
    # glwidget
    src/glwidget.cpp
    include/glwidget.h
    # -------------------------------------------------------
    # tranlation
    ${TS_FILES}
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(
        ${PROJECT_NAME}
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        src/filelistwidget.cpp
        include/filelistwidget.h
        include/fileutil.h
        src/fileutil.cpp
        include/filelistmodel.h
        src/filelistmodel.cpp
    )
    # Define target properties for Android with Qt 6 as:
    #    set_property(TARGET ReView APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
    #                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
    # For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
else()
    if(ANDROID)
        add_library(
            ${PROJECT_NAME}
            SHARED
            ${PROJECT_SOURCES}
        )
    # Define properties for Android with Qt 5 after find_package() calls as:
    #    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(
            ${PROJECT_NAME}
            ${PROJECT_SOURCES}
        )
    endif()

    qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
endif()

target_include_directories(
    ${PROJECT_NAME}
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    ${OpenCV_LIBS}
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.${PROJECT_NAME})
endif()
set_target_properties(
    ${PROJECT_NAME}
    PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(
    TARGETS
    ${PROJECT_NAME}
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(${PROJECT_NAME})
endif()
