#include "Tbc.hpp"
#include "utill.hpp"
#include <cstring>

namespace decode {

Tbc::Tbc(float* data, float* outdata,uint8_t* grayData,float inputfreq, int len)
    :inputFreq_(inputfreq), data_(data), outdata_(outdata),grayData_(grayData), len_(len)
{
}


int Tbc::setGray(float freq,int j)
{
    //FieldIndex 0表示第一场 1表示的二场
    if (FieldIndex < 0 || FieldIndex > 1) return -1;
    const int greyLineIndex = LineIndex[FieldIndex];
    const int fieldEnd = FieldIndex == 0 ? 313 : Params_PAL.frame_lines;
    if (!grayData_ || j < 0 || j >= Params_PAL.outlinelen ||
        greyLineIndex < (FieldIndex == 0 ? 0 : 313) || greyLineIndex >= fieldEnd)
        return -1;
    if ( freq <= ire0_threshold_)
    {
        grayData_[greyLineIndex*1136 + j] = 0;
    }
    else if ( freq >= ire0_threshold_ + 100 * Params_PAL.hz_ire)
    {
        grayData_[greyLineIndex*1136 + j] = 255;
    }
    else
    {
        grayData_[greyLineIndex*1136 + j] = static_cast<uint8_t>((freq - ire0_threshold_) / (100 * Params_PAL.hz_ire) * 255);
    }
    return 0;
}



/// @brief 
/// @param startIndex 必须为每次的第一个脉冲 
/// @param lastPoint 上次的最后一个脉冲
/// @return 
int Tbc::calculateOutputCrack(float startIndex, float lastPoint)
{
    if (startIndex < 0 || startIndex >= len_) {
        return -1;
    }
    if (lastPoint < 0 || lastPoint >= len_) {
        //处理完一行给对应的行索引加一,因为上一次的索引异常不处理
        LineIndex[FieldIndex]++;
        return -1;
    }

    float remainlength = DATALEN - lastPoint;
    float linelength = startIndex + remainlength;
    float K = linelength / Params_PAL.outlinelen;
    float level_adjust = linelength / (Params_PAL.line_period*inputFreq_);

    int index_forth = (remainlength - 3)*Params_PAL.outlinelen / linelength;
    // float coord = lastPoint + index * K;
    // int coord_int_forth = static_cast<int>(coord);
    // std::cout << "remainlength:" << remainlength << "|linelength:" << linelength << std::endl;
    // std::cout << "K:" << K << "|coord_int_forth:" << coord_int_forth << "|coord:" << coord << std::endl;
    // std::cout << "index_forth:" << index_forth << std::endl;
    // std::cout << DATALEN - lastPoint - index_forth * K << std::endl;

    int index_back = std::ceil((remainlength+1)*Params_PAL.outlinelen / linelength);
    // std::cout << "index_back:" << index_back << std::endl;
    // std::cout << lastPoint + index_back * K  - DATALEN << std::endl;
    // coord = lastPoint + index * K;
    // int coord_int_back = static_cast<int>(coord);
    float coord = 0;
    int coord_int = 0;
    int i = 0;
    //访问上个block没有抽样的点
    const float *buf = data_ - BLOCKSIZE;
    float p[4];
    float x,a,b,c;

    /**/
    for(i = 0; i <= index_forth; ++i)
    {
        coord = lastPoint + i * K;
        coord_int = static_cast<int>(coord);
        x = coord - coord_int;

        // get the data from the buffer that aligns to the wow factor index
        p[0] = buf[coord_int - 1];
        p[1] = buf[coord_int];
        p[2] = buf[coord_int + 1];
        p[3] = buf[coord_int + 2];
        // perform cubic scaling
        a = p[2] - p[0];
        b = 2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3];
        c = 3.0f * (p[1] - p[2]) + p[3] - p[0];
        outdata_[i] = level_adjust * (p[1] + 0.5f * x * (a + x * (b + x * c)));
        setGray(outdata_[i],i);
    }
    for(i = index_forth+1 ; i < index_back; ++i)
    {
        coord = lastPoint + i * K;
        coord_int = static_cast<int>(coord);
        x = coord - coord_int;

        coord_int--;
        // get the data from the buffer that aligns to the wow factor index
        int j = 0;
        for ( ; coord_int < DATALEN; coord_int++)
        {
            p[j] = buf[coord_int];
            ++j;
        }
        //越界访问现在的block，访问下个block的点
        coord_int += (ENDCUT + OFFSET);
        for ( ; j < 4; ++j)
        {
            p[j] = buf[coord_int];
            coord_int++;
        }

        // perform cubic scaling
        a = p[2] - p[0];
        b = 2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3];
        c = 3.0f * (p[1] - p[2]) + p[3] - p[0];
        outdata_[i] = level_adjust * (p[1] + 0.5f * x * (a + x * (b + x * c)));
        setGray(outdata_[i],i);
    }

    for (i = index_back ; i < Params_PAL.outlinelen; ++i)
    {
        coord = lastPoint + i * K;
        coord_int = static_cast<int>(coord);
        x = coord - coord_int;

        //越界访问现在的block，访问下个block的点
        coord_int += (ENDCUT + OFFSET);
        // get the data from the buffer that aligns to the wow factor index
        p[0] = buf[coord_int - 1];
        p[1] = buf[coord_int];
        p[2] = buf[coord_int + 1];
        p[3] = buf[coord_int + 2];
        // perform cubic scaling
        a = p[2] - p[0];
        b = 2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3];
        c = 3.0f * (p[1] - p[2]) + p[3] - p[0];
        outdata_[i] = level_adjust * (p[1] + 0.5f * x * (a + x * (b + x * c)));
        setGray(outdata_[i],i);
    }
    //处理完一行给对应的行索引加一
    LineIndex[FieldIndex]++;

    return 0;
}
/// @brief 
/// @param vsyn_points_f vsyn_points_f 所有的脉冲信息
/// @param start 开始脉冲索引
/// @param end 
/// @param FieldIndex 0表示第一场 1表示的二场
/// @return 
int Tbc::calculateOutput(Points vsyn_points_f,int start,int end)
{

    //第一部分的计算已经在calculateOutputCrack中完成了，剩下的部分只需要计算第二部分的点
    if (start < 1 || start > end) {
        return -1;
    }
    // float lastPoint = vsyn_points_f(0, vsyn_points_f.n_cols - 1);
    // return calculateOutputCrack(lastPoint, lastPoint);
    int i = 0,j = 0,tempIndex=0;
    float K = 2.2f;
    float linelength = Params_PAL.line_period*inputFreq_;

    float level_adjust = 1.0f;
    float coord = 0;
    int coord_int = 0;
    
    //访问现在的block没有抽样的点
    const float *buf = data_;
    float p[4];
    float x,a,b,c;

    for (i = start; i < end; ++i)
    {
        linelength = vsyn_points_f[i].IndexF - vsyn_points_f[i-1].IndexF;
        //防止距离相近的点干扰计算，距离过近的点不进行计算，直接跳过
        if (linelength <= 10.0f) continue;

        K = linelength / Params_PAL.outlinelen;
        level_adjust = linelength / (Params_PAL.line_period*inputFreq_);
        for (j = 0; j < Params_PAL.outlinelen; ++j)
        {
            coord = vsyn_points_f[i-1].IndexF + j * K;
            coord_int = static_cast<int>(coord);
            x = coord - coord_int;

            // get the data from the buffer that aligns to the wow factor index
            p[0] = buf[coord_int - 1];
            p[1] = buf[coord_int];
            p[2] = buf[coord_int + 1];
            p[3] = buf[coord_int + 2];
            // perform cubic scaling
            a = p[2] - p[0];
            b = 2.0f * p[0] - 5.0f * p[1] + 4.0f * p[2] - p[3];
            c = 3.0f * (p[1] - p[2]) + p[3] - p[0];
            tempIndex = i*Params_PAL.outlinelen + j;
            outdata_[tempIndex] = level_adjust * (p[1] + 0.5f * x * (a + x * (b + x * c)));
            setGray(outdata_[tempIndex],j);
        }
        //处理完一行给对应的行索引加一
        LineIndex[FieldIndex]++;
    }
    return 0;
}

int Tbc::processOddEven(Points points, int8_t VblankNum[], int8_t VblankStart[],float lastPoint,int pointsNum)
{
    if (pointsNum <= 0) return -1;
    for (int type = Hsyn; type <= Equaback; ++type) {
        if (VblankNum[type] < 0 || VblankStart[type] < -1 ||
            VblankStart[type] >= pointsNum ||
            (VblankStart[type] >= 0 && VblankStart[type] + VblankNum[type] > pointsNum))
            return -1;
    }
    int min = Params_PAL.line_period*inputFreq_/3,mid = min*2,max = min*4;
    int i=0,j = 0;
    float lineLen = 0;
    if( VblankStart[Equaforth]>0 )
    {
        lineLen = points[VblankStart[Equaforth]].IndexF 
                - points[VblankStart[Equaforth]-1].IndexF;
        if ( lineLen > mid )
        {
            for (j = Equaback; j > Hsyn; --j)
            {
                if (VblankStart[j]>=0) break;
            }
            for (i = VblankStart[Equaforth]+1; i < VblankStart[j]+VblankNum[j]; i+=2)
            {
                points[i]=points[i-1];
            }
            
            

            calculateOutputCrack(points[0].IndexF,lastPoint);
            calculateOutput(points,1,VblankStart[Equaforth]);
            resetIndex2();
            FieldIndex = 1;//第二场
            // calculateOutputCrack(points[0].IndexF, lastPoint);
            calculateOutput(points,VblankStart[Equaforth],pointsNum);
        }
        else
        {
            for (j = Equaback; j > Hsyn; --j)
            {
                if (VblankStart[j]>=0) break;
            }
            for (i = VblankStart[Equaforth]; i < VblankStart[j]+VblankNum[j]; i+=2)
            {
                points[i]=points[i-1];
            }
            calculateOutputCrack(points[0].IndexF,lastPoint);
            calculateOutput(points,1,VblankStart[Equaforth]-1);
            resetIndex1();
            FieldIndex = 0;//第一场
            // calculateOutputCrack(points[0].IndexF, lastPoint);
            calculateOutput(points,VblankStart[Equaforth]-1,pointsNum);

        }
    }
    else if ( VblankStart[Equaback] > -1 &&
              VblankStart[Equaback] + VblankNum[Equaback] < pointsNum )
    {
        lineLen = points[VblankStart[Equaback]+VblankNum[Equaback]].IndexF 
                - points[VblankStart[Equaback]+VblankNum[Equaback]-1].IndexF;
        if ( lineLen > mid )
        {
            // std::cout << "2" << std::endl;
            for (i = VblankStart[Equaback]+VblankNum[Equaback]-2; i >= 0; i-=2)
            {
                points[i]=points[i+1];
            }
            
            if (VblankNum[Equaforth]>=5)
            {
                calculateOutputCrack(points[0].IndexF, lastPoint);
                resetIndex2();
                FieldIndex = 1;//第二场
            }
            else
            {
                if (FieldIndex != 1) resetIndex2();
                FieldIndex = 1;//第二场
                calculateOutputCrack(points[0].IndexF, lastPoint);
            }
            calculateOutput(points,1,pointsNum);
        }
        else
        {
            // std::cout << "1" << std::endl;
            for (i = VblankStart[Equaback]+VblankNum[Equaback]-1; i >= 0; i-=2)
            {
                points[i]=points[i+1];
            }
            if (FieldIndex != 0) resetIndex1();
            FieldIndex = 0;//第一场
            calculateOutputCrack(points[0].IndexF, lastPoint);
            calculateOutput(points,1,pointsNum);

        }
    }
    return 0;
}

} // namespace decode
