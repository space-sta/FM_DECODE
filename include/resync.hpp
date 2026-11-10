#ifndef RESYNC_HPP
#define RESYNC_HPP

#include <armadillo>
#include "utill.hpp"

namespace Sync {

class Resync {
public:
    Resync(float* input, int len, float infreq, int pointlen);
    ~Resync();

    // void resync() {
    //     // 实现重新同步的逻辑
    // }

    void setStep(int step) { step_ = step; }
    float inputFreq() const noexcept;
    const int (&timings() const noexcept)[3][2];

    int computeTimings();
    int updateOneTiming(int index, int min, int max);
    int updateAllTiming(int hsyn_len, int eq_len, int vsyn_len);
    int findEqAndVsyn();
    int getLevels(Points vsyn_points_f, arma::fvec& levels);
    int getDetailPos(float& detailPos, float hsyn_threshold, char UpDown, int start) const;
    int getTestPoints(Points points,int start,bool detail = false) const;

    int calLength(Points points);
    int initTypes(Points points,int& EqAndVsynNum);
    int getVblankInfos(Points points, int8_t VblankNum[], int8_t VblankStart[]);

    int getPointNum() const { return pointsNum_; }

    // Public readable member variables
    float hsyn_threshold_ = -1.0;
    float ireNeg20_threshold_ = -1.0;
    float ire0_threshold_ = -1.0;
    float ire10_threshold_ = -1.0;

private:
    static bool inRange(int x, int min, int max) noexcept { return x >= min && x <= max; }
    float* input_ = nullptr;
    int pointlen_;//预设的点的最大数量，必须足够大以存储所有的点，否则会越界访问points数组
    int pointsNum_;//真实的点的数量
    int ranges_[3][2] = {0};
    float infreq_;
    int len_ = 0;
    int step_ = 10;

};

inline const int (&Resync::timings() const noexcept)[3][2]
{
    return ranges_;
}

inline float Resync::inputFreq() const noexcept
{
    return infreq_;
}

} // namespace Sync

#endif // RESYNC_HPP