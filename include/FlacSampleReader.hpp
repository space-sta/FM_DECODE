#pragma once

#include <FLAC/stream_decoder.h>
#include <cstddef>
#include <string>
#include <vector>

// Synchronous native-FLAC reader. Samples retain their signed integer magnitude.
class FlacSampleReader {
public:
    ~FlacSampleReader();
    FlacSampleReader() = default;
    FlacSampleReader(const FlacSampleReader&) = delete;
    FlacSampleReader& operator=(const FlacSampleReader&) = delete;

    bool open(const char* path);
    void close();
    size_t readSamples(float* dst, size_t count);
    const std::string& error() const { return error_; }
    unsigned bitsPerSample() const { return bits_; }
    unsigned containerSampleRate() const { return rate_; }

private:
    static FLAC__StreamDecoderWriteStatus writeCallback(
        const FLAC__StreamDecoder*, const FLAC__Frame*,
        const FLAC__int32* const[], void*);
    static void metadataCallback(const FLAC__StreamDecoder*,
                                 const FLAC__StreamMetadata*, void*);
    static void errorCallback(const FLAC__StreamDecoder*,
                             FLAC__StreamDecoderErrorStatus, void*);

    FLAC__StreamDecoder* decoder_ = nullptr;
    std::vector<float> pending_;
    size_t position_ = 0;
    unsigned bits_ = 0, rate_ = 0;
    bool metadataSeen_ = false;
    std::string error_;
};
