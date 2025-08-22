#include "FormatTools.h"

#include <algorithm>
#include <iostream>

// audio tools

// Adjust volume of an Int16 vector
void AudioTools::adjustVolumeInt16(const int16_t* input, int16_t* output, const size_t samples, const float volumePercent) {
    float gain = volumePercent / 100.0f;
    for (size_t i = 0; i < samples; ++i) {
        int scaled = static_cast<int>(input[i] * gain);
        output[i] = static_cast<int16_t>(std::clamp(scaled, INT16_MIN, INT16_MAX));
    }
}
// Adjust volume of an Int32 vector
void AudioTools::adjustVolumeInt32(const int32_t* input, int32_t* output, const size_t samples, const float volumePercent) {
    float gain = volumePercent / 100.0f;
    for (size_t i = 0; i < samples; ++i) {
        int64_t scaled = input[i] * gain;
        output[i] = static_cast<int32_t>(std::clamp(scaled, static_cast<int64_t>(INT32_MIN), static_cast<int64_t>(INT32_MAX)));
    }
}
// Adjust volume of an Float32 vector
void AudioTools::adjustVolumeFloat32(const float* input, float* output, const size_t samples, const float volumePercent) {
    float gain = volumePercent / 100.0f;
    for (size_t i = 0; i < samples; ++i) {
        float scaled = input[i] * gain;
        output[i] = std::clamp(scaled, -1.0f, 1.0f);
    }
}

// audiobuffer
/**
 * Create a new internal vector buffer based on the internal type.
 *
 * If the buffer exists already, it will be destroyed and replaced.
 * @param samples The size of the buffer
 */
void AudioBuffer::allocate(size_t samples) {
    switch (format) {
        case FormatType::Int16: {
                data = std::vector<int16_t>(samples);
                break;
            }
            case FormatType::Int24:
            [[fallthrough]];
            case FormatType::Int32: {
                data = std::vector<int32_t>(samples);
                break;
            case FormatType::Float32: {
                data = std::vector<float>(samples);
                break;
            }
        }
    }
}

/**
 * Get the internal vector's size.
 * @return The size of the internal vector
 */
size_t AudioBuffer::size() const {
    return std::visit([](const auto& vec) {
        return vec.size();
    }, data);
}

/**
 * Check if the internal vector is empty.
 * @return Whether the internal vector is empty
 */
bool AudioBuffer::empty() const {
    return std::visit([](const auto& vec) {
        return vec.empty();
    }, data);
}

/**
 * Clear the internal vector.
 *
 * NOTE: This does NOT deallocate the vector's memory, as it will remain the same size.
 * To free that memory, you must reallocate, resize, or delete this object.
 */
void AudioBuffer::clear() {
    std::visit([](auto& vec) {
        vec.clear();
    }, data);
}

/**
 * Resize the internal vector.
 *
 * NOTE: It is VERY important you ONLY call this function if the data you intend to fill it with
 * is the SAME TYPE as the internal vector! If this is not guaranteed, you should ALWAYS use .allocate instead!
 * @param samples The new size of the vector
 * @param shrink Whether to call shrink_to_fit on the vector afterwards
 */
void AudioBuffer::resize(size_t samples, const bool shrink) {
    std::visit([&](auto& vec) {
        vec.resize(samples);
        if (shrink) {
            vec.shrink_to_fit();
        }
    }, data);
}


// Return the internal vector in Int16 form.
std::vector<int16_t>& AudioBuffer::getInt16Buffer() {
    return std::get<std::vector<int16_t>>(data);
};
// Return the internal vector in Int32 form.
std::vector<int32_t>& AudioBuffer::getInt32Buffer() {
    return std::get<std::vector<int32_t>>(data);
};
// Return the internal vector in Float(32) form.
std::vector<float>& AudioBuffer::getFloat32Buffer() {
    return std::get<std::vector<float>>(data);
};


/**
 * Wrapper to read data into a buffer using libsndfile based on the FormatType.
 * @param file The file to read
 * @param buffer The buffer to read into
 * @param frames How much data to read
 * @param format The format of the file being read
 * @return How many frames were successfully read
 */
sf_count_t FormatReader::read(SNDFILE *file, AudioBuffer *buffer, sf_count_t frames, FormatType format) {
    switch (format) {
        // native support
        case FormatType::Int16: {
            return sf_readf_short(file, buffer->getInt16Buffer().data(), frames);
        }
        // no support - converted into Int32
        case FormatType::Int24:
        // native support
        case FormatType::Int32: {
            return sf_readf_int(file, buffer->getInt32Buffer().data(), frames);
        }
        // native support
        case FormatType::Float32: {
            return sf_readf_float(file, buffer->getFloat32Buffer().data(), frames);
        }
    }
    return 0;
}

// handler

/**
 * Get the FormatType of a file from its libsndfile format integer.
 * @param format The libsndfile format integer
 * @return The FormatType enum equivilant
 */
FormatType FormatTools::fromLibsndfile(const int format) {
    switch (const int subtype = format & SF_FORMAT_SUBMASK) {
        case SF_FORMAT_PCM_16:   return FormatType::Int16;
        case SF_FORMAT_PCM_24:   return FormatType::Int24;
        case SF_FORMAT_PCM_32:   return FormatType::Int32;
        case SF_FORMAT_MPEG_LAYER_III: [[fallthrough]]; // MP3 works better as float?
        case SF_FORMAT_FLOAT:    return FormatType::Float32;
        default:
            // for safety, default to Float32 as it's the most generous
            std::cerr << "!! Unknown format (defaulting to Float32): " << std::hex << subtype << std::dec << "\n";
            return FormatType::Float32; // fallback to something safe
    }
}

// Convert FormatType into a 'pa<format>' type.
std::map<FormatType, PaSampleFormat> FormatTools::toPortAudio = {
    {FormatType::Int16, paInt16},
    {FormatType::Int24, paInt32}, // padded 24 bit
    {FormatType::Int32, paInt32}, // true 32 bit
    {FormatType::Float32, paFloat32}
};
