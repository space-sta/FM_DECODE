#include "resync.hpp"


namespace Sync {

Resync::Resync(float *input, int len, float infreq, int pointlen)
: input_(input), len_(len), infreq_(infreq), pointlen_(pointlen)
{
}

Resync::~Resync()
{
}

int Resync::computeTimings()
{
    if (infreq_ < 10.0) {
        return -1;
    }
    //0 hsyn_len 1 eq_len 2 vsyn_len

    ranges_[0][0] = usectoinpx(Params_PAL.hsyncPulseUS - 1.75, infreq_);
    ranges_[0][1] = usectoinpx(Params_PAL.hsyncPulseUS + 2.0, infreq_);

    ranges_[1][0] = usectoinpx(Params_PAL.eqPulseUS - 0.5, infreq_);
    ranges_[1][1] = usectoinpx(Params_PAL.eqPulseUS + 0.5, infreq_);

    ranges_[2][0] = usectoinpx(Params_PAL.vsyncPulseUS - 1.0, infreq_);
    ranges_[2][1] = usectoinpx(Params_PAL.vsyncPulseUS + 1.0, infreq_);

    return 0;
}

int Resync::updateOneTiming(int index, int min, int max)
{
    if (index < 0 || index >= 3) {
        return -1;
    }
    ranges_[index][0] = min;
    ranges_[index][1] = max;
    return 0;
}

int Resync::updateAllTiming(int hsyn_len, int eq_len, int vsyn_len)
{
    // updateOneTiming(0, hsyn_len - 5, hsyn_len + 5);
    // updateOneTiming(1, eq_len - 5, eq_len + 5);
    // updateOneTiming(2, vsyn_len - 5, vsyn_len + 5);
    ranges_[0][0] = hsyn_len + usectoinpx(- 1.75, infreq_);
    ranges_[0][1] = hsyn_len + usectoinpx( + 2.0, infreq_);

    ranges_[1][0] = eq_len + usectoinpx( - 0.5, infreq_);
    ranges_[1][1] = eq_len + usectoinpx( + 0.5, infreq_);

    ranges_[2][0] = vsyn_len + usectoinpx( - 1.0, infreq_);
    ranges_[2][1] = vsyn_len + usectoinpx( + 1.0, infreq_);
    return 0;
}

/// @brief 
/// @param detailPos 精确位置的输出参数
/// @param mid_threshold ireNeg20_threshold_
/// @param UpDown up 上升沿 +1          down 下降沿 -1
/// @param start 起点
/// @return 
int Resync::getDetailPos(float& detailPos, float mid_threshold, char UpDown, int start) const
{
    int step_used = step_;  // Initial step size for binary search
    if (step_used < 1) {
        return -1;
    }

    // while (step_used > 1) {
    //     step_used = step_used >> 1;
    //     if (input_[start] < mid_threshold) {
    //         start += (UpDown * step_used);
    //     } else {
    //         start -= (UpDown * step_used);
    //     }
    // }

    // if (input_[start] < mid_threshold) {
    //     step_used = UpDown * 1;
    // } else {
    //     step_used = -UpDown * 1;
    // }
    //detailPos = start + inc * (input_[start] - mid_threshold) / (input_[start] - input_[start + inc]);

    bool is_below = input_[start] < mid_threshold;
    int inc = is_below ? UpDown : -UpDown;
    start += inc;
    while (step_used > 0) {
        if ((input_[start] < mid_threshold) == is_below) {
            start += inc;
        } else {
            break;
            // start -= inc;
        }
        step_used--;
    }
    detailPos = start - inc * (input_[start] - mid_threshold) / (input_[start] - input_[start - inc]);
    // std::cout << "detailPos:" << detailPos << "|start:" << start << "|start-inc:" << start - inc << std::endl;
    return 0;
}

int Resync::findEqAndVsyn()
{
    if (ireNeg20_threshold_ <= 0 || ire10_threshold_ <= 0) {
        return -1;
    }


    // const size_t N = len_;
    int step = step_;
    int i = 0;
    int cur_start = 0;
    int last_start = 0;
    int len = 0;
    int count1 = 0;
    int count2 = 0;
    bool in_pulse = input_[0] < ireNeg20_threshold_;
    bool enable_count2 = false;

    for (i = 1; i < len_; i += step) {
        if (in_pulse) {
            if (input_[i] > ireNeg20_threshold_) {
                if (cur_start == last_start) {
                    enable_count2 = false;
                    in_pulse = false;
                    step = 15;
                    continue;
                }
                len = static_cast<int>(i) - cur_start;
                //异常负脉冲 处理
                if (len < ranges_[1][0]) {
                    cur_start = last_start;
                    in_pulse = false;
                    enable_count2 = false;
                    continue;
                }
                int tempi = static_cast<int>(cur_start + (Params_PAL.line_period - 2) * infreq_ / 4);
                i = i >= tempi ? i : tempi;
                step = 15;
                in_pulse = false;
            }
        } else if (input_[i] <= ireNeg20_threshold_) {
            step = step_;
            count1 = 0;
            last_start = cur_start;
            cur_start = static_cast<int>(i);
            in_pulse = true;
            enable_count2 = true;
        } else if (input_[i] > ire10_threshold_) {
            if (cur_start == last_start) {
                continue;
            }
            ++count1;
            if (enable_count2) {
                --count2;
            }
            if (count1 > 10) {
                i = static_cast<int>(cur_start + (Params_PAL.line_period - 2) * infreq_);
                last_start = cur_start;
                count1 = 0;
                step = step_;
            }
        } else if (enable_count2) {
            ++count2;
            if (-5 <= count2 && count2 <= 10) {
                continue;
            }
            if (count2 > 10) {
                if (inRange(len, ranges_[1][0], ranges_[1][1])) {
                    return cur_start;
                }
                if (inRange(len, ranges_[2][0], ranges_[2][1])) {
                    return cur_start;
                }
                if (inRange(len, ranges_[0][0], ranges_[1][0])) {
                    i = static_cast<int>(cur_start + (Params_PAL.line_period - 2) * infreq_);
                    last_start = cur_start;
                }
            }
            int tempi = static_cast<int>(cur_start + (Params_PAL.line_period - 2) * infreq_ / 2);
            i = i >= tempi ? i : tempi;
            count2 = 0;
            enable_count2 = false;
        }
    }
    return -1;
}


int Resync::getTestPoints(Points points, int start, bool detail) const
{
    if (ireNeg20_threshold_ <= 0 || ire10_threshold_ <= 0) {
        return -1;
    }

    // const size_t N = len_;
    int i=0,j = 0;
    int count = 0;
    int cur_start = start;
    int last_start = 0;
    bool in_pulse = input_[start] < ireNeg20_threshold_;

    for (i = start + 1; i < len_; i += step_) {
        if (in_pulse) {
            if (input_[i] > ireNeg20_threshold_) {
                if (cur_start == last_start) {
                    in_pulse = false;
                    continue;
                }

                if (detail) {
                    float temp_pos = cur_start;
                    getDetailPos(temp_pos, ireNeg20_threshold_, -1, cur_start);
                    points[j].IndexF = temp_pos;
                    temp_pos = i;
                    getDetailPos(temp_pos, ireNeg20_threshold_, 1, i);
                    points[j].length = temp_pos - points[j].IndexF;
                } else {
                    points[j].IndexF = cur_start;
                    points[j].length = static_cast<int>(i) - cur_start - 3;
                }
                //异常负脉冲 处理
                if (points[j].length < ranges_[1][0]) {
                    cur_start = last_start;
                    in_pulse = false;
                    points[j].IndexF=0;
                    continue;
                }
                ++j;
                if (j >= pointlen_) {
                    break;
                }
                int tempi = static_cast<int>(cur_start + (Params_PAL.line_period - 2) * infreq_ / 2);
                i = i >= tempi ? i : tempi;
                in_pulse = false;
            }
        } else if (input_[i] <= ireNeg20_threshold_) {
            last_start = cur_start;
            cur_start = static_cast<int>(i);
            in_pulse = true;
            count = 0;
        } else if (input_[i] > ire10_threshold_) {
            if (cur_start == last_start) {
                continue;
            }
            ++count;
            if (count > 10) {
                i = static_cast<int>(cur_start + (Params_PAL.line_period - 2) * infreq_);
                last_start = cur_start;
                count = 0;
            }
        }
    }
    if (j >= pointlen_) return -2;
    if (in_pulse) {
        points[j].IndexF = cur_start;
        if (detail) {
            float temp_pos = cur_start;
            getDetailPos(temp_pos, ireNeg20_threshold_, -1, cur_start);
            points[j].IndexF = temp_pos;
        } else {
            points[j].IndexF = cur_start;
        }
        points[j].type = unknown;
    }
    else
    {
        if (input_[len_ - 1] < ireNeg20_threshold_) {
            if (detail) {
                float temp_pos = len_ - 1;
                getDetailPos(temp_pos, ireNeg20_threshold_, -1, len_ - 1);
                points[j].IndexF = temp_pos;
            } else {
                points[j].IndexF = len_ - 1;
            }
            points[j].type = unknown;
        }
    }
    return 0;
}

int Resync::calLength(Points points)
{
    int i;
    for ( i = pointlen_ - 1; i >= 0; i--)
    {
        if (points[i].IndexF)
        {
            break;
        }
    }
    pointsNum_ = i+1;
    return 0;
}

int Resync::initTypes(Points points, int &EqAndVsynNum)
{
    for (int i = 0; i < pointsNum_; ++i)
    {
        //0 hsyn_len 1 eq_len 2 vsyn_len
        int len = points[i].length;
        if (inRange(len, ranges_[0][0], ranges_[0][1]))
        {
            points[i].type=Hsyn;
        }
        else if (inRange(len, ranges_[1][0], ranges_[1][1]))
        {
            //暂时都初始化为前均衡脉冲
            points[i].type=Equaforth;
            EqAndVsynNum++;
        }
        else if (inRange(len, ranges_[2][0], ranges_[2][1]))
        {
            points[i].type=Vsyn;
            EqAndVsynNum++;
        }
        else if (points[i].type==unknown)
        {
            //跳过
        }
        else if ( i > 0)
        {
            //对于长度异常的点，如果它前后都是正常点，则认为它是一个被干扰的点，直接将它的坐标设置为前一个点的坐标
            points[i].IndexF=points[i-1].IndexF;
        }
    }
    return 0;
}

int Resync::getVblankInfos(Points points, int8_t VblankNum[], int8_t VblankStart[])
{
    //Hsyn开始索引
    VblankStart[Hsyn]=0;
    int i;
    int8_t lastType = Hsyn,nextType=Hsyn;

    for (i = 0; i < pointsNum_; ++i)
    {
        if (points[i].type > Hsyn) break;
        VblankNum[Hsyn]++;
    }

    if (i >= pointsNum_) return 0;
    synType curTpye=points[i].type;
    nextType = curTpye;

    for (; i < pointsNum_; ++i)
    {
        curTpye = points[i].type;
        if (curTpye < Hsyn || curTpye > Equaback) continue;
        if (nextType != curTpye )
        {
            switch (curTpye)
            {
            case Hsyn:
                if (VblankNum[Hsyn]==0 && VblankNum[Equaback]==0)
                {
                    VblankNum[Equaback]=VblankNum[Equaforth];
                    VblankNum[Equaforth]=0;
                    VblankStart[Equaback] = VblankStart[Equaforth];
                    VblankStart[Equaforth]=-1;
                    i = pointsNum_;//结束循环
                }
                break;
            case Equaforth:
                // std::cout << "Equa" << i << std::endl;
                if (nextType==Equaback)
                {
                    VblankNum[Equaback]?:VblankStart[Equaback]=i;
                    VblankNum[Equaback]++;
                }
                else{
                    VblankNum[Equaforth]++;
                }
                break;
            case Vsyn:
                VblankNum[Vsyn]++;
                // std::cout << "Vsyn" << i << std::endl;
                break;
            default:
                break;
            }
        }
        else
        {
            if (curTpye > Hsyn)
            {
                // lastType = curTpye;
                VblankStart[curTpye]=i;
                VblankNum[curTpye]++;
                nextType = curTpye + 1;
            }
        }
        
    }
    return 0;
}

int Resync::getLevels(Points vsyn_points_f, fvec& levels)
{
    size_t i = 0;
    size_t j = 0;
    size_t k = 0;
    const float* dataptr = input_;
    const float* begin_loc = nullptr;
    fvec hsyn((Params_PAL.line_period + 8) * infreq_ / 2, arma_nozeros_indicator{});
    uint16_t NoEmptylen = pointsNum_;

    // for (i = pointlen_ - 1; i >= 0; --i)
    // {
    //     if (vsyn_points_f[i].IndexF)
    //     {
    //         break;
    //     }
    // }
    // NoEmptylen=i+1;

    fvec Hsyn_median(NoEmptylen);
    fvec ire0_median(NoEmptylen);

    for (i = 0; i < NoEmptylen; ++i) {
        if (i < pointlen_ - 1 && vsyn_points_f[i].type == Equaforth && vsyn_points_f[i + 1].IndexF) {
            begin_loc = dataptr + static_cast<int>(vsyn_points_f[i].IndexF) + vsyn_points_f[i].length;
            int len = static_cast<int>(vsyn_points_f[i + 1].IndexF - vsyn_points_f[i].IndexF) - vsyn_points_f[i].length;
            std::memcpy(reinterpret_cast<void*>(hsyn.memptr()), reinterpret_cast<const void*>(begin_loc), len * sizeof(input_[0]));
            std::nth_element(hsyn.begin(), hsyn.begin() + (len >> 1), hsyn.begin() + len);
            ire0_median[j] = *(hsyn.begin() + (len >> 1));
            ++j;
            continue;
        }
        if (vsyn_points_f[i].type == Equaforth) {
            continue;
        }
        begin_loc = dataptr + static_cast<int>(vsyn_points_f[i].IndexF);
        int len = vsyn_points_f[i].length;
        std::memcpy(reinterpret_cast<void*>(hsyn.memptr()), reinterpret_cast<const void*>(begin_loc), len * sizeof(input_[0]));
        //求中位数
        std::nth_element(hsyn.begin(), hsyn.begin() + (len >> 1), hsyn.begin() + len);
        Hsyn_median[k] = *(hsyn.begin() + (len >> 1));
        ++k;
    }
    //求中位数的中位数
    if (k > 0) {
        std::nth_element(Hsyn_median.begin(), Hsyn_median.begin() + (k >> 1), Hsyn_median.begin() + k);
        levels[0] = Hsyn_median[k >> 1];
    } else {
        levels[0] = -1;
    }

    if (j > 0) {
        std::nth_element(ire0_median.begin(), ire0_median.begin() + (j >> 1), ire0_median.begin() + j);
        levels[1] = ire0_median[j >> 1];
    } else {
        levels[1] = -1;
    }

    return 0;
}

}