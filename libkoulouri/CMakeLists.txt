add_library(libkoulouri STATIC player.cpp metahandler.cpp
        FormatTools.cpp
        FormatTools.h)

find_package(PkgConfig REQUIRED)
pkg_check_modules(TAGLIB REQUIRED IMPORTED_TARGET taglib) # taglib doesn't provide a cmake file
target_link_libraries(libkoulouri PRIVATE portaudio sndfile PkgConfig::TAGLIB)

target_include_directories(libkoulouri PUBLIC "${CMAKE_SOURCE_DIR}/libkoulouri/..")
