#include <iostream>
#include <portaudio.h>
#include <cstring>
#include <algorithm>
#include <sndfile.h>
#include <sstream>
#include <string>
#include "FormatTools.h"
#include "logger.h"
#include "player_gapless.h"

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
    if (result != 0) {
        std::remove(tmpTemplate); // file isn't removed if we don't continue
        throw std::runtime_error("FFmpeg conversion failed");
    }
}

std::string formatToString(const int format) {
    // check with both FORMAT_SUBMASK and FORMAT_TYPEMASK, since this information is present in the same integer
    switch (( format & SF_FORMAT_SUBMASK | format & SF_FORMAT_TYPEMASK )) {
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
        case SF_FORMAT_WAV:    return "WAV";
    default: return "Unknown";
    }
}

AudioBuffer &AudioPlayer::getBuffer(const int f) {
    if (f==-1) {
        return lbuffIndex ? buff2 : buff1;
    }
    return (f==1) ? buff2 : buff1;
}

AudioPlayer::AudioPlayer() : logger(Logger("libkoulouri")), volume(0) {
    logger.log(Logger::Level::DEBUG, "Initializing...");
    buff1 = AudioBuffer(); // initialize first buffer for loading...
    Pa_Initialize();
}

AudioPlayer::~AudioPlayer() {
    logger.log(Logger::Level::DEBUG, "Killing all streams...");
    clear();
    logger.log(Logger::Level::DEBUG, "Terminating PortAudio...");
    Pa_Terminate();
    logger.log(Logger::Level::INFO, "Goodbye!");
}

PlayerActionResult AudioPlayer::load(const std::string& filePath, bool allowConversion, bool forceConversion, bool preload) {
    SF_INFO sfInfo;
    SNDFILE* file = nullptr;

    if (!forceConversion) {
        file = sf_open(filePath.c_str(), SFM_READ, &sfInfo);
    } else {
        logger.log(Logger::Level::INFO, "Forcing conversion!");
    }

    if (getBuffer().format == FormatType::Undefined && getBuffer().sampleRate == -1 && getBuffer().numChannels == -1) {
        // if the next buffer hasn't been initialized, we need to reconfigure regardless
        flags.reconfigureNeeded.store(true);
    } else {
        flags.reconfigureNeeded.store(stream != nullptr && (getBuffer().sampleRate != sfInfo.samplerate || getBuffer().numChannels != sfInfo.channels || getBuffer().format != FormatTools::fromLibsndfile(sfInfo.format)));
    }

    logger.log(Logger::Level::INFO, "Loading file: " + filePath);

    // file failed to open (as it is unsupported/unrecognized) and we're allowed to convert
    if ((!file && sf_error(NULL) == 1 && allowConversion) || forceConversion) {
        logger.log(Logger::Level::WARNING, "File is unsupported/unknown format!");
        logger.log(Logger::Level::INFO, "Attempting conversion via FFmpeg...");
        try {
            FfmpegFile converted_file(filePath);
            file = sf_open(converted_file.file().c_str(), SFM_READ, &sfInfo);
        } catch (std::runtime_error e){
            logger.log(Logger::Level::ERROR, "FFmpeg could not be located or failed to convert!");
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
        return PlayerActionResult(PlayerActionEnum::FAIL, "Unknown error!");
    }

    if (preload) {
        lbuffIndex = !lbuffIndex;
        logger.log(Logger::Level::DEBUG, "Preloading next track into buffer: " + std::to_string(lbuffIndex));
        flags.trackPreloaded.store(true);
    }

    sf_count_t totalFrames = sfInfo.frames;
    getBuffer().format = FormatTools::fromLibsndfile(sfInfo.format);


    // ALWAYS CALL .allocate!
    // AudioBuffer STORES AN INTERNAL VECTOR - FORMAT CHANGES WILL LEAD TO SEGFAULT!
    getBuffer().allocate(totalFrames * sfInfo.channels);

    logger.log(Logger::Level::INFO, "Buffer size is: " + std::to_string(getBuffer().size()));
    logger.log(Logger::Level::DEBUG, "Reading file...");

    sf_count_t framesRead = FormatReader::read(file, &getBuffer(), totalFrames, getBuffer().format);

    // If audio data made it, this is fine. We can simply adjust!
    if (framesRead != totalFrames) {
        std::string error = "Partial read! Expected " + std::to_string(totalFrames) + ", got " + std::to_string(framesRead);
        logger.log(Logger::Level::ERROR, error);
        logger.log(Logger::Level::WARNING, "File is likely corrupt or missing proper headers!");
        logger.log(Logger::Level::WARNING, "Trusting decoded data...");

        // adjust internal variables to match decoded data
        totalFrames = framesRead;
        getBuffer().resize((totalFrames*sfInfo.channels), true);

        logger.log(Logger::Level::DEBUG, "Buffer size is now: " + std::to_string(getBuffer().size()));
    }

    sf_close(file);
    getBuffer().sampleRate = sfInfo.samplerate;
    getBuffer().numChannels = sfInfo.channels;

    std::stringstream ss;
    ss << "Audio details are: Sample Rate: " << getBuffer().sampleRate
              << ", Channels: " << getBuffer().numChannels
              << ", Major format: " << formatToString(sfInfo.format & SF_FORMAT_TYPEMASK)
              << ", Sub format: " << formatToString(sfInfo.format & SF_FORMAT_SUBMASK) << ", read as " << formatTypeString[getBuffer().format];
    logger.log(Logger::Level::INFO, ss.str());

    _isLoaded = true;

    return PlayerActionResult(PlayerActionEnum::PASS);
}

PaStream* AudioPlayer::configureStream() {
    logger.log(Logger::Level::DEBUG, "Setting up stream...");
    PaStream* newStream = nullptr;

    PaStreamParameters outputParams;
    outputParams.device = Pa_GetDefaultOutputDevice();
    outputParams.channelCount = getBuffer().numChannels;
    outputParams.sampleFormat = FormatTools::toPortAudio[getBuffer().format];
    outputParams.suggestedLatency = Pa_GetDeviceInfo(outputParams.device)->defaultLowOutputLatency;
    outputParams.hostApiSpecificStreamInfo = nullptr;

    logger.log(Logger::Level::DEBUG, "Opening PortAudio stream...");
    Pa_OpenStream(&newStream, nullptr, &outputParams, getBuffer().sampleRate,
                  1024, paClipOff, audioCallback, this);
    // Automatically set the 'isComplete' flag once playback stops (unless paused)
    Pa_SetStreamFinishedCallback(newStream, [](void *userData) {
        if (auto* player = static_cast<AudioPlayer *>(userData); !player->_isPaused) {
            player->_isPlaying = false;
            player->flags.trackFinished.store(true);
        };
    });

    return newStream;
}

PlayerActionResult AudioPlayer::play() {
    if (getBuffer().empty()) return PlayerActionResult(PlayerActionEnum::NOTREADY, "Current audio buffer is empty. Nothing to play!");
    if (_isPlaying) return PlayerActionResult(PlayerActionEnum::NOTREADY, "Stop current playback first!");

    if (stream != nullptr) {
        if (flags.reconfigureNeeded.load()) {
            logger.log(Logger::Level::DEBUG, "Incoming differs! Reconfiguring stream...");
            Pa_StopStream(stream);
            Pa_CloseStream(stream);

            stream = nullptr;
            stream = configureStream();
        }
        logger.log(Logger::Level::DEBUG, "(Re)starting stream!");
        Pa_StartStream(stream);
    } else {
        logger.log(Logger::Level::DEBUG, "Starting new stream!");
        stream = configureStream();
        Pa_StartStream(stream);
    }

    _isPlaying = true;
    _isPaused = false;

    if (flags.trackPreloaded.load()) {
        setVPos(0);
        pbuffIndex = !pbuffIndex;
        flags.trackPreloaded.store(false);
    }

    // reset playing status flags
    flags.trackFinished.store(false);
    flags.trackAdvanced.store(false);

    return PlayerActionResult(PlayerActionEnum::PASS);
}

PlayerActionResult AudioPlayer::pause() {
    if (stream && _isPlaying) {
        logger.log(Logger::Level::DEBUG, "Pausing!");
        _isPaused = true;
        Pa_StopStream(stream);
        _isPlaying = false;
        return PlayerActionResult(true);
    }
    return PlayerActionResult(PlayerActionEnum::NOTREADY, "Stream is either closed or already paused!");
}

PlayerActionResult AudioPlayer::resume() {
    if (stream && !_isPlaying) {
        logger.log(Logger::Level::DEBUG, "Resuming!");
        _isPaused = false;
        flags.trackFinished.store(false); // resuming restarts the stream
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

double AudioPlayer::vposToSeconds(size_t vpos) {
    return static_cast<double>(std::clamp(vpos, std::size_t{0}, getBuffer(pbuffIndex).size())) / (getBuffer(pbuffIndex).sampleRate * getBuffer(pbuffIndex).numChannels);
};

size_t AudioPlayer::secondsToVPos(double secs) {
    return static_cast<size_t>(std::clamp(secs, 0.0, vposToSeconds(getBuffer(pbuffIndex).size())) * (getBuffer(pbuffIndex).sampleRate * getBuffer(pbuffIndex).numChannels));
};

void AudioPlayer::setVPos(size_t to) {
    playhead = std::clamp(to, std::size_t{0}, getBuffer(pbuffIndex).size());
}


void AudioPlayer::stop() {
    logger.log(Logger::Level::DEBUG, "Stopping current stream!");

    if (stream) {
        Pa_StopStream(stream);
    }
    if (getBuffer(pbuffIndex).empty()) {
        getBuffer(pbuffIndex).clear();
    }

    _isPlaying = false;
    _isLoaded = false;
    _isPaused = false;
    flags.trackAdvanced.store(false);
    flags.trackFinished.store(false);
    playhead = 0;
}

void AudioPlayer::clear() {
    logger.log(Logger::Level::DEBUG, "Clearing all buffers!");

    if (stream) {
        Pa_StopStream(stream);
        Pa_CloseStream(stream);
    }
    if (getBuffer(0).empty()) {
        getBuffer(0).clear();
    }
    if (getBuffer(1).empty()) {
        getBuffer(1).clear();
    }

    _isPlaying = false;
    _isLoaded = false;
    _isPaused = false;
    flags.trackAdvanced.store(false);
    flags.trackFinished.store(false);
    flags.trackPreloaded.store(false);
    flags.reconfigureNeeded.store(false);
    playhead = 0;
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
    if (player->getVPos() >= player->getBuffer(player->pbuffIndex).size()) {
        if (player->flags.trackPreloaded.load() && !player->flags.reconfigureNeeded.load()){
            // if preloaded, simply continue playing and notify the frontend
            player->flags.trackFinished.store(true);
            player->flags.trackAdvanced.store(true);
            player->flags.trackPreloaded.store(false);

            player->pbuffIndex = !player->pbuffIndex;
            player->setVPos(0);

            return paContinue;
        }

        return paComplete;
    }

    size_t samplesToWrite = framesPerBuffer * player->getBuffer(player->pbuffIndex).numChannels;
    size_t availableSamples = player->getBuffer(player->pbuffIndex).size() - player->getVPos();
    samplesToWrite = std::min(samplesToWrite, availableSamples);

    // choose a volume adjustment function based on the format.
    // done within this callback for simplicity - shouldn't affect processing speed?
    switch (player->getBuffer(player->pbuffIndex).format) {
        case FormatType::Int16: {
            int16_t* out = static_cast<int16_t*>(outputBuffer);
            const int16_t* in = &player->getBuffer(player->pbuffIndex).getInt16Buffer()[player->getVPos()];
            AudioTools::adjustVolumeInt16(in, out, samplesToWrite, player->getVolume());
            break;
        }
        case FormatType::Int24: // no native support - converted into padded Int32
            [[fallthrough]];
        case FormatType::Int32: {
            int32_t* out = static_cast<int32_t*>(outputBuffer);
            const int32_t* in = &player->getBuffer(player->pbuffIndex).getInt32Buffer()[player->getVPos()];
            AudioTools::adjustVolumeInt32(in, out, samplesToWrite, player->getVolume());
            break;
        }
        case FormatType::Float32: {
            float* out = static_cast<float*>(outputBuffer);
            const float* in = &player->getBuffer(player->pbuffIndex).getFloat32Buffer()[player->getVPos()];
            AudioTools::adjustVolumeFloat32(in, out, samplesToWrite, player->getVolume());
            break;
        }
    }

    player->setVPos(player->getVPos() + samplesToWrite);
    if (player->getVPos() >= player->getBuffer(player->pbuffIndex).size()) {
        if (player->flags.trackPreloaded.load() && !player->flags.reconfigureNeeded.load()){
            // if preloaded, simply continue playing and notify the frontend
            player->flags.trackFinished.store(true);
            player->flags.trackAdvanced.store(true);
            player->flags.trackPreloaded.store(false);

            player->pbuffIndex = !player->pbuffIndex;
            player->setVPos(0);

            return paContinue;
        }

        return paComplete;
    }

    return paContinue;
}
