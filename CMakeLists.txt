cmake_minimum_required(VERSION 3.16)

project(RView VERSION 0.1 LANGUAGES C CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

# --------------------------------------------------------------------
# C++ settings

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE on)

include(InstallRequiredSystemLibraries)

# Set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

if(MSVC)
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /O2")
elseif(UNIX)
  set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -O0 -Wall")
  set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")
endif()

if (NOT ${CMAKE_BUILD_TYPE} STREQUAL "Release")
    add_definitions(-DRVIEW_DEBUG_BUILD)
    message(STATUS "Added \"-DRVIEW_DEBUG_BUILD\" to compiler flags")
endif()

# --------------------------------------------------------------------
# Find libraries
find_package(
    QT
    NAMES
    Qt6 Qt5
    REQUIRED
    COMPONENTS Core Widgets LinguistTools OpenGL OpenGLWidgets
)

find_package(
    Qt${QT_VERSION_MAJOR}
    REQUIRED
    COMPONENTS Core Widgets LinguistTools OpenGL OpenGLWidgets
)

# Find OpenCV
find_package(OpenCV REQUIRED)

# --------------------------------------------------------------------
# Third party libraries

# tinyxml2
add_subdirectory(
    third_party/tinyxml2-11.0.0
)

set(
    TINYXML2_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/tinyxml2-11.0.0
)
set(
    TINYXML2_LIBRARIES
    tinyxml2
)

# tinyexif
add_subdirectory(
    third_party/TinyEXIF
)

set(
    TINYEXIF_INCLUDE_DIRS
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/TinyEXIF
)
set(
    TINYEXIF_LIBRARIES
    TinyEXIFstatic
)

# --------------------------------------------------------------------
# Set up the build environment
set(
    TS_FILES
    resource/RView_ja_JP.ts
)

set(
    PROJECT_SOURCES
    src/main.cpp
    # -------------------------------------------------------
    # maincontrol
    include/maincontrol.h
    src/maincontrol.cpp
    # -------------------------------------------------------
    # mainwindow
    include/mainwindow.h
    src/mainwindow.cpp
    src/mainwindow.ui
    # -------------------------------------------------------
    # glwidget
    include/glwidget.h
    src/glwidget.cpp
    # -------------------------------------------------------
    # shaders
    include/shaders.h
    src/shaders.cpp
    # -------------------------------------------------------
    # image
    include/image.h
    src/image.cpp
    # --------------------------------------------------------
    # file util
    include/fileutil.h
    src/fileutil.cpp
    # --------------------------------------------------------
    # filelistwidget
    include/filelistwidget.h
    src/filelistwidget.cpp
    # --------------------------------------------------------
    # filelistmodel
    include/filelistmodel.h
    src/filelistmodel.cpp
    # --------------------------------------------------------
    # imageloader
    include/imageloader.h
    src/imageloader.cpp
    # --------------------------------------------------------
    # tranlation
    ${TS_FILES}
    # --------------------------------------------------------
    # common
    include/common.h
    # --------------------------------------------------------
    # resource
    resource/icon.rc
    resource/icons.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    if (APPLE)
        message(STATUS "Building for macOS")
        set(MACOSX_BUNDLE_ICON_FILE app.icns)
        set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/resource/app.icns")
        set_source_files_properties(
            ${app_icon_macos}
            PROPERTIES
            MACOSX_PACKAGE_LOCATION
            "Resources"
        )

        qt_add_executable(
            ${PROJECT_NAME}
            MACOSX_BUNDLE
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
            ${app_icon_macos}
        )
    else()
        qt_add_executable(
            ${PROJECT_NAME}
            MANUAL_FINALIZATION
            ${PROJECT_SOURCES}
        )
    endif()

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
    ${TINYXML2_INCLUDE_DIRS}
    ${TINYEXIF_INCLUDE_DIRS}
)

target_link_libraries(
    ${PROJECT_NAME}
    PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::OpenGL
    Qt${QT_VERSION_MAJOR}::OpenGLWidgets
    Qt${QT_VERSION_MAJOR}::Core
    ${OpenCV_LIBS}
    ${TINYXML2_LIBRARIES}
    ${TINYEXIF_LIBRARIES}
)


# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
    set(
        BUNDLE_ID_OPTION
        MACOSX_BUNDLE_GUI_IDENTIFIER
        com.example.${PROJECT_NAME}
    )
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

# Message
############################################################################################################
message(STATUS "# =======================================================================================================")
message(STATUS "#  '${PROJECT_NAME}' build configuration ")
message(STATUS "# =======================================================================================================")
message(STATUS "#  [CMake]")
message(STATUS "#    CMake version                        : ${CMAKE_VERSION}")
message(STATUS "#    CMake build type                     : ${CMAKE_BUILD_TYPE}")
message(STATUS "#    CMAKE_CXX_FLAGS                      : ${CMAKE_CXX_FLAGS}")
message(STATUS "#    CMAKE_CXX_FLAGS_DEBUG                : ${CMAKE_CXX_FLAGS_DEBUG}")
message(STATUS "#    CMAKE_CXX_FLAGS_RELEASE              : ${CMAKE_CXX_FLAGS_RELEASE}")
message(STATUS "# ")
message(STATUS "#  [C/C++]")
message(STATUS "#    C   Compiler                         : ${CMAKE_C_COMPILER_ID} | ${CMAKE_C_COMPILER_VERSION} | ${CMAKE_C_COMPILER}")
message(STATUS "#    C++ Compiler                         : ${CMAKE_CXX_COMPILER_ID} | ${CMAKE_CXX_COMPILER_VERSION} | ${CMAKE_CXX_COMPILER}")
message(STATUS "#    CXX STANDARD                         : ${CMAKE_CXX_STANDARD}")
message(STATUS "# ")
message(STATUS "#  [Cache Variables]")
message(STATUS "#    OpenCV_INCLUDE_DIRS                  : ${OpenCV_INCLUDE_DIRS}")
message(STATUS "#    OpenCV_LIBS                          : ${OpenCV_LIBS}")
message(STATUS "#    TINYXML2_INCLUDE_DIRS                : ${TINYXML2_INCLUDE_DIRS}")
message(STATUS "#    TINYXML2_LIBRARIES                   : ${TINYXML2_LIBRARIES}")
message(STATUS "#    TINYEXIF_INCLUDE_DIRS                : ${TINYEXIF_INCLUDE_DIRS}")
message(STATUS "#    TINYEXIF_LIBRARIES                   : ${TINYEXIF_LIBRARIES}")
message(STATUS "# =======================================================================================================")
