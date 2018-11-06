#include "amrFileCodec.h"
#include "esp_log.h"
static const char *TAG = "amrFileCodec";

static Speech_Encode_FrameState *speech_encoder_state = NULL;
static sid_syncState *sid_state = NULL;
/* input speech vector */
static short speech[160];
static Word16 serial[250];
         /* pointer to encoder state structure */
static int *enstate;
//         unsigned int inaddr=0,outaddr=0;
         /* requested mode */
static enum Mode req_mode = MR122;
static enum Mode used_mode;
static enum TXFrameType tx_type;
unsigned char amrdone;
// ´ÓWAVEÎÄ¼þÖÐÌø¹ýWAVEÎÄ¼þÍ·£¬Ö±½Óµ½PCMÒôÆµÊý¾Ý
void SkipToPCMAudioData(FILE* fpwave)
{
         RIFFHEADER riff;
         FMTBLOCK fmt;
         XCHUNKHEADER chunk;
         WAVEFORMATX wfx;
         int bDataBlock = 0;
 
         // 1. ¶ÁRIFFÍ·
         fread(&riff, 1, sizeof(RIFFHEADER), fpwave);
 
         // 2. ¶ÁFMT¿é - Èç¹û fmt.nFmtSize>16 ËµÃ÷ÐèÒª»¹ÓÐÒ»¸ö¸½Êô´óÐ¡Ã»ÓÐ¶Á
         fread(&chunk, 1, sizeof(XCHUNKHEADER), fpwave);
         if ( chunk.nChunkSize>16 )
         {
                   fread(&wfx, 1, sizeof(WAVEFORMATX), fpwave);
         }
         else
         {
                   memcpy(fmt.chFmtID, chunk.chChunkID, 4);
                   fmt.nFmtSize = chunk.nChunkSize;
                   fread(&fmt.wf, 1, sizeof(WAVEFORMAT), fpwave);
         }
 
         // 3.×ªµ½data¿é - ÓÐÐ©»¹ÓÐfact¿éµÈ¡£
         while(!bDataBlock)
         {
                   fread(&chunk, 1, sizeof(XCHUNKHEADER), fpwave);
                   if ( !memcmp(chunk.chChunkID, "data", 4) )
                   {
                            bDataBlock = 1;
                            break;
                   }
                   // ÒòÎªÕâ¸ö²»ÊÇdata¿é,¾ÍÌø¹ý¿éÊý¾Ý
                   fseek(fpwave, chunk.nChunkSize, SEEK_CUR);
         }
}
 
// ´ÓWAVEÎÄ¼þ¶ÁÒ»¸öÍêÕûµÄPCMÒôÆµÖ¡
// ·µ»ØÖµ: 0-´íÎó >0: ÍêÕûÖ¡´óÐ¡
int ReadPCMFrame(short speech[], char* inbuffer, int nChannels, int nBitsPerSample,int len)
{
//         int nread=0;
           int x = 0, y=0;
         unsigned short ush1=0, ush2=0, ush=0;
//         unsigned char inbuffer[PCM_FRAME_SIZE*2];
         // 原始PCM音频帧数�?
         unsigned char pcmFrame_8b1[PCM_FRAME_SIZE];
         unsigned char pcmFrame_8b2[PCM_FRAME_SIZE<<1];
         unsigned short pcmFrame_16b1[PCM_FRAME_SIZE];
         unsigned short pcmFrame_16b2[PCM_FRAME_SIZE<<1];
//		 nread = fread(inbuffer,1,PCM_FRAME_SIZE*nChannels*nBitsPerSample/8,inputbuffer);
         if(len/(PCM_FRAME_SIZE*nChannels*nBitsPerSample/8))
         {
         if (nBitsPerSample==8 && nChannels==1)
         {
                   for(x=0; x<PCM_FRAME_SIZE; x++)
                   {
                       pcmFrame_8b1[x]=(unsigned char)(*(inbuffer+x));
                       speech[x] =(short)((short)pcmFrame_8b1[x] << 7);
                   }
         }
         else
         if (nBitsPerSample==8 && nChannels==2)
         {
				   for(x=0;x<PCM_FRAME_SIZE*2;x++)				   
				   pcmFrame_8b1[x]=(unsigned char)(*(inbuffer+x));
                   for( x=0, y=0; y<PCM_FRAME_SIZE; y++,x+=2 )
                   {
                      speech[y] =(short)((short)pcmFrame_8b2[x+0] << 7);
                   }
         }
         else
         if (nBitsPerSample==16 && nChannels==1)
         {
                   for(x=0; x<PCM_FRAME_SIZE; x++)
                   {   
                       pcmFrame_16b1[x]=(short)((short)(*(inbuffer+2*x))+(unsigned short)(*(inbuffer+2*x+1)<<8));
                       speech[x] = (short)pcmFrame_16b1[x];
                   }
         }
         else
         if (nBitsPerSample==16 && nChannels==2)
         {
                   for(x=0;x<PCM_FRAME_SIZE*2;x++)				   	
                       pcmFrame_16b2[x]=(unsigned short)((short)(*(inbuffer+2*x))+(unsigned short)(*(inbuffer+2*x+1)<<8));
                   for( x=0, y=0; y<PCM_FRAME_SIZE; y++,x+=2 )
                   {
                       speech[y] = (short)((int)((int)pcmFrame_16b2[x+0] + (int)pcmFrame_16b2[x+1])) >> 1;
                   }
         } 
         // 如果读到的数据不是一个完整的PCM�? 就返�?
         return PCM_FRAME_SIZE*nChannels*nBitsPerSample/8;
         }
		 else
         return 0;
}
 
// WAVEÒôÆµ²ÉÑùÆµÂÊÊÇ8khz
// ÒôÆµÑù±¾µ¥ÔªÊý = 8000*0.02 = 160 (ÓÉ²ÉÑùÆµÂÊ¾ö¶¨)
// ÉùµÀÊý 1 : 160
//        2 : 160*2 = 320
// bps¾ö¶¨Ñù±¾(sample)´óÐ¡
// bps = 8 --> 8Î» unsigned char
//       16 --> 16Î» unsigned short
int AMRCodeInit(void)
{
	
	speech_encoder_state = NULL;
	sid_state = NULL;
	int dtx = 0;	
	if (   Speech_Encode_Frame_init(&speech_encoder_state, dtx, "encoder")
	   || sid_sync_init (&sid_state))
		return 2;
	else return 0;

}
int AMRCodeEnd(void)
{
	Speech_Encode_Frame_exit(&speech_encoder_state);
	sid_sync_exit (&sid_state);
    return 0;
}
int EncodeWAVEFileToAMRFile(char* inbuffer, unsigned char* outbuffer, int nChannels, int nBitsPerSample,int len)
{        
         int bytes = 0;
         int i;       
         Word16 packed_size;
         while(1)
         {
                   // read one pcm frame
                   if (!ReadPCMFrame(speech, inbuffer, nChannels, nBitsPerSample,len)) break;
				   len-=PCM_FRAME_SIZE*nChannels*nBitsPerSample/8;
				   inbuffer+=PCM_FRAME_SIZE*nChannels*nBitsPerSample/8;

				   for (i = 0; i < 250; i++)
					   serial[i] = 0;
				   
				   /* check for homing frame */
				   encoder_homing_frame_test(speech);
                   Speech_Encode_Frame(speech_encoder_state, req_mode,speech, &serial[1], &used_mode); 
                   sid_sync (sid_state, used_mode, &tx_type);
                   packed_size = PackBits(used_mode, req_mode, tx_type, &serial[1], outbuffer);
				   outbuffer+=packed_size;
                   bytes += packed_size;
         }
		 amrdone=1;
         return bytes;
}
