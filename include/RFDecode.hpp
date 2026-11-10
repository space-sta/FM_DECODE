#ifndef RFDECODE_HPP
#define RFDECODE_HPP

#include "MatSave.hpp"
#include "utill.hpp"
#include <armadillo>
#include "sigpack.h"   // Sigpack 主头文件
#include "fftf.hpp"

namespace FMdecode {

using arma::fill::none;

class RFDecode {
public:
    RFDecode(int length, float infreq);
    ~RFDecode();
    
    void process(float* input, float* output);
private:
    int length_;
    float infreq_;
    // 其他成员变量，如滤波器系数等

    analyzer::FFTf fft{BLOCKSIZE};
    arma::cx_fvec::fixed<BLOCKSIZE> cx_out{none}, 
                                    out_hilbert{none};
    arma::fvec::fixed<BLOCKSIZE> RFVideo{none},
                                hilbert{none},
                                abs_h_f_l{none};
    arma::cx_fvec::fixed<BLOCKSIZE> deemp{none},
                                    h_05{none};

};

} // namespace FMdecode

#endif // RFDECODE_HPP