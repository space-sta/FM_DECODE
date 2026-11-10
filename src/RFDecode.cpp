#include "RFDecode.hpp"
#include "fileAdress.hpp"

namespace FMdecode {
    RFDecode::RFDecode(int length, float infreq) : length_(length),infreq_(infreq) 
    {
        // 初始化成员变量，如分配内存、设置滤波器系数等
        ReadFromMat((FilterDir+"RFVideo_f.mat").c_str(),    "mag_s",      RFVideo.memptr(),BLOCKSIZE);
        // ReadFromMat((FilterDir+"hilbert.mat").c_str(),    "hilbert",  hilbert.memptr(),BLOCKSIZE);
        ReadFromMat((FilterDir+"deemp_f.mat").c_str(),      "h_deemp_s",  deemp.memptr(),BLOCKSIZE);
        ReadFromMat((FilterDir+"abs_h_f_l_f.mat").c_str(),  "abs_h_f_l_s",abs_h_f_l.memptr(),BLOCKSIZE);
        ReadFromMat((FilterDir+"h_05_f.mat").c_str(),       "h_05_s",     h_05.memptr(),BLOCKSIZE);

        // 设计 Hilbert 滤波器
        std::fill_n(hilbert.memptr()+1, BLOCKSIZE/2, 2.0f);
        std::fill_n(hilbert.memptr() + BLOCKSIZE/2 + 1, BLOCKSIZE/2-1, 0.0f);
        hilbert[0]=1.0f;

    }

    void RFDecode::process(float *input, float *output)
    {
        // 实现 FM 解调的核心处理逻辑
        // 1. 对输入信号进行 Hilbert 变换，得到解析信号
        // 2. 计算解析信号的相位，并进行解包络处理
        // 3. 应用去加重滤波器
        // 4. 输出解调后的音频信号

        uword ii;

        fft.fft(input,cx_out.memptr());

        for ( ii = 0; ii < BLOCKSIZE/2+1; ++ii)
        {
            cx_out[ii]*=(RFVideo[ii]*hilbert[ii]);
        }

        std::fill_n(cx_out.memptr() + BLOCKSIZE/2 + 1, BLOCKSIZE/2-1, std::complex<float>(0.0f, 0.0f));
        // std::cout << cx_out.tail(2).t() << std::endl;
        
        fft.ifft_cx(cx_out.memptr(),out_hilbert.memptr());
        //FM解调
        unwrap_hilbert(output,out_hilbert.memptr(),infreq_*1e6,BLOCKSIZE);
        //去加重
        fft.fft(output,cx_out.memptr());
        for ( ii = 0; ii < BLOCKSIZE; ++ii)
        {
            cx_out[ii]*=(deemp[ii]*abs_h_f_l[ii]);
        }

        fft.ifft(cx_out.memptr(),output);
    }
    FMdecode::RFDecode::~RFDecode()
    {
    }
}
