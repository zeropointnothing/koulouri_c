#include "alsasilencer.h"
#include <csignal>
#include <iostream>

#ifdef HAS_ALSA
#include <alsa/asoundlib.h>
extern "C" void silentAlsaHandler(const char*, int, const char*, int, const char*, ...) {
    // Shut up, ALSA!!
}
#endif

/*
 * If 'HAS_ALSA' is set to 1 (could be found by CMake), this function will attempt to overwrite
 * ALSA's error handler to silence it completely.
 *
 * To undo this (reset to default), simply call 'snd_lib_error_set_handler(nullptr)',
 * though let's be honest, you likely don't want to do that. ;)
 */
void AlsaSilencer::supressAlsa() {
#ifdef HAS_ALSA
    snd_lib_error_set_handler(silentAlsaHandler);
#endif
}



