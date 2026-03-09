#pragma once
#include <atomic>
#include <iostream>
#include <sndfile.h>
#include <portaudio.h>
#include <utility>
#include <map>
#include <unistd.h>

#include "FormatTools.h"
#include "logger.h"

class PlayerFlags {
    public:
    std::atomic<bool> trackFinished = false; // track finished playing
    std::atomic<bool> trackAdvanced = false; // libkoulouri started the preloaded track
    std::atomic<bool> trackPreloaded = false; // whether a track has been preloaded into the next buffer
    std::atomic<bool> reconfigureNeeded = false; // whether or not PortAudio needs to be reconfigured

};

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

    PlayerFlags flags = PlayerFlags();

    PlayerActionResult load(const std::string& filePath, bool allowConverision, bool forceConversion = false, bool preload = false);
    PlayerActionResult play();
    PlayerActionResult pause();
    PlayerActionResult resume();

    /**
     * Stop the currently playing buffer.
     *
     * This will stop playback, clear the buffer and reset the player state, without
     * affecting the next buffer.
     */
    void stop();
    /**
     * Stop and clear all buffers.
     *
     * Typically only useful when cleaning up. Does what .stop() does on all buffers.
     */
    void clear();


    void setVolume(int volume);
    void setVPos(size_t to);

    int getVolume();
    int getSampleRate() { return getBuffer(pbuffIndex).sampleRate; };
    int getChannels() { return getBuffer(pbuffIndex).numChannels; };

    bool isLoaded();
    bool isPlaying();

    size_t getVPos() const { return playhead; };
    size_t getMaxVPos() { return getBuffer(pbuffIndex).size(); };

    double vposToSeconds(size_t vpos);
    size_t secondsToVPos(double secs);

private:
    Logger logger;

    AudioBuffer &getBuffer(const int f = -1);
    PaStream* configureStream();


    static int audioCallback(const void *inputBuffer, void *outputBuffer,
                             unsigned long framesPerBuffer,
                             const PaStreamCallbackTimeInfo *timeInfo,
                             PaStreamCallbackFlags statusFlags,
                             void *userData);
    size_t playhead = 0;
    int volume;

    PaStream *stream = nullptr;
    bool _isPlaying = false; // playing audio - data loaded
    bool _isPaused = false; // not playing audio - data still loaded
    bool _isLoaded = false; // audio loaded

    int pbuffIndex = 0; // playback - 0=buff1, 1=buff2
    int lbuffIndex = 0; // loading - 0=buff1, 1=buff2

    AudioBuffer buff1; // primary buffer
    AudioBuffer buff2; // secondary buffer
};
