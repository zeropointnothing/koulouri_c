#include "player.h"
#include <iostream>
#include <portaudio.h>
#include <cstring>
#include <algorithm>
#include <sstream>
#include "logger.h"

FfmpegFile::FfmpegFile(const std::string &inputPath) {
    // Create temp file
    char tmpTemplate[] = "/tmp/koulouriconv_XXXXXX";
    int fd = mkstemp(tmpTemplate);
    if (fd == -1) throw std::runtime_error("Failed to initiate FFmpeg conversion - /tmp file creation failed!");

    close(fd);  // We'll let FFmpeg write to it
    tempPath = tmpTemplate;

    // Run FFmpeg to convert inputPath to WAV
    std::string command = "ffmpeg -i \"" + inputPath + "\" -f wav -y \"" + tempPath + "\" 2>&1";
    FILE* pipe = popen(command.c_str(), "r");
    if (!pipe) throw std::runtime_error("Failed to start FFmpeg process");

    char buffer[256];
    while (fgets(buffer, sizeof(buffer), pipe) != nullptr) {
        std::string line(buffer);
        if (!line.empty() && line.back() == '\n') {
            line.pop_back();  // Remove trailing newline
        }
        Logger::g_log("libkoulouri", Logger::Level::DEBUG, "ffmpeg", line);
    }

    int result = pclose(pipe);
    if (result != 0) throw std::runtime_error("FFmpeg conversion failed");

}


/**
 * @brief Creates the AudioPlayer and initializes PortAudio.
 *
 * Note, while an internal logger instance is created here, it is your responsibility to instruct libkoulouri how
 * and where it should log.
 */
AudioPlayer::AudioPlayer() : logger(Logger("libkoulouri")), stream(nullptr), _isPlaying(false), _isLoaded(false), volume(0) {
    logger.log(Logger::Level::DEBUG, "initializing PortAudio...");
    // create the buffer (to prep it for allocate)
    rawAudio = AudioBuffer();
    Pa_Initialize();
}

/**
 * @brief If the stream hasn't been stopped, stops it and terminates PortAudio.
 *
 */
AudioPlayer::~AudioPlayer() {
    logger.log(Logger::Level::DEBUG, "quitting PortAudio...");

    stop();
    Pa_Terminate();
}

std::string formatToString(int format) {
    switch (format & SF_FORMAT_SUBMASK) {
    case SF_FORMAT_PCM_16: return "PCM 16-bit";
    case SF_FORMAT_PCM_24: return "PCM 24-bit";
    case SF_FORMAT_PCM_32: return "PCM 32-bit";
    case SF_FORMAT_FLOAT:  return "Float 32-bit";
    case SF_FORMAT_DOUBLE: return "Float 64-bit";
    case SF_FORMAT_ULAW:   return "μ-law";
    case SF_FORMAT_ALAW:   return "A-law";
    case SF_FORMAT_MPEG_LAYER_III:    return "MP3";
    case SF_FORMAT_VORBIS: return "Ogg Vorbis";
    case SF_FORMAT_FLAC:   return "FLAC";
    default: return "Unknown";
    }
}

/**
 * @brief Loads a sound file to memory.
 *
 * Does not automatically play the file.
 */
PlayerActionResult AudioPlayer::load(const std::string& filePath, bool allowConverision) {
    SF_INFO sfInfo;
    SNDFILE* file = sf_open(filePath.c_str(), SFM_READ, &sfInfo);

    // file failed to open (as it is unsupported/unrecognized) and we're allowed to convert
    if (!file && sf_error(NULL) == 1 && allowConverision) {
        logger.log(Logger::Level::DEBUG, "Attempting conversion via FFmpeg...");
        try {
            FfmpegFile converted_file(filePath);
            file = sf_open(converted_file.file().c_str(), SFM_READ, &sfInfo);
        } catch (std::runtime_error e){
            std::string msg = "FFmpeg could not be located or it failed to convert the file. | ";
            msg.append(e.what()).append("");
            return PlayerActionResult(PlayerActionEnum::FAIL, msg);
        }
    }

    if (!file) {
        int code = sf_error(nullptr);
        std::string msg = "Failed to open file: ";
        msg += sf_strerror(nullptr);
        logger.log(Logger::Level::ERROR, msg);

        if (code == 2) {
            PlayerActionResult res = PlayerActionResult(PlayerActionEnum::NOTFOUND, "No such file exists or it could not be read!");
            return res;
        }
        if (code == 1) {
            return PlayerActionResult(PlayerActionEnum::NOTSUPPORTED, "File format is either unknown or unsupported!");
        }
    }

    // Fetch metadata
    // std::cout << (sf_get_string(file, SF_STR_TITLE)? : "not available") << std::endl;
    // std::cout << (sf_get_string(file, SF_STR_ALBUM)? : "not available") << std::endl;
    // std::cout << (sf_get_string(file, SF_STR_ARTIST)? : "not available") << std::endl;
    // std::cout << (sf_get_string(file, SF_STR_GENRE)? : "not available") << std::endl;

    sf_count_t totalFrames = sfInfo.frames;
    format = FormatTools::fromLibsndfile(sfInfo.format);

    // ALWAYS CALL .allocate!
    // AudioBuffer STORES AN INTERNAL VECTOR - FORMAT CHANGES WILL LEAD TO SEGFAULT!
    rawAudio.format = format; // <- DO NOT CHANGE THIS LINE - AudioBuffer HOLDS ITS OWN COPY!
    rawAudio.allocate(totalFrames * sfInfo.channels);

    logger.log(Logger::Level::DEBUG, "Final rawAudio vector size is: " + std::to_string(rawAudio.size()));

    // Read all samples into rawAudio
    sf_count_t framesRead = FormatReader::read(file, &rawAudio, totalFrames, format);

    if (framesRead != totalFrames) {
        std::string error = "Partial read: expected " + std::to_string(totalFrames) + ", got " + std::to_string(framesRead);
        logger.log(Logger::Level::ERROR, error);
        sf_close(file);
        return PlayerActionResult(PlayerActionEnum::FAIL, error);
    }

    // Manually clamp the floats into int16 (should ensure we avoid clipping).
    // for (size_t i = 0; i < floatBuffer.size(); ++i) {
    //     float sample = std::clamp(floatBuffer[i], -1.0f, 1.0f);
    //     rawAudio[i] = static_cast<int16_t>(sample * 32767.0f);
    // }

    sf_close(file);
    this->sampleRate = sfInfo.samplerate;
    this->numChannels = sfInfo.channels;
    this->playbackSize = rawAudio.size();

    std::stringstream ss;
    ss << "Audio details are: Sample Rate: " << sampleRate
              << ", Channels: " << numChannels
              << ", Major format: " << formatToString(sfInfo.format & SF_FORMAT_TYPEMASK)
              << ", Sub format: " << formatToString(sfInfo.format & SF_FORMAT_SUBMASK) << ", read as " << formatTypeString[format];
    logger.log(Logger::Level::INFO, ss.str());

    _isLoaded = true;

    return PlayerActionResult(PlayerActionEnum::PASS);
}


/**
 * @brief Assuming a file has been loaded via `load()`, plays the file.
 *
 * Volume should be set first, as it defaults to 0.
 */
PlayerActionResult AudioPlayer::play() {
    if (rawAudio.empty()) return PlayerActionResult(PlayerActionEnum::NOTREADY, "Current audio buffer is empty. Nothing to play!");

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = numChannels;
    outputParams.sampleFormat = FormatTools::toPortAudio[format];
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    Pa_OpenStream(&stream, nullptr, &outputParams, sampleRate,
                  1024, paClipOff, audioCallback, this);
    // Automatically set the 'isComplete' flag once playback stops (unless paused)
    Pa_SetStreamFinishedCallback(stream, [](void *userData) {
        if (auto* player = static_cast<AudioPlayer *>(userData); !player->_isPaused) {
            player->_isComplete = true;
        };
    });
    Pa_StartStream(stream);
    _isPlaying = true; // audio playback starts here
    _isPaused = false; // this resets the paused state anyway
    _isComplete = false; // audio (shouldn't) be marked 'complete' after playback start

    return PlayerActionResult(true);
}

PlayerActionResult AudioPlayer::pause() {
    if (stream && _isPlaying) {
        _isPaused = true;
        Pa_StopStream(stream);
        _isPlaying = false;
        return PlayerActionResult(true);
    }
    return PlayerActionResult(PlayerActionEnum::NOTREADY, "Stream is either closed or already paused!");
}

PlayerActionResult AudioPlayer::resume() {
    if (stream && !_isPlaying) {
        _isPaused = false;
        _isComplete = false; // resuming restarts the stream
        Pa_StartStream(stream);
        _isPlaying = true;
        return PlayerActionResult(true);
    }
    return PlayerActionResult(PlayerActionEnum::NOTREADY, "Stream is either closed or already playing!");
}

void AudioPlayer::setVolume(int volume) {
    if (volume > 100) {
        volume = 100;
    } else if (volume < 0) {
        volume = 0;
    }

    if (this->volume != volume) {
        this->volume = volume;

        // processAudioData();
        logger.log(Logger::Level::DEBUG, "adjusting volume to: " + std::to_string(volume));
    } else {
        logger.log(Logger::Level::DEBUG, "volume did not update, for it was already: " + std::to_string(volume));
    }
}

int AudioPlayer::getVolume() {
    return this->volume;
}

bool AudioPlayer::isPlaying() {
    return this->_isPlaying;
}

bool AudioPlayer::isLoaded() {
    return this->_isLoaded;
}

bool AudioPlayer::isCompleted() {
    return this->_isComplete;
}


void AudioPlayer::stop() {
    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
        stream = nullptr;
    }
    if (!rawAudio.empty()) {
        rawAudio.clear();
    }

    _isPlaying = false;
    _isLoaded = false;
    _isPaused = false;
    _isComplete = false; // no data - should pretend we aren't complete for safety
    playbackPos = 0; // reset the 'play head'
}


int AudioPlayer::audioCallback(
    const void *inputBuffer,
    void *outputBuffer,
    unsigned long framesPerBuffer,
    const PaStreamCallbackTimeInfo *timeInfo,
    PaStreamCallbackFlags statusFlags,
    void *userData
    ) {
    AudioPlayer* player = static_cast<AudioPlayer*>(userData);

    // Despite the return call, none of the code following this statement is safe to run if the playbackPos
    // is greater than or equal to the size of the internal buffer. Thus, we should safely quit here by signalling to
    // PortAudio that we've completed the playback.
    if (player->getPos() >= player->rawAudio.size()) {
        return paComplete;
    }

    size_t samplesToWrite = framesPerBuffer * player->numChannels;
    size_t availableSamples = player->rawAudio.size() - player->getPos();
    samplesToWrite = std::min(samplesToWrite, availableSamples);

    // choose a volume adjustment function based on the format.
    // done within this callback for simplicity - shouldn't affect processing speed?
    switch (player->format) {
        case FormatType::Int16: {
            int16_t* out = static_cast<int16_t*>(outputBuffer);
            const int16_t* in = &player->rawAudio.getInt16Buffer()[player->getPos()];
            AudioTools::adjustVolumeInt16(in, out, samplesToWrite, player->getVolume());
            break;
        }
        case FormatType::Int24: // no native support - converted into padded Int32
            [[fallthrough]];
        case FormatType::Int32: {
            int32_t* out = static_cast<int32_t*>(outputBuffer);
            const int32_t* in = &player->rawAudio.getInt32Buffer()[player->getPos()];
            AudioTools::adjustVolumeInt32(in, out, samplesToWrite, player->getVolume());
            break;
        }
        case FormatType::Float32: {
            float* out = static_cast<float*>(outputBuffer);
            const float* in = &player->rawAudio.getFloat32Buffer()[player->getPos()];
            AudioTools::adjustVolumeFloat32(in, out, samplesToWrite, player->getVolume());
            break;
        }
    }

    player->setPos(player->getPos() + samplesToWrite);
    return (player->getPos() >= player->rawAudio.size()) ? paComplete : paContinue;
}
// int AudioPlayer::audioCallback(const void *inputBuffer, void *outputBuffer, unsigned long framesPerBuffer, const PaStreamCallbackTimeInfo *timeInfo, PaStreamCallbackFlags statusFlags, void *userData) {
//     AudioPlayer* player = static_cast<AudioPlayer*>(userData);
//     float* out = static_cast<float*>(outputBuffer);
//
//     size_t samplesToWrite = framesPerBuffer * player->numChannels;
//     size_t availableSamples = player->rawAudio.size() - player->getPos();
//     samplesToWrite = std::min(samplesToWrite, availableSamples);
//
//     const float* raw = &player->rawAudio.getFloat32Buffer()[player->getPos()];
//     float gain = player->volume / 100.0f;
//     for (size_t i = 0; i < samplesToWrite; ++i) {
//         float scaled = raw[i] * gain;
//         out[i] = std::clamp(scaled, -1.0f, 1.0f); // assuming normalized float audio
//     }
//
//     player->setPos(player->getPos() + samplesToWrite);
//     return (player->getPos() >= player->rawAudio.size()) ? paComplete : paContinue;
//
// }




void AudioPlayer::print(std::string text) {
    std::cout << text << std::endl;
}
