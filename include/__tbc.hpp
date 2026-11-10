#ifndef TBC_H
#define TBC_H

#include <armadillo>
#include "sigpack.h"   // Sigpack 主头文件
#include "SysParams.hpp"
#include <algorithm>

using namespace arma;

#define INRANGE(x,min,max) ((x)>max?false:((x)<min?false:true))

// template<typename T>
int get_timings(s32_mat& ranges,const double inputfreq);
int updata_timings(s32_mat& ranges,const int index,int min,int max);
int get_levels(vec& demode_05,double inputfreq,s32_mat& test_points,vec& levels);
int find_eqAndvsyn(const vec& demode_05,const double inputfreq,const double hsyn_threshold,const double hsyn_level);
int get_detailPos(const vec& demode_05,double& detailPos,const double hsyn_threshold,const char UpDown,int start,int step_used);

template<typename T>
int get_testpoints(const vec& demode_05,double inputfreq,const int start,const double hsyn_threshold,const double ire10_threshold,T& points,bool detail=false)
{
    const uword N = demode_05.n_elem;
    uword i=0,j=0,step=10,count=0;
    // using ElementType = typename T::elem_type; // 获取元素类型
    s32 cur_start = start,last_start=0;
    bool in_pulse = demode_05[start]<hsyn_threshold;


    for ( i = start+1; i < N; i+=step)
    {
        // if (demode_05[i] < hsyn_threshold)
        // {
        //     if (in_pulse)
        //     {
        //         continue;
        //     }
        //     cur_start=i;
        //     in_pulse = true;
        //     step=3;
        // }
        // else if (!cur_start) 
        // {
        //     if (in_pulse) in_pulse=false;
            
        //     // step=10;
        //     // continue;
        // }
        // else if (demode_05[i]<ire10_threshold)
        // {
        //     if (in_pulse)
        //     {
        //         starts[j]=cur_start;
        //         lens[j] = i-cur_start;
        //         ++j;
        //         if (j>=5)
        //         {
        //             break;
        //         }
                
        //         i=i+(Params_PAL.line_period-Params_PAL.hsyncPulseUS-1)*inputfreq/2;
        //         in_pulse=false;
        //     }
        // }
        // else
        // {
        //     count++;
        //     step=5;
        //     if (count>5)
        //     {
        //         i=cur_start+(Params_PAL.line_period-1)*inputfreq;
        //         count=0;
        //         step=3;
        //     }
            
        // }
        
        if (in_pulse)
        {
            if (demode_05[i]>hsyn_threshold)
            {
                if (cur_start==last_start)
                {
                    in_pulse=false;
                    // step=10;
                    continue;
                }
                if (detail)
                {

                    double temp_pos = cur_start;
                    get_detailPos(demode_05,temp_pos,hsyn_threshold,-1,cur_start,step);
                    points(0,j)=temp_pos;
                    // std::cout <<" s2:"<< temp_pos ;
                    temp_pos=i;
                    // std::cout <<" e1:"<< temp_pos ;
                    get_detailPos(demode_05,temp_pos,hsyn_threshold,1,i,step);
                    // std::cout <<" e2:"<< temp_pos ;
                    points(1,j) = temp_pos-points(0,j);
                }
                else
                {                    
                    points(0,j)=cur_start;
                    points(1,j) = i-cur_start-3;

                }
                ++j;
                if (j>=points.n_cols)
                {
                    break;
                }
                
                uword tempi =cur_start+(Params_PAL.line_period-2)*inputfreq/2;
                i = i>=tempi?i:tempi;
                // step=10;
                // std::cout << i << "|"<<tempi << "h   ";
                in_pulse=false;
            }
        }
        else if (demode_05[i] <= hsyn_threshold)
        {
            last_start=cur_start;
            cur_start=i;
            in_pulse = true;
            // step=5;
            count=0;
        }
        else if (demode_05[i]>ire10_threshold)
        {
            if (cur_start==last_start) continue;
            count++;
            if (count>10)
            {
                i=cur_start+(Params_PAL.line_period-2)*inputfreq;
                last_start=cur_start;
                // std::cout << i <<"    ";
                count=0;
                // step=5;
            }
            
        }
    }
    // if (in_)
    return 0;
}


#endif