#include "tbc.hpp"


int get_timings(s32_mat& ranges,const double inputfreq)
{
    // s32_mat ranges(2,3);
    //0 hsyn_len 1 eq_len 2 vsyn_len
    if (inputfreq<10)
    {
        return -1;
    }
    
    ranges(0,0) = usectoinpx(Params_PAL.hsyncPulseUS-1.75,inputfreq);
    ranges(1,0) = usectoinpx(Params_PAL.hsyncPulseUS+2,inputfreq);

    ranges(0,1) = usectoinpx(Params_PAL.eqPulseUS-0.5,inputfreq);
    ranges(1,1) = usectoinpx(Params_PAL.eqPulseUS+0.5,inputfreq);

    ranges(0,2) = usectoinpx(Params_PAL.vsyncPulseUS-1,inputfreq);
    ranges(1,2) = usectoinpx(Params_PAL.vsyncPulseUS+1,inputfreq);
    return 0;
}

int updata_timings(s32_mat& ranges,const int index,int min,int max)
{
    ranges(0,index) = min;
    ranges(1,index) = max;
    return 0;
}

// up 上升沿 +1          down 下降沿 -1
int get_detailPos(const vec& demode_05,double& detailPos,const double hsyn_threshold,const char UpDown,int start,int step_used)
{
    if (step_used<1)
    {
        return -1;
    }
    while (step_used>1)
    {
        step_used = step_used>>1;
        if (demode_05[start]<hsyn_threshold)
        {
            start += (UpDown*step_used);
        }
        else
        {
            start-= (UpDown*step_used);
        }
    }

    if (demode_05[start]<hsyn_threshold)
    {
        step_used = UpDown*1;
        // step_used = UpDown*step_used;
    }
    else
    {
        step_used= -UpDown*1;
        // step_used= -UpDown*step_used;
    }

    detailPos = start+step_used*(demode_05[start]-hsyn_threshold)/(demode_05[start]-demode_05[start+step_used]);

    return 0;
    
}

int find_eqAndvsyn(const vec& demode_05,const double inputfreq,const double hsyn_threshold,const double hsyn_level)
{
    uword N = demode_05.n_elem;
    uword i=0,step=10,cur_start = 0,last_start=0,len=0;
    s32 count1=0,count2=0;
    bool in_pulse = demode_05[0]<hsyn_threshold,enable_count2=false;
    double ire10Hz = hsyn_level + (10-Params_PAL.vsync_ire)*Params_PAL.hz_ire;
    // std::cout << hsyn_threshold <<"|"<<ire10Hz<<endl;
    s32_mat ranges(2,3);
    get_timings(ranges,inputfreq);
    
    for (i = 1; i < N; i+=step)
    {
        if (in_pulse)
        {
            if (demode_05[i]>hsyn_threshold)
            {
                if (cur_start==last_start)
                {
                    enable_count2=false;
                    in_pulse=false;
                    step=15;
                    continue;
                }
                len = i - cur_start;
                //异常负脉冲 处理
                if (len < ranges(0,1))
                {
                    cur_start = last_start;
                    in_pulse=false;
                    enable_count2=false;
                    continue;
                }
                
                s32 tempi =cur_start+(Params_PAL.line_period-2)*inputfreq/4;
                i = i>=tempi?i:tempi;
                step=15;
                // std::cout << i << "q   ";
                in_pulse=false;
            }
            
        }
        else if (demode_05[i]<=hsyn_threshold)
        {
            step=10;
            count1=0;
            last_start = cur_start;
            cur_start=i;
            in_pulse=true;
            enable_count2=true;
        }
        else if (demode_05[i]>ire10Hz)
        {
            if (cur_start==last_start) continue;
            count1++;
            if (enable_count2) count2--;
            if (count1>10)
            {
                i=cur_start+(Params_PAL.line_period-2)*inputfreq;
                last_start=cur_start;
                // std::cout << i <<"A   ";
                count1=0;
                step=10;
            }
        }
        else if (enable_count2)
        {
            count2++;
            if (-5 <= count2 && 10 >= count2)
            {
                continue;
            }
            // std::cout << count2 << "c2  ";
            if (10 < count2)
            {
                if (INRANGE(len, ranges(0,1),ranges(1,1)))
                {
                    return cur_start;
                }
                else if (INRANGE(len, ranges(0,2),ranges(1,2)))
                {
                    return cur_start;
                }
                else if (INRANGE(len, ranges(0,0),ranges(1,0)))
                {
                    i=cur_start+(Params_PAL.line_period-2)*inputfreq;
                    last_start=cur_start;
                }
            }
            
            s32 tempi =cur_start+(Params_PAL.line_period-2)*inputfreq/2;
            i = i>=tempi?i:tempi;
            // std::cout << i << "h   ";
            count2=0;
            enable_count2=false;
            
        }
    }
    return -1;
}

int get_levels(vec& demode_05,double inputfreq,s32_mat& test_points,vec& levels)
{
    // s32_mat test_points(3,5);
    // get_testpoints(demode_05,40,0,iretohz(-20),iretohz(10),test_points);
    // cout << endl << test_points.t() <<endl;
    // cout << iretohz(-20)<<"|"<<iretohz(Params_PAL.vsync_ire)<<endl;

    // op_median::direct_median
    uword i=0,j=0,k=0;
    double *dataptr=demode_05.begin(),*begin_loc=nullptr;
    vec hsyn((Params_PAL.line_period+8)*inputfreq/2,arma_nozeros_indicator{});
    uint16_t NoEmptylen=0;
    for (i = 0; i < test_points.n_cols && test_points(0,i); ++i)
    {
        NoEmptylen++;
    }
    vec Hsyn_median(NoEmptylen),ire0_median(NoEmptylen);

    for (i = 0; i < NoEmptylen; ++i)
    {
        if (i<test_points.n_cols-1)
        {
            if (test_points(2,i)==Equa && test_points(0,i+1))
            {
                begin_loc = dataptr+test_points(0,i)+test_points(1,i);
                uint32_t len = test_points(0,i+1)-test_points(0,i)-test_points(1,i);
                memcpy(reinterpret_cast<void*>(hsyn.memptr()),reinterpret_cast<void*>(begin_loc),len*sizeof(demode_05[0]));
                std::nth_element(hsyn.begin(),hsyn.begin()+(len>>1),hsyn.begin()+len);
                ire0_median[j]=*(hsyn.begin()+(len>>1));
                j++;
                continue;
            }
        }
        if (test_points(2,i)==Equa) continue;
        
        // else if (DATALEN > test_points(0,i)+Params_PAL.line_period*inputfreq/2)
        // {

        // }
        
        begin_loc = dataptr+test_points(0,i);
        uint32_t len = test_points(1,i);
        memcpy(reinterpret_cast<void*>(hsyn.memptr()),reinterpret_cast<void*>(begin_loc),len*sizeof(demode_05[0]));
        // begin_loc=hsyn.begin();
        //求中位数
        std::nth_element(hsyn.begin(),hsyn.begin()+(len>>1),hsyn.begin()+len);
        Hsyn_median[k]=*(hsyn.begin()+(len>>1));
        k++;
        // std::cout << *(hsyn.begin()+(len>>1)) << endl;
    }
    // std::cout<<hsyn.tail(6).t()<<endl;
    //求中位数的中位数
    if (k>0)
    {
        std::nth_element(Hsyn_median.begin(),Hsyn_median.begin()+(k>>1),Hsyn_median.begin()+k);
        // std::cout<<hsyn.tail(6).t()<<endl;
        levels[0] = Hsyn_median[k>>1];
    }
    else
    {
        levels[0]=-1;
    }

    if (j>0)
    {
        std::nth_element(ire0_median.begin(),ire0_median.begin()+(j>>1),ire0_median.begin()+j);
        levels[1] = ire0_median[j>>1];
    }
    else
    {
        levels[1]=-1;
    }
    return 0;
}