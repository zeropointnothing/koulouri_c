#include <cstdlib>
#include <iostream>
#include <iterator>

#include "koulouri_shared/alsasilencer.h"

int main(int argc, const char **argv) {
    std::cout << "Koulouri (TUI)" << std::endl;

    if (!getenv("KOULOURI_ALLOW_ALSAWHINE")) {
        AlsaSilencer::supressAlsa(); // on systems that support it, force ALSA to zip it with the PCM errors
    }
}