#ifndef ANALYZER_FFTF_H
#define ANALYZER_FFTF_H

#include <fftw3.h>
#include <complex>
#include <stdexcept>
#include <string>

namespace analyzer
{

/// FFTf: single-dimensional FFT helper for float input and FFTW single-precision output
class FFTf
{
private:
    fftwf_plan pl_fft;      ///< plan for real-to-complex FFT
    fftwf_plan pl_ifft;     ///< plan for complex-to-real IFFT
    fftwf_plan pl_fft_cx;   ///< plan for complex-to-complex FFT
    fftwf_plan pl_ifft_cx;  ///< plan for complex-to-complex IFFT
    unsigned int N;         ///< FFT length
    int alg;                ///< FFTW planner flag
    int export_alg;         ///< wisdom export planner flag

    static void error_handler(const char* msg)
    {
        throw std::runtime_error(msg);
    }

    static inline void conj_complex(const std::complex<float>& in, std::complex<float>& out)
    {
        out = std::conj(in);
    }

public:
    /// Constructor for 1D FFT
    explicit FFTf(unsigned int _N, int _alg = FFTW_ESTIMATE)
        : pl_fft(nullptr)
        , pl_ifft(nullptr)
        , pl_fft_cx(nullptr)
        , pl_ifft_cx(nullptr)
        , N(_N)
        , alg(_alg)
        , export_alg(FFTW_PATIENT)
    {
    }

    /// Destructor cleans up FFTW plans and global state
    ~FFTf()
    {
        if (pl_fft) fftwf_destroy_plan(pl_fft);
        if (pl_ifft) fftwf_destroy_plan(pl_ifft);
        if (pl_fft_cx) fftwf_destroy_plan(pl_fft_cx);
        if (pl_ifft_cx) fftwf_destroy_plan(pl_ifft_cx);
        fftwf_cleanup();
    }

    /// Compute complex FFT of complex input data
    /// @param x input complex array of length N
    /// @param Pxx output complex FFT array of length N
    void fft_cx(std::complex<float>* x, std::complex<float>* Pxx)
    {
        if (pl_fft_cx == nullptr)
        {
            pl_fft_cx = fftwf_plan_dft_1d(static_cast<int>(N),
                                          reinterpret_cast<fftwf_complex*>(x),
                                          reinterpret_cast<fftwf_complex*>(Pxx),
                                          FFTW_FORWARD,
                                          alg);
            if (pl_fft_cx == nullptr)
            {
                error_handler("Unable to create complex FFTW plan");
            }
        }
        fftwf_execute_dft(pl_fft_cx,
                          reinterpret_cast<fftwf_complex*>(x),
                          reinterpret_cast<fftwf_complex*>(Pxx));
    }

    /// Compute inverse complex FFT
    /// @param Pxx input complex spectrum of length N
    /// @param x output complex time-domain array of length N
    void ifft_cx(std::complex<float>* Pxx, std::complex<float>* x)
    {
        if (pl_ifft_cx == nullptr)
        {
            pl_ifft_cx = fftwf_plan_dft_1d(static_cast<int>(N),
                                           reinterpret_cast<fftwf_complex*>(Pxx),
                                           reinterpret_cast<fftwf_complex*>(x),
                                           FFTW_BACKWARD,
                                           alg);
            if (pl_ifft_cx == nullptr)
            {
                error_handler("Unable to create complex IFFTW plan");
            }
        }
        fftwf_execute_dft(pl_ifft_cx,
                          reinterpret_cast<fftwf_complex*>(Pxx),
                          reinterpret_cast<fftwf_complex*>(x));
                          
        float inv = 1.0f / N;
        for (unsigned int i = 0; i < N; ++i)
        {
            x[i] *= inv;
        }
    }

    /// Compute real-to-complex FFT from float input
    /// @param x input float array of length N
    /// @param Pxx output complex FFT array of length N
    void fft(float* x, std::complex<float>* Pxx)
    {
        if (pl_fft == nullptr)
        {
            pl_fft = fftwf_plan_dft_r2c_1d(static_cast<int>(N), x,
                                          reinterpret_cast<fftwf_complex*>(Pxx),
                                          alg);
            if (pl_fft == nullptr)
            {
                error_handler("Unable to create real FFTW plan");
            }
        }

        fftwf_execute_dft_r2c(pl_fft, x, reinterpret_cast<fftwf_complex*>(Pxx));

        unsigned int offset = (N + 1) / 2;
        unsigned int n_elem = N - offset;
        for (unsigned int i = 0; i < n_elem; ++i)
        {
            conj_complex(Pxx[n_elem - i], Pxx[offset + i]);
        }
    }

    /// Compute inverse FFT from complex spectrum to float output
    /// @param Pxx input complex spectrum of length N
    /// @param x output float array of length N
    void ifft(std::complex<float>* Pxx, float* x)
    {
        if (pl_ifft == nullptr)
        {
            pl_ifft = fftwf_plan_dft_c2r_1d(static_cast<int>(N),
                                           reinterpret_cast<fftwf_complex*>(Pxx),
                                           x,
                                           alg);
            if (pl_ifft == nullptr)
            {
                error_handler("Unable to create real IFFTW plan");
            }
        }
        fftwf_execute_dft_c2r(pl_ifft,
                              reinterpret_cast<fftwf_complex*>(Pxx),
                              x);
        float inv = 1.0f / N;
        for (unsigned int i = 0; i < N; ++i)
        {
            x[i] *= inv;
        }
    }

    /// Import wisdom from a string for FFTW planning
    void import_wisdom_string(const std::string& wisd)
    {
        if (fftwf_import_wisdom_from_string(wisd.c_str()) == 0)
            error_handler("Unable to import wisdom from string!");
    }

    /// Import wisdom from a file for FFTW planning
    void import_wisdom_file(const std::string& fname)
    {
        if (fftwf_import_wisdom_from_filename(fname.c_str()) == 0)
            error_handler("Unable to import wisdom from file!");
    }
};

} // namespace analyzer

#endif // ANALYZER_FFTF_H
