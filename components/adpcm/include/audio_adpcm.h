/*********************************************************************
 *
 *                  AUDIO ADPCM wrapper header
 *
 *********************************************************************
 * FileName:        audio_adpcm.h
 * Dependencies:
 * Processor:       PIC32
 *
 * Complier:        MPLAB Cxx
 *                  MPLAB IDE
 * Company:         Microchip Technology, Inc.
 *
 * Software License Agreement
 * Microchip Audio Library ?PIC32 Software.
 * Copyright ?2008 Microchip Technology Inc.  All rights reserved.
 * 
 * Microchip licenses the Software for your use with Microchip microcontrollers
 * and Microchip digital signal controllers pursuant to the terms of the
 * Non-Exclusive Software License Agreement accompanying this Software.
 *
 * SOFTWARE AND DOCUMENTATION ARE PROVIDED ?S IS?WITHOUT WARRANTY
 * OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION,
 * ANY WARRANTY OF MERCHANTABILITY, TITLE, NON-INFRINGEMENT AND FITNESS
 * FOR A PARTICULAR PURPOSE.
 * MICROCHIP AND ITS LICENSORS ASSUME NO RESPONSIBILITY FOR THE ACCURACY,
 * RELIABILITY OR APPLICATION OF THE SOFTWARE AND DOCUMENTATION.
 * IN NO EVENT SHALL MICROCHIP OR ITS LICENSORS BE LIABLE OR OBLIGATED
 * UNDER CONTRACT, NEGLIGENCE, STRICT LIABILITY, CONTRIBUTION, BREACH
 * OF WARRANTY, OR OTHER LEGAL EQUITABLE THEORY ANY DIRECT OR INDIRECT
 * DAMAGES OR EXPENSES INCLUDING BUT NOT LIMITED TO ANY INCIDENTAL,
 * SPECIAL, INDIRECT, PUNITIVE OR CONSEQUENTIAL DAMAGES, LOST PROFITS
 * OR LOST DATA, COST OF PROCUREMENT OF SUBSTITUTE GOODS, TECHNOLOGY,
 * SERVICES, OR ANY CLAIMS BY THIRD PARTIES (INCLUDING BUT NOT LIMITED
 * TO ANY DEFENSE THEREOF), OR OTHER SIMILAR COSTS.
 *
 *$Id: $
 ********************************************************************/

#ifndef _AUDIO_ADPCM_H_
#define _AUDIO_ADPCM_H_ 

#define AUDIO_SUPPORT_RECORD
#define AUDIO_SUPPORT_PLAY
#define ADPCM_ENCODING

#ifdef ADPCM_ENCODING	

//------------------------------------------------------------------------------------------------------------------
#define	AUDIO_ADPCM_NIBBLES_LE	0	// order of ADPCM nibbles in the file: BE or LE

int AudioGetAdpcmEndianess(void);

typedef enum
{
	AUDIO_RES_OK,			// success
	AUDIO_RES_UNSUPPORTED,		// unsupported format, operation, etc.
	AUDIO_RES_BAD_CALL,		// calling PLAY while recording, etc.
	AUDIO_RES_OUT_OF_MEM,		// not enough memory (lower the number of processed samples?)
	AUDIO_RES_LOW_RESOLUTION,	// not enough resolution could be obtained. increase the system frequency, lower the audio sample frequency
	AUDIO_RES_MIN_FREQ,		// AUDIO_MIN_WORK_FREQ violated. Increase sampling frequency or lower the play/record rate.
	AUDIO_RES_WRITE_ERROR,		// some write operation could not be completed
	AUDIO_RES_READ_ERROR,		// some read operation could not be completed
	AUDIO_RES_DSTREAM_ERROR,	// data error in the input stream
	AUDIO_RES_SSTREAM_ERROR,	// structure error in the input stream
	AUDIO_RES_BUFF_ERROR,		// allocated buffer too small, increase.
	//      
	AUDIO_RES_HIGH_RESOLUTION,	// the work frequency is too high, lower it down
	AUDIO_RES_STREAM_CB_ERROR,	// stream callback needed but not supported
	AUDIO_RES_STREAM_EMPTY,		// missing data in the input stream
	AUDIO_RES_STREAM_END,		// end of stream
	AUDIO_RES_STREAM_ID_ERROR,	// different stream, etc
	AUDIO_RES_COMPRESS_ERROR,	// compression not supported error
	AUDIO_RES_DMA_UNSUPPORTED,	// DMA transfer not supported
	AUDIO_RES_BPS_UNSUPPORTED,	// unsupported bits per sample
	AUDIO_RES_DONE,			// operation completed
	
}AUDIO_RES;	// The result of an audio operation

typedef enum
{
	CODEC_ALLOCATE_NONE	= 0,
	CODEC_ALLOCATE_IN	= 0x1,
	CODEC_ALLOCATE_OUT	= 0x2,
	CODEC_ALLOCATE_INOUT	= CODEC_ALLOCATE_IN|CODEC_ALLOCATE_OUT
}eBuffMask;

//------------------------------------------------------------------------------------------------------------------
typedef void(*pProgressFnc)(int nBytes);			// progress display function
								// during an lengthy operation (AudioConvert)
								// the library will use this callback
								// so that caller can regain control
typedef struct
{
	int		progressStep;		// no of bytes to process before calling the progress callback
	int		progressCnt;		// current counter
	pProgressFnc	progressFnc;		// progress callback
}progressDcpt;	// encoder/decoder progress activity descriptor

typedef void*	audioCodecHandle;	// handle to an audio codec
//------------------------------------------------------------------------------------------------------------------


typedef struct __attribute__((packed))
{
	char			sFmt;		// AUDIO_STREAM_FORMAT: the stream format: AUDIO_STREAM_FREE, AUDIO_STREAM_WAV, etc	
	char			aFmt;		// AUDIO_FORMAT: the encoding data format in the stream. 
	char			aOp;		// AUDIO_OP: PLAY/RECORD type of operation
	char			bitsPerSample;	// 8/16/24/32. Always 16 in this implementation.
	//
	char			numChannels;	// 1 for mono, 2 stereo, etc
	char			activeChannel;	// 1, 2, etc playing mono, 0 for all of them. Just one channel supported in this implementation.
	char			lEndian;	// true if LE, false if BE
	char			reserved;	// padding
	//
	int			streamSz;	// how many total (i.e. for all channels) data bytes available in the stream
	int			sampleRate;	// play/record rate, Hz
	int			bitRate;	// compressed bps
}sStreamInfo;	// an audio stream info data
//  local definitions

typedef struct 
{
	short			prevsample;		/* Predicted adpcm sample *///kafuse
	short			previndex;		/* Index into step size table *///kafuse
}AdpcmState;



typedef struct
{
	AdpcmState	state;		// internal state of the encoder/decoder btw successive operations
	int		sLE;		// samples in LE/BE format
	void*		inBuff;		// internal buffer holding  at least nProcSamples packed ADPCM samples
	int		inBuffSize;
	int		nInBytes;	// currently processed bytes
	void*		outBuff;	// output buffer
	int		outBuffSize;
	int		nOutBytes;	// result bytes (encoding/decoding)
}AdpcmCodecDcpt;	// adpcm codec descriptor
//------------------------------------------------------------------------------------------------------------------
// low level access functions
#ifdef AUDIO_SUPPORT_PLAY
short		ADPCMDecodeSample(unsigned char code, AdpcmState *state);
void		ADPCMDecodeBuffer(AdpcmCodecDcpt* pDcpt);
#endif

#ifdef AUDIO_SUPPORT_RECORD
unsigned char	ADPCMEncodeSample(short sample, AdpcmState *state);
void		ADPCMEncodeBuffer(AdpcmCodecDcpt* pDcpt);
#endif

int		AdpcmAllocateBuffers(AdpcmCodecDcpt* pDcpt, eBuffMask buffMask);
//

// returns the number of samples needed in the decoder out buffer
int			AdpcmDecoderOutSamples(int procSamples, sStreamInfo* pInfo);

// init the decoder
AUDIO_RES		AdpcmInitDecoder(int nProcSamples, sStreamInfo* pInfo, eBuffMask buffMask, audioCodecHandle* pHndl);

// start decoding data
AUDIO_RES		AdpcmReadDecodeData(audioCodecHandle h, progressDcpt* pPrgDcpt);

// clean-up, after done
void			AdpcmCleanUpDecoder(audioCodecHandle h);

// get the output work buffer size
int			AdpcmGetOutBuffSize(audioCodecHandle h);

// get the working output buffer
void*			AdpcmGetOutBuff(audioCodecHandle h);

// set the working output buffer
void			AdpcmSetOutBuff(audioCodecHandle h, void* pBuff);

// get the encoded/decoded bytes and clear internal counter
int			AdpcmGetOutBytes(audioCodecHandle h, int clr);

// returns the number of samples needed in the encoder input buffer
int			AdpcmEncoderInSamples(int procSamples, sStreamInfo* pInfo);

// init the encoder
AUDIO_RES		AdpcmInitEncoder(int nProcSamples, sStreamInfo* pInfo, eBuffMask buffMask, audioCodecHandle* pHndl);

// start decoding data
AUDIO_RES		AdpcmEncodeWriteData(audioCodecHandle h, int nBytes, progressDcpt* pPrgDcpt);


// get the output work buffer size
int			AdpcmGetInBuffSize(audioCodecHandle h);

// get the working input buffer
void*			AdpcmGetInBuff(audioCodecHandle h);


// set the working output buffer
void			AdpcmSetInBuff(audioCodecHandle h, void* pBuff);


// clean-up, when done
#define			AdpcmCleanUpEncoder	AdpcmCleanUpDecoder


// check the play/record format
AUDIO_RES		AdpcmCheckFormat(sStreamInfo* pInfo);

#endif	// ADPCM_ENCODING	


#endif	// _AUDIO_ADPCM_H_

