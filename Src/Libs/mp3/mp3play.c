#include "mp3play.h"
#include "mp3dec.h"
#include "ff.h"
#include "main.h"
#include <log.h>
#include <string.h>
#include <wm8978.h>
#include <mw_audio.h>

static MP3_Para mp3para = {0};

static uint8_t MP2_ID3V1_Decode(uint8_t *buf);
static uint8_t MP2_ID3V2_Decode(uint8_t *buf, UINT br);
static uint8_t MP3_Get_Info(char *pcFilePath);

static FIL fsrc;
uint32_t br;

extern volatile uint8_t wavtransferend;
extern I2S_HandleTypeDef hi2s2;

static uint8_t MP2_ID3V1_Decode(uint8_t *buf)
{
    ID3V1_Tag *id3v1tag = NULL;
    id3v1tag = (ID3V1_Tag *)buf;

    if (strncmp("TAG", (char *)id3v1tag->id, 3) == 0)
    {
        if (id3v1tag->title[0])
        {
            strncpy((char *)mp3para.title, (char *)id3v1tag->title, 30);
        }
        if (id3v1tag->artist[0])
        {
            strncpy((char *)mp3para.artist, (char *)id3v1tag->artist, 30);
        }
    }
    else
    {
        return 1;
    }
    return 0;
}

static uint8_t MP2_ID3V2_Decode(uint8_t *buf, UINT br)
{
    ID3V2_TagHead *taghead = NULL;
    ID3V23_FrameHead *framehead = NULL;
    uint32_t t = 0;
    uint32_t tagsize = 0;
    uint32_t frame_size = 0;

    taghead = (ID3V2_TagHead *)buf;
    if (strncmp("ID3", (const char *)taghead->id, 3) == 0)
    {
        tagsize = ((uint32_t)taghead->size[0] << 21) | ((uint32_t)taghead->size[1] << 14) | ((uint16_t)taghead->size[2] << 7) | taghead->size[3];
        mp3para.datastart = tagsize;
        if (tagsize > br)
        {
            tagsize = br;
        }
        if (taghead->mversion < 3)
        {
#if 0
            printf("not supported mversion!\r\n");
#endif
            return 1;
        }
        t = 10;
        while (t < tagsize)
        {
            framehead = (ID3V23_FrameHead *)(buf + t);
            frame_size = ((uint32_t)framehead->size[0] << 24) | ((uint32_t)framehead->size[1] << 16) | ((uint32_t)framehead->size[2] << 8) | framehead->size[3];
            if (strncmp("TT2", (char *)framehead->id, 3) == 0 || strncmp("TIT2", (char *)framehead->id, 4) == 0)
            {
                strncpy((char *)mp3para.title, (char *)(buf + t + sizeof(ID3V23_FrameHead) + 1), AUDIO_MIN(frame_size - 1, MP3_TITSIZE_MAX - 1));
            }
            if (strncmp("TP1", (char *)framehead->id, 3) == 0 || strncmp("TPE1", (char *)framehead->id, 4) == 0) //找到歌曲艺术家帧
            {
                strncpy((char *)mp3para.artist, (char *)(buf + t + sizeof(ID3V23_FrameHead) + 1), AUDIO_MIN(frame_size - 1, MP3_ARTSIZE_MAX - 1));
            }
            t += frame_size + sizeof(ID3V23_FrameHead);
        }
    }
    else
    {
        mp3para.datastart = 0; //不存在ID3,mp3数据是从0开始
    }
    return 0;
}

static uint8_t MP3_Get_Info(char *pcFilePath)
{
    HMP3Decoder Mp3Decoder = NULL;
    MP3FrameInfo mp3frameinfo = {0};
    MP3_FrameXing *fxing = NULL;
    MP3_FrameVBRI *fvbri = NULL;

    uint8_t *temp_buf = NULL;
    int offset = 0;
    uint32_t p = 0;
    short samples_per_frame = 0; //一帧的采样个数
    uint32_t totframes = 0;		 //总帧数
    uint8_t fun_res = 0;

    FIL fsrc = {0};
    UINT br = 0;

    temp_buf = malloc(MP3_DECODE_SIZE); //为音乐文件解码分配缓存
    if (temp_buf == NULL)
    {
        fun_res = 1;
        goto end_point;
    }
    else
    {
        memset(temp_buf, 0, MP3_DECODE_SIZE);
    }
    fun_res = f_open(&fsrc, pcFilePath, FA_READ); //打开文件
    LOG(EDEBUG, "f_open %d", fun_res);
    fun_res = f_read(&fsrc, temp_buf, MP3_DECODE_SIZE, &br);
    LOG(EDEBUG, "f_read %d", fun_res);

    MP2_ID3V2_Decode(temp_buf, br);				   //解析ID3V2数据
    f_lseek(&fsrc, f_size(&fsrc) - 128);		   //偏移到倒数128的位置
    f_read(&fsrc, temp_buf, 128, &br);			   //读取128字节
    MP2_ID3V1_Decode(temp_buf);					   //解析ID3V1数据
    Mp3Decoder = MP3InitDecoder();				   //MP3解码申请内存
    f_lseek(&fsrc, mp3para.datastart);			   //偏移到数据开始的地方
    f_read(&fsrc, temp_buf, MP3_DECODE_SIZE, &br); //读取5K字节mp3数据
    offset = MP3FindSyncWord(temp_buf, br);		   //查找帧同步信息

    if ((offset >= 0) && (MP3GetNextFrameInfo(Mp3Decoder, &mp3frameinfo, &temp_buf[offset]) == 0)) //找到帧同步信息了,且下一阵信息获取正常
    {
        p = offset + 4 + 32;
        fvbri = (MP3_FrameVBRI *)(temp_buf + p);
        if (strncmp("VBRI", (char *)fvbri->id, 4) == 0) //存在VBRI帧(VBR格式)
        {
            if (mp3frameinfo.version == MPEG1)
            {
                samples_per_frame = 1152; //MPEG1,layer3每帧采样数等于1152
            }
            else
            {
                samples_per_frame = 576; //MPEG2/MPEG2.5,layer3每帧采样数等于576
            }
            totframes = ((uint32_t)fvbri->frames[0] << 24) | ((uint32_t)fvbri->frames[1] << 16) | ((uint16_t)fvbri->frames[2] << 8) | fvbri->frames[3]; //得到总帧数
            mp3para.totsec = totframes * samples_per_frame / mp3frameinfo.samprate;																		//得到文件总长度
        }
        else //不是VBRI帧,尝试是不是Xing帧(VBR格式)
        {
            if (mp3frameinfo.version == MPEG1) //MPEG1
            {
                p = (mp3frameinfo.nChans == 2) ? 32 : 17;
                samples_per_frame = 1152; //MPEG1,layer3每帧采样数等于1152
            }
            else
            {
                p = mp3frameinfo.nChans == 2 ? 17 : 9;
                samples_per_frame = 576; //MPEG2/MPEG2.5,layer3每帧采样数等于576
            }
            p += offset + 4;
            fxing = (MP3_FrameXing *)(temp_buf + p);
            if (strncmp("Xing", (char *)fxing->id, 4) == 0 || strncmp("Info", (char *)fxing->id, 4) == 0) //是Xng帧
            {
                if (fxing->flags[3] & 0X01) //存在总frame字段
                {
                    totframes = ((uint32_t)fxing->frames[0] << 24) | ((uint32_t)fxing->frames[1] << 16) | ((uint16_t)fxing->frames[2] << 8) | fxing->frames[3]; //得到总帧数
                    mp3para.totsec = totframes * samples_per_frame / mp3frameinfo.samprate;																		//得到文件总长度
                }
                else //不存在总frames字段
                {
                    //mp3para.totsec = fsrc.fsize/(mp3frameinfo.bitrate/8);
                }
            }
            else //CBR格式,直接计算总播放时间
            {
                //mp3para.totsec = fsrc.fsize/(mp3frameinfo.bitrate/8);
            }
        }
        mp3para.bitrate = mp3frameinfo.bitrate;		//得到当前帧的码率
        mp3para.samplerate = mp3frameinfo.samprate; //得到采样率.
        if (mp3frameinfo.nChans == 2)
        {
            mp3para.outsamples = mp3frameinfo.outputSamps; //输出PCM数据量大小
        }
        else
        {
            mp3para.outsamples = mp3frameinfo.outputSamps * 2;
        }
        //输出PCM数据量大小,对于单声道MP3,直接*2,补齐为双声道输出
#if 0
        printf("title:%s\r\n",mp3para.title);
        printf("artist:%s\r\n",mp3para.artist);
        printf("bitrate:%dbps\r\n",mp3para.bitrate);
        printf("samplerate:%d\r\n", mp3para.samplerate);
        printf("totalsec:%d\r\n",mp3para.totsec);
#endif
    }
    else
    {
        fun_res = 1;
        goto end_point;
    }

end_point:
    f_close(&fsrc);
    MP3FreeDecoder(Mp3Decoder); //释放内存
    free(temp_buf);
    return fun_res;
}

//填充PCM数据到DAC
//buf:PCM数据首地址
//size:pcm数据量(16位为单位)
//nch:声道数(1,单声道,2立体声)
static void MP3_Buffill(uint16_t *buf, uint16_t size, uint8_t nch)
{
    uint16_t i;
    uint16_t *p;

#if __OS
    xSemaphoreTake(xI2s2DMA_TxBinary, portMAX_DELAY);
    if (DMA_GetCurrentMemoryTarget(I2S2_TX_STREAM) == 0)
    {
        p = (uint16_t *)pAUDIO_SEND_BUF1;
    }
    else
    {
        p = (uint16_t *)pAUDIO_SEND_BUF0;
    }
#else
    if (1 == wavtransferend)
    {
        p = (uint16_t *)wm_audio_play_buffer;
    }
    else if (2 == wavtransferend)
    {
        p = (uint16_t *)wm_audio_play_buffer + MW_AUDIO_BUFFER_SIZE;
    }
#endif
    if (nch == 2)
    {
        for (i = 0; i < size; i++)
        {
            p[i] = buf[i];
        }
    }
    else //单声道
    {
        for (i = 0; i < size; i++)
        {
            p[2 * i] = buf[i];
            p[2 * i + 1] = buf[i];
        }
    }
}

uint8_t MP3_Play(char *pcFilePath)
{
    HMP3Decoder mp3decoder = NULL;
    MP3FrameInfo mp3frameinfo = {0};

    uint8_t *temp_buf = NULL;
    uint8_t *dec_buf = NULL;
    uint8_t *readptr = NULL;
    int offset = 0;
    int outofdata = 0;
    int bytesleft = 0;
    int err = 0;
    uint8_t fun_res = 0;

    fun_res = MP3_Get_Info(pcFilePath);
    if (fun_res)
    {
        LOG(EERROR, "get mp3 info failed, ret:%d", fun_res);
        goto end_point;
    }
    temp_buf = malloc(MP3_DECODE_SIZE);
    dec_buf = malloc(MW_AUDIO_BUFFER_SIZE);
    if (!(dec_buf && temp_buf))
    {
        fun_res = 1;
        LOG(EERROR, "get mp3 malloc failed, ret:%d", fun_res);
        goto end_point;
    }
    WM8978_ADDA_Cfg(1, 0);
    WM8978_Input_Cfg(0, 0, 0);
    WM8978_Output_Cfg(1, 0);
    WM8978_HPvol_Set(0, 0);
    WM8978_SPKvol_Set(63);
    WM8978_MIC_Gain(0);
    WM8978_I2S_Cfg(2, 0);

    HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t *)wm_audio_play_buffer,
                                  (uint16_t *)wm_audio_record_buffer, sizeof(wm_audio_record_buffer) / sizeof(wm_audio_record_buffer[0]) / 2);

    mp3decoder = MP3InitDecoder();
    if (!mp3decoder)
    {
        fun_res = 1;
        LOG(EERROR, "get mp3 decode failed, ret:%d", fun_res);
        goto end_point;
    }
    f_open(&fsrc, pcFilePath, FA_READ);
    f_lseek(&fsrc, mp3para.datastart);
    while (1)
    {
        readptr = temp_buf;
        offset = 0;
        outofdata = 0;
        bytesleft = 0;

        if (f_read(&fsrc, temp_buf, MP3_DECODE_SIZE, &br))
        {
            LOG(EERROR, "get mp3 read failed");
            break;
        }
        if (br == 0)
        {
            LOG(EDEBUG, "get mp3 read over");
            break;
        }
        bytesleft += br;
        err = 0;
        while (!outofdata)
        {
            offset = MP3FindSyncWord(readptr, bytesleft);
            if (offset < 0)
            {
                outofdata = 1;
            }
            else
            {
                readptr += offset;
                bytesleft -= offset;
                err = MP3Decode(mp3decoder, &readptr, &bytesleft, (short *)dec_buf, 0);
                if (err != 0)
                {
#if 0
                    printf("decode error:%d\r\n",err);
#endif
                    break;
                }
                else
                {
                    MP3GetLastFrameInfo(mp3decoder, &mp3frameinfo);
                    if (mp3para.bitrate != mp3frameinfo.bitrate)
                    {
                        mp3para.bitrate = mp3frameinfo.bitrate;
                    }
                    MP3_Buffill((uint16_t *)dec_buf, mp3frameinfo.outputSamps, mp3frameinfo.nChans);
                }
                if (bytesleft < MAINBUF_SIZE * 2)
                {
                    memmove(temp_buf, readptr, bytesleft);
                    f_read(&fsrc, temp_buf + bytesleft, MP3_DECODE_SIZE - bytesleft, &br);
                    if (br < MP3_DECODE_SIZE - bytesleft)
                    {
                        memset(temp_buf + bytesleft + br, 0, MP3_DECODE_SIZE - bytesleft - br);
                    }
                    bytesleft = MP3_DECODE_SIZE;
                    readptr = temp_buf;
                }
            }
        }
    }
    memset(wm_audio_play_buffer, 0, MW_AUDIO_BUFFER_SIZE * 2);
end_point:
    f_close(&fsrc);
    free(temp_buf);
    free(dec_buf);
    HAL_I2S_DMAStop(&hi2s2);
    MP3FreeDecoder(mp3decoder);
    return fun_res;
}

static uint8_t MP3_GetBuffer_Info(const unsigned char *pcBuf)
{
    HMP3Decoder Mp3Decoder = NULL;
    MP3FrameInfo mp3frameinfo = {0};
    MP3_FrameXing *fxing = NULL;
    MP3_FrameVBRI *fvbri = NULL;

    uint8_t *temp_buf = NULL;
    int offset = 0;
    uint32_t p = 0;
    short samples_per_frame = 0;
    uint32_t totframes = 0;
    uint8_t fun_res = 0;

    temp_buf = malloc(MP3_DECODE_SIZE);
    if (temp_buf == NULL)
    {
        fun_res = 1;
        goto end_point;
    }
    else
    {
        memset(temp_buf, 0, MP3_DECODE_SIZE);
    }

    Mp3Decoder = MP3InitDecoder();
    mp3para.datastart = 0;
    memcpy(temp_buf, pcBuf, MP3_DECODE_SIZE);
    offset = MP3FindSyncWord(temp_buf, MP3_DECODE_SIZE);

    if ((offset >= 0) && (MP3GetNextFrameInfo(Mp3Decoder, &mp3frameinfo, &temp_buf[offset]) == 0))
    {
        p = offset + 4 + 32;
        fvbri = (MP3_FrameVBRI *)(temp_buf + p);
        if (strncmp("VBRI", (char *)fvbri->id, 4) == 0)
        {
            if (mp3frameinfo.version == MPEG1)
            {
                samples_per_frame = 1152;
            }
            else
            {
                samples_per_frame = 576;
            }
            totframes = ((uint32_t)fvbri->frames[0] << 24) | ((uint32_t)fvbri->frames[1] << 16) | ((uint16_t)fvbri->frames[2] << 8) | fvbri->frames[3];
            mp3para.totsec = totframes * samples_per_frame / mp3frameinfo.samprate;
        }
        else
        {
            if (mp3frameinfo.version == MPEG1)
            {
                p = (mp3frameinfo.nChans == 2) ? 32 : 17;
                samples_per_frame = 1152;
            }
            else
            {
                p = mp3frameinfo.nChans == 2 ? 17 : 9;
                samples_per_frame = 576;
            }
            p += offset + 4;
            fxing = (MP3_FrameXing *)(temp_buf + p);
            if (strncmp("Xing", (char *)fxing->id, 4) == 0 || strncmp("Info", (char *)fxing->id, 4) == 0)
            {
                if (fxing->flags[3] & 0X01)
                {
                    totframes = ((uint32_t)fxing->frames[0] << 24) | ((uint32_t)fxing->frames[1] << 16) | ((uint16_t)fxing->frames[2] << 8) | fxing->frames[3];
                    mp3para.totsec = totframes * samples_per_frame / mp3frameinfo.samprate;
                }
                else
                {
                    //mp3para.totsec = fsrc.fsize/(mp3frameinfo.bitrate/8);
                }
            }
            else
            {
                //mp3para.totsec = fsrc.fsize/(mp3frameinfo.bitrate/8);
            }
        }
        mp3para.bitrate = mp3frameinfo.bitrate;
        mp3para.samplerate = mp3frameinfo.samprate;
        if (mp3frameinfo.nChans == 2)
        {
            mp3para.outsamples = mp3frameinfo.outputSamps;
        }
        else
        {
            mp3para.outsamples = mp3frameinfo.outputSamps * 2;
        }
    }
    else
    {
        fun_res = 1;
        goto end_point;
    }

end_point:
    MP3FreeDecoder(Mp3Decoder);
    free(temp_buf);
    return fun_res;
}

uint8_t Mp3Buffer_Play(const unsigned char *pcBuf, uint32_t ulBufLen)
{
    HMP3Decoder mp3decoder = NULL;
    MP3FrameInfo mp3frameinfo = {0};

    uint8_t *temp_buf = NULL;
    uint8_t *dec_buf = NULL;
    uint8_t *readptr = NULL;
    int offset = 0;
    int outofdata = 0;
    int bytesleft = 0;
    int err = 0;
    uint8_t fun_res = 0;
    unsigned char *pcBufNAddr = (unsigned char *)pcBuf;

    fun_res = MP3_GetBuffer_Info(pcBuf);
    if (fun_res)
    {
        goto end_point;
    }
    temp_buf = malloc(MP3_DECODE_SIZE);
    dec_buf = malloc(MW_AUDIO_BUFFER_SIZE);
    if (!(dec_buf && temp_buf))
    {
        fun_res = 1;
        goto end_point;
    }
    WM8978_ADDA_Cfg(1, 0);
    WM8978_Input_Cfg(0, 0, 0);
    WM8978_Output_Cfg(1, 0);
    WM8978_HPvol_Set(0, 0);
    WM8978_SPKvol_Set(63);
    WM8978_MIC_Gain(0);
    WM8978_I2S_Cfg(2, 0);

    HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t *)wm_audio_play_buffer,
                                  (uint16_t *)wm_audio_record_buffer, sizeof(wm_audio_record_buffer) / sizeof(wm_audio_record_buffer[0]) / 2);

    mp3decoder = MP3InitDecoder();
    if (!mp3decoder)
    {
        fun_res = 1;
        goto end_point;
    }
    for (uint32_t ulDeccnt = 0; pcBufNAddr < pcBuf + ulBufLen; ulDeccnt++)
    {
        readptr = temp_buf;
        offset = 0;
        outofdata = 0;
        bytesleft = 0;

        memcpy(temp_buf, pcBufNAddr, MP3_DECODE_SIZE);
        pcBufNAddr += MP3_DECODE_SIZE;
        bytesleft += MP3_DECODE_SIZE;
        err = 0;
        while (!outofdata)
        {
            offset = MP3FindSyncWord(readptr, bytesleft);
            if (offset < 0)
            {
                outofdata = 1;
            }
            else
            {
                readptr += offset;
                bytesleft -= offset;
                err = MP3Decode(mp3decoder, &readptr, &bytesleft, (short *)dec_buf, 0);
                if (err != 0)
                {
#if 0
                    printf("decode error:%d\r\n",err);
#endif
                    break;
                }
                else
                {
                    MP3GetLastFrameInfo(mp3decoder, &mp3frameinfo);
                    if (mp3para.bitrate != mp3frameinfo.bitrate)
                    {
                        mp3para.bitrate = mp3frameinfo.bitrate;
                    }
                    MP3_Buffill((uint16_t *)dec_buf, mp3frameinfo.outputSamps, mp3frameinfo.nChans); //填充pcm数据
                }
                if (bytesleft < MAINBUF_SIZE * 2)
                {
                    memmove(temp_buf, readptr, bytesleft);

                    memcpy(temp_buf + bytesleft, pcBufNAddr, MP3_DECODE_SIZE - bytesleft);
                    pcBufNAddr += MP3_DECODE_SIZE - bytesleft;
                    bytesleft = MP3_DECODE_SIZE;
                    readptr = temp_buf;
                }
            }
        }
    }
    //xSemaphoreTake(xI2s2DMA_TxBinary, portMAX_DELAY);
    memset(wm_audio_play_buffer, 0, MW_AUDIO_BUFFER_SIZE * 2);
    //memset(pAUDIO_SEND_BUF1, 0, AUDIO_BUFFER_SIZE);
end_point:
    free(temp_buf);
    free(dec_buf);
    MP3FreeDecoder(mp3decoder);
    return fun_res;
}