#include <cstdlib>
#include <iostream>
#include <thread>

#include "curses_gui/cursesmainwindow.h"
#include "koulouri_shared/alsasilencer.h"
#include "libkoulouri/logger.h"

int main(int argc, const char **argv) {
    Logger log = Logger("main");
    std::cout << "Koulouri (TUI)" << std::endl;

    if (!getenv("KOULOURI_ALLOW_ALSAWHINE")) {
        AlsaSilencer::supressAlsa(); // on systems that support it, force ALSA to zip it with the PCM errors
    }

    std::string path = getenv("HOME");
    path += "/Desktop/koulouri.log";
    FileSink sink(path);

    // Logger::setOutput(&std::cout);
    Logger::setVerbosity(Logger::Level::DEBUG);
    Logger::setSink(&sink);
    if (true) {
        log.log(Logger::Level::CRITICAL, "i am become if statement... destroyer of scopes...");
    }

    log.log(Logger::Level::INFO, "Hello, world!");

    CursesMainWindow w;
    return w.main();
}
