#ifndef UTILL_H
#define UTILL_H

#include <armadillo>
// #include <complex>
#include "SysParams.hpp"
using namespace arma;

#define INRANGE(x,min,max) ((x)>max?false:((x)<min?false:true))
typedef struct
{
    float IndexF;
    int length;
    synType type;;
    
} PointInfo;

using Points = PointInfo[];

int unwrap_hilbert(float* dangles,const std::complex<float>* hilbert,double freq_hz,const uword N);

#endif