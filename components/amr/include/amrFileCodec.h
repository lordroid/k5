#ifndef amrFileCodec_h
#define amrFileCodec_h
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include "typedef.h"
#include "cnst.h"
#include "n_proc.h"
#include "mode.h"
#include "frame.h"
#include "strfunc.h"
#include "sp_enc.h"
#include "pre_proc.h"
#include "sid_sync.h"
#include "vadname.h"
#include "e_homing.h" 
#define AMR_MAGIC_NUMBER "#!AMR/n"
 
#define PCM_FRAME_SIZE 160 // 8khz 8000*0.02=160
#define MAX_AMR_FRAME_SIZE 32
#define AMR_FRAME_COUNT_PER_SECOND 50
//int amrEncodeMode[] = {4750, 5150, 5900, 6700, 7400, 7950, 10200, 12200}; // amr 编码方式
 
typedef struct
{
         char chChunkID[4];
         int nChunkSize;
}XCHUNKHEADER;
 
typedef struct
{
         short nFormatTag;
         short nChannels;
         int nSamplesPerSec;
         int nAvgBytesPerSec;
         short nBlockAlign;
         short nBitsPerSample;
}WAVEFORMAT;
 
typedef struct
{
         short nFormatTag;
         short nChannels;
         int nSamplesPerSec;
         int nAvgBytesPerSec;
         short nBlockAlign;
         short nBitsPerSample;
         short nExSize;
}WAVEFORMATX;
 
typedef struct
{
         char chRiffID[4];
         int nRiffSize;
         char chRiffFormat[4];
}RIFFHEADER;
 
typedef struct
{
         char chFmtID[4];
         int nFmtSize;
         WAVEFORMAT wf;
}FMTBLOCK;
 
// WAVE音频采样频率是8khz
// 音频样本单元数 = 8000*0.02 = 160 (由采样频率决定)
// 声道数 1 : 160
//        2 : 160*2 = 320
// bps决定样本(sample)大小
// bps = 8 --> 8位 unsigned char
//       16 --> 16位 unsigned short
extern unsigned char amrdone;
int AMRCodeInit(void);
int AMRCodeEnd(void);
int EncodeWAVEFileToAMRFile(char* inbuffer,unsigned char* outbuffer, int nChannels, int nBitsPerSample,int len);
 
// 将AMR文件解码成WAVE文件
int DecodeAMRFileToWAVEFile(const char* pchAMRFileName, const char* pchWAVEFilename);
 
#endif