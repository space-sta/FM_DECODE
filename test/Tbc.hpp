#ifndef TBC_HPP
#define TBC_HPP

#include <armadillo>
#include "sigpack.h"
#include "SysParams.hpp"
#include "utill.hpp"
#include <algorithm>
#include <cstdint>

namespace decode {

class Tbc {
public:
    explicit Tbc(float* data,float* outdata,uint8_t* grayData,float inputfreq,int len);

    int setGray(float freq,int j);
    void setIRE0Threshold(float threshold);
    int processOddEven(Points points,int8_t VblankNum[], int8_t VblankStart[],float lastPoint,int pointsNum);
    int calculateOutputCrack(float startIndex, float lastPoint);
    int calculateOutput(Points vsyn_points_f,int start,int end);

    void resetIndex1();
    void resetIndex2();
    int8_t& getFieldIndex() { return FieldIndex; }

    // Public readable member variables

private:

    float inputFreq_ = 0.0;
    const float* data_ = nullptr;
    float* outdata_ = nullptr;
    uint8_t* grayData_ = nullptr;
    int len_ = 0;
    float ire0_threshold_ = -1.0;
    int8_t FieldIndex = 0; //场索引，0表示第一场，1表示第二场
    int LineIndex[2] = {0, 313}; //第一场和第二场的行索引，初始值分别为0和313
};

inline void Tbc::resetIndex1()
{
    LineIndex[0] = 0;
}
inline void Tbc::resetIndex2()
{
    LineIndex[1] = 313;
}
inline void Tbc::setIRE0Threshold(float threshold)
{
    ire0_threshold_ = threshold;

} // namespace decode


}

#endif // TBC_HPP