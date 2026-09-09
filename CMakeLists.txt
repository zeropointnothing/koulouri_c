cmake_minimum_required(VERSION 3.16)

project(koulouri_c VERSION 0.0 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_subdirectory(libkoulouri) # music backend/utilities
add_subdirectory(koulouri_shared) # common code (ALSA silencer)

# find Qt
find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets)

# set up GUI compile targets
add_executable(koulouri_c-qt)
add_executable(koulouri_c-curses)
add_executable(koulouri_c-cli)
target_sources(koulouri_c-qt PRIVATE
        frontend/qt_gui/qtmainwindow.cpp
        frontend/qt_gui/qtmainwindow.h
        frontend/qt_gui/main.cpp
)
target_sources(koulouri_c-curses PRIVATE
        frontend/curses_gui/cursesmainwindow.cpp
        frontend/curses_gui/cursesmainwindow.h
        frontend/curses_gui/main.cpp
)
target_sources(koulouri_c-cli PRIVATE
    frontend/koulouri_cli/main.cpp
)

# set up testing compile target(s)
add_executable(koulouri_test)
target_sources(koulouri_test PRIVATE
    test.cpp
)

# organization
set_target_properties(koulouri_c-qt PROPERTIES FOLDER "GUI/Qt")
set_target_properties(koulouri_c-curses PROPERTIES FOLDER "GUI/Curses")

# link libraries
target_link_libraries(koulouri_c-qt PRIVATE libkoulouri koulouri_shared Qt${QT_VERSION_MAJOR}::Widgets)
target_link_libraries(koulouri_c-curses PRIVATE libkoulouri koulouri_shared ncurses)
target_link_libraries(koulouri_c-cli PRIVATE libkoulouri koulouri_shared)
target_link_libraries(koulouri_test PRIVATE libkoulouri)
