#include <FLAC/stream_decoder.h>
#include <windows.h>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <atomic>
#include <cstring>
#include <iostream>

// ===== Ctrl+C 全局标志 =====
std::atomic<bool> g_stopRequested{false};

BOOL WINAPI ConsoleHandler(DWORD signal) {
    if (signal == CTRL_C_EVENT) {
        g_stopRequested = true;
        return TRUE;
    }
    return FALSE;
}

class FlacStreamer {
public:
    // ===== 参数 =====
    static constexpr size_t CHUNK = 31712;
    static constexpr size_t OVERLAP = 1056;
    static constexpr size_t FFT_SIZE = CHUNK + OVERLAP; // 32768
    static constexpr size_t QUEUE_SIZE = 8;

    bool open(const char* filename) {
        decoder = FLAC__stream_decoder_new();
        if (!decoder) return false;

        auto status = FLAC__stream_decoder_init_file(
            decoder,
            filename,
            write_callback_static,
            nullptr,
            error_callback_static,
            this
        );

        return status == FLAC__STREAM_DECODER_INIT_STATUS_OK;
    }

    void start() {
        running = true;

        worker = std::thread(&FlacStreamer::processingThread, this);

        FLAC__stream_decoder_process_until_end_of_stream(decoder);

        // 解码结束或被stop
        {
            std::lock_guard<std::mutex> lock(mtx);
            running = false;
        }

        cv.notify_all();
        worker.join();
    }

    void stop() {
        stopRequested = true;

        // if (decoder) {
        //     FLAC__stream_decoder_abort(decoder);
        // }

        cv.notify_all();
    }

    void close() {
        if (decoder) {
            FLAC__stream_decoder_finish(decoder);
            FLAC__stream_decoder_delete(decoder);
            decoder = nullptr;
        }
    }

private:
    FLAC__StreamDecoder* decoder = nullptr;

    // ===== 状态 =====
    std::atomic<bool> stopRequested{false};
    bool running = false;

    // ===== overlap =====
    float overlap[OVERLAP] = {0};

    // ===== chunk缓存 =====
    float temp_chunk[CHUNK];
    size_t temp_size = 0;

    // ===== 零分配环形队列 =====
    float queue[QUEUE_SIZE][FFT_SIZE];

    size_t write_idx = 0;
    size_t read_idx = 0;
    size_t count = 0;

    std::mutex mtx;
    std::condition_variable cv;

    std::thread worker;

    // ===== 回调桥接 =====
    static FLAC__StreamDecoderWriteStatus write_callback_static(
        const FLAC__StreamDecoder*,
        const FLAC__Frame* frame,
        const FLAC__int32* const inBuffers[],
        void* client_data)
    {
        return static_cast<FlacStreamer*>(client_data)
            ->write_callback(frame, inBuffers);
    }

    static void error_callback_static(
        const FLAC__StreamDecoder*,
        FLAC__StreamDecoderErrorStatus status,
        void*)
    {
        std::cerr << "FLAC error: "
                  << FLAC__StreamDecoderErrorStatusString[status]
                  << "\n";
    }

    // ===== 解码回调 =====
    FLAC__StreamDecoderWriteStatus write_callback(
        const FLAC__Frame* frame,
        const FLAC__int32* const inBuffers[])
    {
        if (stopRequested)
            return FLAC__STREAM_DECODER_WRITE_STATUS_ABORT;

        const int blocksize = frame->header.blocksize;
        const FLAC__int32* in = inBuffers[0]; // 单声道

        for (int i = 0; i < blocksize; ++i) {
            temp_chunk[temp_size++] = in[i] / 32768.0f;

            if (temp_size == CHUNK) {
                pushFFTBlock(temp_chunk);
                temp_size = 0;
            }
        }

        return FLAC__STREAM_DECODER_WRITE_STATUS_CONTINUE;
    }

    // ===== 生产 FFT block =====
    void pushFFTBlock(const float* newData) {
        std::unique_lock<std::mutex> lock(mtx);

        cv.wait(lock, [this] {
            return count < QUEUE_SIZE || stopRequested;
        });

        if (stopRequested) return;

        float* dst = queue[write_idx];

        // overlap + new
        memcpy(dst, overlap, OVERLAP * sizeof(float));
        memcpy(dst + OVERLAP, newData, CHUNK * sizeof(float));

        // 更新 overlap
        memcpy(overlap,
               newData + (CHUNK - OVERLAP),
               OVERLAP * sizeof(float));

        write_idx = (write_idx + 1) % QUEUE_SIZE;
        count++;

        lock.unlock();
        cv.notify_one();
    }

    // ===== 消费线程 =====
    void processingThread() {
        while (true) {
            float* data = nullptr;

            {
                std::unique_lock<std::mutex> lock(mtx);

                cv.wait(lock, [this] {
                    return count > 0 || stopRequested;
                });

                if (stopRequested && count == 0)
                    break;

                if (count > 0) {
                    data = queue[read_idx];
                    read_idx = (read_idx + 1) % QUEUE_SIZE;
                    count--;
                }
            }

            cv.notify_one();

            if (data)
                processChunk(data);
        }
    }

    // ===== 你的FFT处理 =====
    void processChunk(float* data) {
        // 长度 = 32768

        std::cout << "FFT block ready\n";

        // 👉 在这里接 FFTW
        // fftwf_execute(plan);
    }
};

// ===== main =====
int main() {
    SetConsoleCtrlHandler(ConsoleHandler, TRUE);

    FlacStreamer decoder;

    if (!decoder.open("test.flac")) {
        std::cerr << "Open failed\n";
        return -1;
    }

    std::thread t([&] {
        decoder.start();
    });

    // 主线程监控 Ctrl+C
    while (!g_stopRequested) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::cout << "\nCtrl+C detected, stopping...\n";

    decoder.stop();

    t.join();
    decoder.close();

    std::cout << "Exited cleanly.\n";
    return 0;
}