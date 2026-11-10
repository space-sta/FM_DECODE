#include "FlacSampleReader.hpp"
#include <algorithm>
#include <exception>

FlacSampleReader::~FlacSampleReader() { close(); }

void FlacSampleReader::close() {
    if (decoder_) {
        FLAC__stream_decoder_finish(decoder_);
        FLAC__stream_decoder_delete(decoder_);
        decoder_ = nullptr;
    }
    pending_.clear();
    position_ = 0;
}

bool FlacSampleReader::open(const char* path) {
    close();
    error_.clear();
    bits_ = rate_ = 0;
    metadataSeen_ = false;
    decoder_ = FLAC__stream_decoder_new();
    if (!decoder_) {
        error_ = "Cannot allocate FLAC decoder";
        return false;
    }
    const auto status = FLAC__stream_decoder_init_file(
        decoder_, path, writeCallback, metadataCallback, errorCallback, this);
    if (status != FLAC__STREAM_DECODER_INIT_STATUS_OK) {
        error_ = FLAC__StreamDecoderInitStatusString[status];
        close();
        return false;
    }
    if (!FLAC__stream_decoder_process_until_end_of_metadata(decoder_) ||
        !metadataSeen_ || !error_.empty()) {
        if (error_.empty()) error_ = "Cannot read FLAC STREAMINFO";
        close();
        return false;
    }
    return true;
}

size_t FlacSampleReader::readSamples(float* dst, size_t count) {
    size_t copied = 0;
    while (decoder_ && error_.empty() && copied < count) {
        if (position_ < pending_.size()) {
            const size_t n = std::min(count - copied, pending_.size() - position_);
            std::copy_n(pending_.data() + position_, n, dst + copied);
            position_ += n;
            copied += n;
            continue;
        }
        pending_.clear();
        position_ = 0;
        if (FLAC__stream_decoder_get_state(decoder_) == FLAC__STREAM_DECODER_END_OF_STREAM)
            break;
        if (!FLAC__stream_decoder_process_single(decoder_)) {
            if (error_.empty())
                error_ = FLAC__stream_decoder_get_resolved_state_string(decoder_);
            break;
        }
    }
    return copied;
}

void FlacSampleReader::metadataCallback(const FLAC__StreamDecoder*,
    const FLAC__StreamMetadata* metadata, void* client) {
    auto& self = *static_cast<FlacSampleReader*>(client);
    if (metadata->type != FLAC__METADATA_TYPE_STREAMINFO) return;
    self.metadataSeen_ = true;
    self.bits_ = metadata->data.stream_info.bits_per_sample;
    self.rate_ = metadata->data.stream_info.sample_rate;
    if (metadata->data.stream_info.channels != 1)
        self.error_ = "RF input must be mono FLAC";
    // Preserve exact int16 -> float equivalence for the initial implementation.
    if (self.bits_ != 16)
        self.error_ = "RF input must be 16-bit FLAC (no implicit amplitude conversion)";
}

FLAC__StreamDecoderWriteStatus FlacSampleReader::writeCallback(
    const FLAC__StreamDecoder*, const FLAC__Frame* frame,
    const FLAC__int32* const input[], void* client) {
    auto& self = *static_cast<FlacSampleReader*>(client);
    if (!self.error_.empty()) return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    if (frame->header.channels != 1 || frame->header.bits_per_sample != 16) {
        self.error_ = "Unexpected FLAC frame format";
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    try {
        self.pending_.resize(frame->header.blocksize);
        self.position_ = 0;
        for (size_t i = 0; i < self.pending_.size(); ++i)
            self.pending_[i] = static_cast<float>(input[0][i]);
    } catch (const std::exception&) {
        self.error_ = "Cannot allocate FLAC sample buffer";
        return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;
    }
    return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
}

void FlacSampleReader::errorCallback(const FLAC__StreamDecoder*,
    FLAC__StreamDecoderErrorStatus status, void* client) {
    static_cast<FlacSampleReader*>(client)->error_ =
        FLAC__StreamDecoderErrorStatusString[status];
}
