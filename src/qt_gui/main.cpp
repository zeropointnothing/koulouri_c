#include <cstdlib>
#include <iostream>
#include <ostream>
#include <QApplication>

#include "qt_gui/qtmainwindow.h"
#include "koulouri_shared/alsasilencer.h"

int main(int argc, char *argv[]) {
    std::cout << "Koulouri (GUI)" << std::endl;

    if (!getenv("KOULOURI_ALLOW_ALSAWHINE")) {
        AlsaSilencer::supressAlsa(); // on systems that support it, force ALSA to zip it with the PCM errors
    }

    Logger log = Logger("main");
    Logger::setVerbosity(Logger::Level::DEBUG);
    Logger::setOutput(&std::cout);
    QApplication a(argc, argv);
    QtMainWindow w;

    log.log(Logger::Level::INFO, "Hello, world!");
    w.show();
    return a.exec();
    return 0;
}