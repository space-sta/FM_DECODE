#include <iostream>
#include <fstream>
#include <array>
#include <algorithm>
#include <cctype>
#include <cstring>
#include "FlacSampleReader.hpp"

#include <armadillo>
#include <QApplication>
#include "sigpack.h"   // Sigpack 主头文件
// #include "fftw/fftw.h"
// #include "MatSave.hpp"
// #include "utill.hpp"
#include "mainwindow.hpp"
#include "SysParams.hpp"
// #include "tbc.hpp"
#include "Tbc.hpp"
// #include "fftf.hpp"
#include "GrayImage.hpp"
#include "RFDecode.hpp"
#include "fileAdress.hpp"
#include "resync.hpp"

// #define BLOCKSIZE 1600000
// #define BLOCKSIZE 4*1833
// #define BLOCKSIZE 32*1024

using arma::fill::none;

// #include <QTimer>
// #include "GrayImage.hpp"
// #include <vector>

// int main(int argc, char *argv[])
// {
//     QApplication app(argc, argv);

//     int width = 512;
//     int height = 512;

//     GrayImageWidget widget;
//     widget.init(width, height);
//     widget.show();

//     // std::vector<unsigned char> grayData(width * height);
//     uchar* grayData = widget.getImagePointer();

//     QTimer timer;
//     QObject::connect(&timer, &QTimer::timeout, [&]() {
//         QMutexLocker locker(&widget.m_mutex);
//         static int t = 0;
//         for(int i = 0; i < width * height; ++i) {
//             grayData[i] = (i + t) % 256; // 模拟动态灰度
//         }
//         t++;
//         widget.updateGrayData();
//     });

//     timer.start(30); // ~33 FPS
//     return app.exec();
// }

int main(int argc, char *argv[]) {

    std::cout << "Starting application..." << std::endl;
    QApplication a(argc, argv);

    // Munday-Walking-Around-Garden_2022-09-08_01-15-22.raw   2023-SMPTE-WSS-EdgeCrop-CXADC-CX-Blue-8bit.raw
    // Optional command-line path; otherwise use fileAdress.cpp configuration.
    const std::string inputPath = argc > 1 ? argv[1] : DataDir + FileName;
    std::string extension = inputPath.substr(inputPath.find_last_of('.') == std::string::npos
                                            ? inputPath.size() : inputPath.find_last_of('.'));
    std::transform(extension.begin(), extension.end(), extension.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    const bool isFlac = extension == ".flac";
    FlacSampleReader flac;
    std::ifstream file;
    if (isFlac) {
        if (!flac.open(inputPath.c_str())) {
            std::cerr << "FLAC open failed: " << flac.error() << std::endl;
            return 1;
        }
        std::cout << "FLAC: " << flac.bitsPerSample() << " bits, container rate "
                  << flac.containerSampleRate()
                  << " Hz; actual RF rate remains 40 MHz" << std::endl;
    } else {
        file.open(inputPath, std::ios::binary);
        if (!file) {
            std::cerr << "Cannot open input: " << inputPath << std::endl;
            return 1;
        }
    }
    std::array<int16_t, BLOCKSIZE> rawBuffer;
    std::array<float, BLOCKSIZE> rfBlock;
    auto readSamples = [&](float* dst, size_t count) -> size_t {
        if (isFlac) return flac.readSamples(dst, count);
        file.read(reinterpret_cast<char*>(rawBuffer.data()), count * sizeof(int16_t));
        const size_t n = static_cast<size_t>(file.gcount()) / sizeof(int16_t);
        for (size_t k = 0; k < n; ++k) dst[k] = static_cast<float>(rawBuffer[k]);
        return n;
    };
    bool firstBlock = true;

    int i=0,m=4,start=-1;
    float inputfreq=40.0f;
    constexpr int pointlen = 32;
    FMdecode::RFDecode rfDecoder(BLOCKSIZE,inputfreq);
    
    MainWindow w;
    w.resize(1800, 600);
    w.show();
    GrayImageWidget widget;
    widget.init(Params_PAL.outlinelen, Params_PAL.frame_lines);
    widget.show();
    std::cout << "GrayImageWidget initialized." << std::endl;
    std::cout << "Output line length: " << widget.width() << std::endl;
    
    // arma::vec data = arma::vec(BLOCKSIZE,nozeros);
    arma::fmat::fixed<BLOCKSIZE, 2> data(none);
    arma::fvec outdata(Params_PAL.outlinelen * pointlen, none);//长度必须足够，太短后面的函数会越界访问outdata的属性
    uchar* grayData = widget.getImagePointer();
    
    Sync::Resync resync(data.begin_col(1)+OFFSET,DATALEN,inputfreq,pointlen);
    decode::Tbc tbc(data.begin_col(1)+OFFSET, outdata.memptr(),grayData,inputfreq, DATALEN);
    // resync.hsyn_threshold_ = static_cast<float>(iretohz(Params_PAL.vsync_ire));
    // resync.ireNeg20_threshold_ = static_cast<float>(iretohz(-20));
    // resync.ire0_threshold_ = static_cast<float>(iretohz(0));
    // resync.ire10_threshold_ = static_cast<float>(iretohz(10));
    resync.hsyn_threshold_ = 3.81368e+06;
    resync.ire0_threshold_ = 4.11006e+06; //上下相差296,380
    resync.ireNeg20_threshold_ = (resync.ire0_threshold_ + resync.hsyn_threshold_) / 2;
    resync.ire10_threshold_ = resync.ire0_threshold_ + 10 * Params_PAL.hz_ire;

    resync.computeTimings();
    // ranges[0][0] = 111; ranges[1][0] = 67;  ranges[2][0] = 1048;
    // ranges[0][1] = 261; ranges[1][1] = 107; ranges[2][1] = 1128;
    resync.updateOneTiming(0,111,261);
    resync.updateOneTiming(1,67,107);
    resync.updateOneTiming(2,1048,1128);

    tbc.setIRE0Threshold(resync.ire0_threshold_);

    // ranges.print("initial ranges:");
    std::cout << "Initial timings (Hsync, Equa, Vsync):" << std::endl;
    
    w.updateSeries2Points(resync.hsyn_threshold_,resync.ire0_threshold_);

    bool firstVsyn = false;
    float lastPoint = DATALEN;
    
    int bytesPerLine = widget.getBytesPerLine();
    int pointsNum = 0,lastPoint_int=DATALEN,EqAndVsynNum=0;

    while (m > 0)
    {
        const size_t retained = firstBlock ? 0 : OFFSET + ENDCUT;
        if (!firstBlock) {
            std::memmove(rfBlock.data(), rfBlock.data() + DATALEN,
                         retained * sizeof(float));
        }
        const size_t wanted = BLOCKSIZE - retained;
        const size_t received = readSamples(rfBlock.data() + retained, wanted);
        if ((isFlac && !flac.error().empty()) || (!isFlac && file.bad())) {
            std::cerr << "Input read failed: "
                      << (isFlac ? flac.error() : "RAW I/O error") << std::endl;
            return 1;
        }
        if (received != wanted) {
            std::cout << "End of input; discarded " << received
                      << " new samples in the incomplete final block." << std::endl;
            break;
        }
        firstBlock = false;
        // Keep original RF samples separate: RFDecode overwrites its input.
        std::copy(rfBlock.begin(), rfBlock.end(), data.begin_col(1));

        rfDecoder.process(data.begin_col(1),data.begin_col(1));

        if (!firstVsyn)
        {
            start = resync.findEqAndVsyn();
            if (start>=0)
            {
                firstVsyn = true;
            }
        }

        // QList<QPointF> points;
        // points.reserve(DATALEN);
        // for (i = 0; i < DATALEN; ++i) {
        //     points.append(QPointF(i, data2(i+OFFSET)));
        // }
        // w.updateSeries1Points(points);
        // cout << file.tellg() << endl;
        // w.waitForContinue();
        // Points float_vsyn_points={0};
        PointInfo float_vsyn_points[pointlen]={0};
        if (start<0)
        {
            //找到Hsyn,更新hsyn的长度
            continue;
        }
        else
        {
            // cout << file.tellg() << endl;
            //找到Equa和Vsyn，更新Equa和Vsyn的长度
            resync.getTestPoints(float_vsyn_points,0,true);
            resync.calLength(float_vsyn_points);
            pointsNum = resync.getPointNum();
            
        }
        if (pointsNum <= 0) {
            lastPoint = DATALEN;
            continue;
        }
        EqAndVsynNum=0;
        resync.initTypes(float_vsyn_points,EqAndVsynNum);
        // float_vsyn_points.print("float_vsyn_points:");
        
        if (EqAndVsynNum>0)
        {
            /* */
            //numPulses的0索引不使用
            int8_t numPulses[5]={0},startPulse[5]={-1,-1,-1,-1,-1};
            resync.getVblankInfos(float_vsyn_points, numPulses, startPulse);
            
            std::cout << startPulse[0] + 0 << "    "<< numPulses[0] + 0 << "|";
            std::cout << startPulse[1] + 0 << "    "<< numPulses[1] + 0 << "|";
            std::cout << startPulse[2] + 0 << "    "<< numPulses[2] + 0 << "|";
            std::cout << startPulse[3] + 0 << "    "<< numPulses[3] + 0 << "|";
            std::cout << startPulse[4] + 0 << "    "<< numPulses[4] + 0 << "|";
            std::cout << std::endl;
            tbc.processOddEven(float_vsyn_points,numPulses,startPulse,lastPoint,pointsNum);
            
            widget.updateGrayData();
            w.waitForContinue();
        }
        else
        {
            tbc.calculateOutputCrack(float_vsyn_points[0].IndexF, lastPoint);
            tbc.calculateOutput(float_vsyn_points,1,pointsNum);
            
        }

        // std::cout << "lastPoint:" << lastPoint << std::endl;
        lastPoint = float_vsyn_points[pointsNum-1].IndexF;
        //多复制一个点，以确保第一个点可以正确抽样
        if (!std::isfinite(lastPoint) || lastPoint < 1 || lastPoint >= DATALEN) {
            lastPoint = DATALEN;
            continue;
        }
        lastPoint_int = static_cast<int>(lastPoint) - 1;
        std::memcpy(data.begin_col(0)+ OFFSET + lastPoint_int,
                    data.begin_col(1)+ OFFSET + lastPoint_int,
                    (DATALEN - lastPoint_int)*sizeof(float));
        /*
        // if (counter++ < 24)
        // {
        //     continue;
        // }
        // float_vsyn_points.submat(0,0,2,5).print("float_vsyn_points.head(5):");
        // float_vsyn_points.submat(0,pointsNum-6,2,pointsNum-1).print("float_vsyn_points.tail_cols(5):");

        float *data_ptr = data.begin_col(1)+ OFFSET;
        QList<QPointF> points;
        points.reserve(DATALEN);
        for (i = 0; i < DATALEN; ++i) {
            points.append(QPointF(i, data_ptr[i]));
        }
        // qDebug() << points.at(0)<<"|"<<points.at(0); 
        QList<QPointF> points1;
        points1.reserve(20); // QList 预分配内存

        for (i = 0; i < pointsNum; ++i) {
            points1.append(QPointF(float_vsyn_points[i].IndexF, resync.ireNeg20_threshold_)); 
        }
        // cout << "points1.length:" << points1.length() << endl;
        w.updateSeries1Points(points);
        
        w.addScatterPoints(points1);
        w.waitForContinue();

        data_ptr = data.begin_col(0)+ OFFSET;
        QList<QPointF> points2;
        points2.reserve(DATALEN);
        for (i = 0; i < Params_PAL.outlinelen*pointsNum; ++i) 
        {
            points2.append(QPointF(i, outdata[i]));
        }
        for (; i < DATALEN; ++i) {
            points2.append(QPointF(i, data_ptr[i]));
        }
        w.updateSeries1Points(points2);

        w.waitForContinue();
        */
    }

    // 设计一个低通 FIR 滤波器（使用 Sigpack）
    // Create a FIR filter
	// sp::FIR_filt<double,double,double> fir_filt;
    // int order = 50;
    // double Wn = 0.2;  // 归一化截止频率
    // // arma::vec b = sp::fir1(order, Wn, sp::LOWPASS);  // 返回滤波器系数
    // arma::vec b = sp::fir1_hp(order,Wn);    // 返回滤波器系数

    // fir_filt.set_coeffs(b);
    // // 滤波
    // arma::vec y = fir_filt.filter(x);

    // std::cout << "滤波完成，输出长度 = " << y.n_elem << std::endl;

    return a.exec();
    // return 0;
}
/*
{
    arma::fvec levels(2);
        // get_levels(demode_05,inputfreq,vsyn_points,levels);
        tbc.getLevels(vsyn_points, levels);
        if (levels[0]<=0 || levels[0]>10e6)
        {
            cerr << "Error: Invalid levels detected!" << endl;
        }
        else
        {
            tbc.hsyn_threshold_ = levels[0];
        }
        if (levels[1]<=0 || levels[1]>10e6)
        {
            cerr << "Error: Invalid levels detected!" << endl;
        }
        else
        {
            tbc.ireNeg20_threshold_ = (levels[1] + levels[0])/2;
            tbc.ire0_threshold_ = levels[1];
            tbc.ire10_threshold_ = levels[1] + 10 * Params_PAL.hz_ire;

        }
        cout << levels[0]<<"|" << levels[1]<<endl;

        arma::fmat float_vsyn_points(3,20);
        // get_testpoints(demode_05,inputfreq,0,(levels[0]+levels[1])/2,levels[1]+10*Params_PAL.hz_ire,float_vsyn_points,true);
        tbc.getTestPoints(0,float_vsyn_points,true);
        float_vsyn_points.head_cols(10).print("float_vsyn_points.head_cols(10):");
        float_vsyn_points.tail_cols(10).print("float_vsyn_points.tail_cols(10):");

        uint16_t NoEmptylen=0;
        for (i = 0; i < float_vsyn_points.n_cols && float_vsyn_points(0,i); ++i)
        {
            NoEmptylen++;
        }

        double len=0;
        arma::fmat new_lens(2,3);
        for (i = 0; i < NoEmptylen; ++i)
        {
            //0 hsyn_len 1 eq_len 2 vsyn_len
            len = float_vsyn_points(1,i);
            if (INRANGE(len, ranges(0,0),ranges(1,0)))
            {
                float_vsyn_points(2,i)=Hsyn;
                new_lens(0,0)+=len;
                new_lens(1,0)++;
            }
            else if (INRANGE(len, ranges(0,1),ranges(1,1)))
            {
                float_vsyn_points(2,i)=Equa;
                new_lens(0,1)+=len;
                new_lens(1,1)++;
            }
            else if (INRANGE(len, ranges(0,2),ranges(1,2)))
            {
                float_vsyn_points(2,i)=Vsyn;
                new_lens(0,2)+=len;
                new_lens(1,2)++;
            }
        }
        for (i = 0; i < new_lens.n_cols; ++i)
        {
            new_lens(0,i)/=new_lens(1,i);
        }
        new_lens.print("new_lens");
        // tbc.updateAllTiming(static_cast<int>(new_lens(0,0)),static_cast<int>(new_lens(0,1)),static_cast<int>(new_lens(0,2)));
        // ranges.print("ranges:");
}
*/