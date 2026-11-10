#ifndef SYSPARAMS_H
#define SYSPARAMS_H

#define BLOCKSIZE (32 * 1024)
#define OFFSET 1024
#define ENDCUT 32
#define DATALEN (BLOCKSIZE-OFFSET-ENDCUT)

enum synType
{
    Hsyn=1,
    Equaforth,
    Vsyn,
    Equaback,
    unknown,
};

//  Calculate the exact line length for a given situation (such as 4FSC)
inline int calclinelen(int line_period,double mult,double mhz)
{
    return int(line_period * mhz * mult + 0.5);
}

//  These are constant, system-level parameters for PAL and NTSC
typedef struct SysParams_NTSC
{
    double fsc_mhz = 315.0 / 88.0;
    //  NTSC LD doesn't have a pilot signal, so just recycle FSC
    double pilot_mhz= 315.0 / 88.0;
    int frame_lines = 525;
    int field_lines[2] = {263, 262};
    double hz_ire = 1e6 / 140.0;
    double ire0 = 4.4e6 - hz_ire * 100;
    int vsync_ire = -40;
    // most NTSC disks have analog audio, except CD-V and a few Panasonic demos
    // "analog_audio": True,
    //  From the spec - audio frequencies are multiples of the (color) line rate
    // "audio_lfreq": (1000000 * 315 / 88 / 227.5) * 146.25,
    // "audio_rfreq": (1000000 * 315 / 88 / 227.5) * 178.75,
    //  On AC3 disks, the right channel is replaced by a QPSK 2.88mhz channel
    // "audio_rfreq_AC3": 2880000,
    double colorBurstUS[2] = {5.3, 7.8};
    // Known-good area for computing black SNR - for NTSC pull from VSYNC
    //  tuple: (line, beginning, length)
    // "blacksnr_slice": (1, 10, 20),
    // In NTSC framing, the distances between the first/last eq pulses and the
    // corresponding next lines are different.
    double firstFieldH[2] = {0.5, 1};
    int numPulses = 6;  // number of equalization pulses per section
    double hsyncPulseUS = 4.7;
    double eqPulseUS = 2.3;
    double vsyncPulseUS = 27.1;
    //  What 0 IRE/0V should be in 16-bit digital output
    // "outputZero": 1024,
    // "fieldPhases": 4,
    //  Likely locations of solid white in VITS on LD's (line, start, length)
    //  The first three are typical VITS locations (first most common), and last
    //  is the MCA Code first-field flag.
    // "LD_VITS_whitelocs": [(20, 14, 12), (20, 52, 8), (13, 13, 15), (11, 12, 45)],
    //  Similar but with percentile to use to compute white level
    //  (in case VITS white test areas are not present)
    // "LD_VITS_code_slices": [(16, 12, 48, 85), (17, 12, 48, 85), (10, 13, 39, 85)],
    //  Compute dictionary entries for things that use FSC, etc.

    //  In color NTSC, the line period was changed from 63.5 to 227.5 color cycles,
    //  which works out to 63.555(with a bar on top) usec
    double line_period  = 1.0 / (fsc_mhz / 227.5);
    double activeVideoUS[2] = {9.45, line_period - 1.0};

    double FPS = 1e6 / (525.0 * line_period);

    int outlinelen  = calclinelen(line_period,4,fsc_mhz);
    double outfreq = 4 * fsc_mhz;
}SysParams_NTSC;



typedef struct SysParams_PAL
{
    int FPS = 25;
    //  from wikipedia: 283.75 × 15625 Hz + 25 Hz = 4.43361875 MHz
    double fsc_mhz = ((1.0 / 64.0) * 283.75) + (25.0 / 1e6);
    double pilot_mhz = 3.75;
    int frame_lines = 625;
    int field_lines[2] = {312, 313};
    int line_period = 64;
    double ire0 = 4.1e6;
    double hz_ire = 700000 / 100.0;
    //  only early PAL disks have analog audio
    // "analog_audio": True,
    //  From the spec - audio frequencies are multiples of the (colour) line rate
    // "audio_lfreq": (1000000 / 64) * 43.75,
    // "audio_rfreq": (1000000 / 64) * 68.25,
    double colorBurstUS[2] = {5.6, 7.85};
    double activeVideoUS[2] = {10.5, 64 - 1.5};
    //  In PAL, the first field's line sync<->first/last EQ pulse are both .5H
    double firstFieldH[2] = {1, 0.5};
    // Known-good area for computing black SNR - for PAL this is blanked in mastering
    // tuple: (line, beginning, length)
    //"blacksnr_slice": (22, 12, 50),
    int numPulses = 5;  // number of equalization pulses per section
    double hsyncPulseUS = 4.7;
    double eqPulseUS = 2.35;
    double vsyncPulseUS = 27.3;
    // What 0 IRE/0V should be in digitaloutput
    // "outputZero": 256,
    // "fieldPhases": 8,
    // Likely locations of solid white in VITS on LD's
    // (an array of line/start/length)
    // "LD_VITS_whitelocs": [(19, 12, 8)],
    // Similar but with percentile to use to compute white level
    // (in case VITS white test areas are not present)
    // "LD_VITS_code_slices": [(16, 11, 49, 85), (17, 11, 49, 85)],

    // int outlinelen  = int(line_period*4*fsc_mhz+0.5);
    int outlinelen  = calclinelen(line_period,4,fsc_mhz);
    int outlinelen_pilot = calclinelen(line_period,4,pilot_mhz);
    double outfreq = 4 * fsc_mhz;

    double vsync_ire = -0.3 * (100.0 / 0.7);
}SysParams_PAL;


extern SysParams_NTSC Params_NTSC;
extern SysParams_PAL Params_PAL;

inline double iretohz(double ire)
{
    return Params_PAL.ire0 + ire*Params_PAL.hz_ire;
}

inline double hztoire(double hz)
{
    return (hz - Params_PAL.ire0)/Params_PAL.hz_ire;
}

inline double usectoinpx(double x,double inputfreq)
{
    return x * inputfreq;
}

inline double inpxtousec(double x,double inputfreq)
{
    return x / inputfreq;
}

#endif