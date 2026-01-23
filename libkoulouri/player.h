#pragma once
#include <algorithm>
#include <iostream>
#include <sndfile.h>
#include <portaudio.h>
#include <utility>
#include <vector>
#include <map>
#include <unistd.h>

#include "FormatTools.h"
#include "logger.h"

enum class PlayerActionEnum {
    /**
     * Generic 'PASS'.
     */
    PASS,
    /**
     * Generic 'FAIL'.
     */
    FAIL,
    /**
     * The requested resource could not be found.
     */
    NOTFOUND,
    /**
     * The requested resource it not ready.
     */
    NOTREADY,
    /**
     * The requested resource is not of a supported type.
     */
    NOTSUPPORTED
};

/**
 * Custom 'result' class that contains whether the operation was succesful
 * and a message otherwise.
 *
 * If `result` is anything except PASS, assume the operation has failed.
 * Likewise, if `result` is PASS, assume no message has been supplied.
 */
class PlayerActionResult {
public:
    PlayerActionEnum result;
    std::string message;

    explicit PlayerActionResult(const PlayerActionEnum result, std::string message = "") {
        this->result = result;
        this->message = std::move(message);
    }

    operator bool() const {
        return result == PlayerActionEnum::PASS;
    };

    /**
         * Creates a 'friendly' string representation of the result.
         */
    std::string getFriendly() const {
        std::map<PlayerActionEnum, std::string> resultNames = {
            {PlayerActionEnum::PASS, "PASS"},
            {PlayerActionEnum::FAIL, "FAIL"},
            {PlayerActionEnum::NOTFOUND, "NOT_FOUND"},
            {PlayerActionEnum::NOTREADY, "NOT_READY"},
            {PlayerActionEnum::NOTSUPPORTED, "NOT_SUPPORTED"}
        };

        try {
            return resultNames.at(this->result) + " : " + this->message;
        } catch (std::out_of_range &e) {
            std::cerr << "WARNING!! BAD RESULT TYPE! FIX THIS ASAP!!" << std::endl;
            return "UNKNOWN";
        }
    }

    /**
         * Boolean cast, allowing for simpler true/false results.
         */
    explicit PlayerActionResult(bool success) : result(success ? PlayerActionEnum::PASS : PlayerActionEnum::FAIL) {};
};


class FfmpegFile {
public:
    explicit FfmpegFile(const std::string &inputPath);

    const std::string &file() const {
        return tempPath;
    }

    ~FfmpegFile() {
        if (!tempPath.empty()) {
            unlink(tempPath.c_str());
        }
    }

private:
    std::string tempPath;
};

class AudioPlayer {
public:
    AudioPlayer();
    ~AudioPlayer();

    PlayerActionResult load(const std::string& filePath, bool allowConverision, bool forceConversion = false);
    PlayerActionResult play();
    PlayerActionResult pause();
    PlayerActionResult resume();
    void stop();
    void setVolume(int volume);
    int getVolume();
    bool isLoaded();
    bool isCompleted();
    bool isPlaying();

    size_t getPos() const { return playbackPos; };
    double posToSeconds(size_t raw) const {
        return static_cast<double>(std::clamp(raw, std::size_t{0}, playbackSize)) / (sampleRate * numChannels);
    };
    size_t secondsToPos(double seconds) const {
        return static_cast<size_t>(std::clamp(seconds, 0.0, posToSeconds(playbackSize)) * (sampleRate * numChannels));
    };

    void setPos(size_t to) {
        playbackPos = std::clamp(to, std::size_t{0}, rawAudio.size());
    };
    size_t getMaxPos() const { return playbackSize; };

    int getSampleRate() const { return sampleRate; };
    int getChannels() const { return numChannels; };

    void print(std::string text);

private:
    Logger logger;

    static int audioCallback(const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo *timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData);
    size_t playbackPos = 0;
    size_t playbackSize = 0;


    PaStream *stream;
    bool _isPlaying; // playing audio - data loaded
    bool _isComplete; // done playing audio - data still loaded
    bool _isPaused; // not playing audio - data still loaded
    bool _isLoaded; // audio loaded
    std::string currentPath;

    // std::vector<int16_t> rawAudio;
    AudioBuffer rawAudio;
    // using VolumeCallback = std::function<void(const void* input, void* output, size_t samples, float volume)>;
    // std::function<void(const void* input, void* output, size_t samples, float volume)> volumeCallback;
    int sampleRate;
    int numChannels;
    int volume;
    FormatType format;
};
