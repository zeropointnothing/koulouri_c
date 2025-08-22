#pragma once
#include <functional>
#include <map>
#include <portaudio.h>
#include <sndfile.h>
#include <string>
#include <variant>
#include <vector>

enum class FormatType {
    Int16,
    Int24,
    Int32,
    Float32
};

using RawBuffer = std::variant<
    std::vector<int16_t>,
    std::vector<int32_t>,
    std::vector<float>
>;

static std::map<FormatType, std::string> formatTypeString = {
    {FormatType::Int16, "16-bit int"},
    {FormatType::Int24, "24-bit int (converted to 32-bit int)"},
    {FormatType::Int32, "32-bit int"},
    {FormatType::Float32, "32-bit float"}
};

class AudioTools {
public:
    static void adjustVolumeInt16(const int16_t* input, int16_t* output, size_t samples, float volumePercent);
    static void adjustVolumeInt32(const int32_t* input, int32_t* output, size_t samples, float volumePercent);
    static void adjustVolumeFloat32(const float* input, float* output, size_t samples, float volumePercent);
};

class AudioBuffer {
    public:
    FormatType format;
    RawBuffer data;

    std::vector<int16_t>& getInt16Buffer();
    std::vector<int32_t>& getInt32Buffer();
    std::vector<float>& getFloat32Buffer();

    void allocate(size_t samples);
    [[nodiscard]] size_t size() const;
    [[nodiscard]] bool empty() const;
    void clear();
    void resize(size_t samples, bool shrink);

    private:

};

class FormatReader {
public:
    static sf_count_t read(SNDFILE* file, AudioBuffer* buffer, sf_count_t frames, FormatType format);
};

class FormatTools {
public:
    static std::map<FormatType, PaSampleFormat> toPortAudio;
    static FormatType fromLibsndfile(int format);
};
