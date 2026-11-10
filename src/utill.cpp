#include "utill.hpp"

int unwrap_hilbert(float* dangles,const std::complex<float>* hilbert,double freq_hz,const uword N)
{
    uword jj;
    dangles[0]=0.0;
    double innerProduct=0.0;
    for(jj=1; jj < N; jj++)
    {
        // F(data[ii]);
        // F(data[jj]);
        // std::arg(data[ii])
        innerProduct = hilbert[jj-1].real()*hilbert[jj].real()+hilbert[jj-1].imag()*hilbert[jj].imag();
        innerProduct /=(std::abs(hilbert[jj-1])*std::abs(hilbert[jj]));
        dangles[jj] = std::acos(innerProduct);
        dangles[jj]*=(freq_hz/arma::datum::tau);
    }
    
    return 0;
}