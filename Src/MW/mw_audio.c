#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <i2s.h>
#include <WM8978.h>
#include <ff.h>
#include <wavplay.h>
#include <log.h>
#include <mw_file.h>
#include "mw_audio.h"

// 需要使用volatile, 否则播放不正常，值易变，会导致使用副本变量不会得到立即更新
volatile uint8_t wavtransferend=0;	//i2s传输完成标志
volatile uint8_t recordend=0;		//i2sbufx指示标志
uint8_t wm_audio_record_buffer[MW_AUDIO_BUFFER_SIZE*2] = {0};
uint8_t wm_audio_play_buffer[MW_AUDIO_BUFFER_SIZE*2] = {0};

static void wm_audio_rx_callback(void);
static void wm_audio_tx_callback(void);

__weak uint8_t wm_audio_begin()
{
    return WM8978_Init();
}

__weak uint8_t wm_audio_end()
{
    Audio.stop();
    return 0;
}

__weak uint8_t wm_audio_reset()
{
    return 0;
}

__weak uint8_t wm_audio_config(mw_audio_config_t *pstConfig)
{
    uint8_t uRes = 0;
    
    memcpy(&Audio.audio_config, pstConfig, sizeof(mw_audio_config_t));
    if (16 == pstConfig->uSampleBit) {
        WM8978_I2S_Cfg(2,0);		//飞利浦标准,16位数据长度
    } else if ((24 == pstConfig->uSampleBit) || (32 == pstConfig->uSampleBit)) {
        WM8978_I2S_Cfg(2,2);		//飞利浦标准,24位数据长度
    }
    else {
       return 1; 
    }
    
    return uRes;
}

__weak uint8_t wm_audio_record()
{
    uint8_t uRes = 0;
    
    WM8978_ADDA_Cfg(0,1);		//开启ADC
    WM8978_Input_Cfg(1,0,0);	//开启输入通道(MIC)
    WM8978_Output_Cfg(0,0);		//开启BYPASS输出
    WM8978_HPvol_Set(0,0);
    WM8978_SPKvol_Set(0);
    WM8978_MIC_Gain(Audio.audio_config.uVolume);		//MIC增益设置(多年实战下来的值)
    WM8978_I2S_Cfg(2,0);		//飞利浦标准,16位数据长度
    
    Audio.uType = E_MW_AUDIO_TYPE_RECORD;
    
    //HAL_I2S_Receive_DMA(&hi2s2, (uint16_t *)wm_audio_record_buffer, sizeof(wm_audio_record_buffer)/sizeof(wm_audio_record_buffer[0])/2);
    HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t *)wm_audio_play_buffer, 
        (uint16_t *)wm_audio_record_buffer, sizeof(wm_audio_record_buffer)/sizeof(wm_audio_record_buffer[0])/2);
    //__HAL_I2S_DISABLE(&hi2s2);
    //hi2s2.hdmarx->XferHalfCpltCallback = I2SEx_DMAHalfCplt;
    /* Set the I2S Rx DMA transfer complete callback */
    //hi2s2.hdmarx->XferCpltCallback  = I2SEx_RxDMACplt;
    return uRes;
}

__weak uint8_t wm_audio_play()
{
    uint8_t uRes = 0;
    
    WM8978_ADDA_Cfg(1,0);	    // 开启DAC
	WM8978_Input_Cfg(0,0,0);    // 关闭输入通道
	WM8978_Output_Cfg(1,0);	    // 开启DAC输出
    
    int8_t volume = Audio.audio_config.uVolume;
    WM8978_HPvol_Set(volume,volume);	// 耳机音量设置
	WM8978_SPKvol_Set(volume);		// 喇叭音量设置
    
    Audio.uType = E_MW_AUDIO_TYPE_PLAY;
    
    //HAL_I2S_Transmit_DMA(&hi2s2, (uint16_t *)wm_audio_play_buffer, sizeof(wm_audio_play_buffer)/sizeof(wm_audio_play_buffer[0])/2);
    HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t *)wm_audio_play_buffer, 
        (uint16_t *)wm_audio_record_buffer, sizeof(wm_audio_play_buffer)/sizeof(wm_audio_play_buffer[0])/2);
    return uRes;
}

__weak void wm_audio_stop()
{
    Audio.uType = E_MW_AUDIO_TYPE_NONE;
    HAL_I2S_DMAStop(&hi2s2);
}

mw_audio_t Audio = {
    E_MW_AUDIO_TYPE_NONE,
    {16000, 16, 1, 0, NULL},
    wm_audio_begin,
    wm_audio_end,
    wm_audio_reset,
    wm_audio_config,
    wm_audio_record,
    wm_audio_play,
    NULL,
    wm_audio_stop,
};

// demo for record test
__weak void wm_audio_record_test()
{
    File file;
    uint32_t bw, total = 0;
    uint8_t uRes = 0;
    uint32_t uStart, uEnd;
    
    mw_file_init(&file);
    uRes = file.open(&file, "0:/record.pcm", FILE_CREATE_ALWAYS | FILE_WRITE);
    if (0 != uRes) {
        LOG(EDEBUG, "open failed");
        return ;
    }
    
    mw_audio_config_t config;
    config.uSampleRate = 16000;
    config.uSampleBit = 16;
    config.uChannel = 2;
    config.uVolume = 50;
    config.callback = NULL;
    
    uRes = Audio.begin();
    uRes |= Audio.config(&config);
    Audio.record();
    LOG(EDEBUG, "record start");
    while(E_MW_AUDIO_TYPE_RECORD == Audio.uType) {
        if (2 == recordend) {
            recordend = 0;
            bw = file.write(&file, wm_audio_record_buffer, MW_AUDIO_BUFFER_SIZE);
            total += bw;
            if (bw != MW_AUDIO_BUFFER_SIZE)break;
            if (total >= 320000) break;
        } else if (1 == recordend) {
            recordend = 0;
            bw = file.write(&file, wm_audio_record_buffer+MW_AUDIO_BUFFER_SIZE, MW_AUDIO_BUFFER_SIZE); 
            total += bw;
            if (bw != MW_AUDIO_BUFFER_SIZE)break;
            if (total >= 320000) break;
        }
    }
    LOG(EDEBUG, "record end");
    file.close(&file);
    Audio.stop();
}

// demo for play test
__weak void wm_audio_play_test()
{
    File file;
    uint32_t bw, total = 0;
    uint8_t uRes = 0;
    uint32_t uStart, uEnd;
    
    mw_file_init(&file);
    uRes = file.open(&file, "0:/record.pcm", FILE_READ);
    //uRes = file.open(&file, "0:/music/arcs.wav", FILE_READ);
    if (0 != uRes) {
        LOG(EDEBUG, "open failed");
        return ;
    }
    
    mw_audio_config_t config;
    config.uSampleRate = 16000;
    config.uSampleBit = 16;
    config.uChannel = 2;
    config.uVolume = 63;
    config.callback = NULL;
    config.userdata = NULL;
    
    uRes = Audio.begin();
    uRes |= Audio.config(&config);
    Audio.play();
    LOG(EDEBUG, "play start");
    while(E_MW_AUDIO_TYPE_PLAY == Audio.uType) {
        if (1 == wavtransferend) {
            wavtransferend = 0;
            bw = file.read(&file, wm_audio_play_buffer, MW_AUDIO_BUFFER_SIZE);
            if(bw != MW_AUDIO_BUFFER_SIZE) {
                LOG(EDEBUG, "play end");
                break;
            }
            //LOG(EDEBUG, "bw:%d", bw);
        } else if (2 == wavtransferend) {
            wavtransferend = 0;
            bw = file.read(&file, wm_audio_play_buffer+MW_AUDIO_BUFFER_SIZE, MW_AUDIO_BUFFER_SIZE);
            if(bw != MW_AUDIO_BUFFER_SIZE) {
                LOG(EDEBUG, "play end");
                break;
            }
            //LOG(EDEBUG, "bw:%d", bw);
        }
        
    }
    file.close(&file);
    Audio.stop();
}

// demo for wavplay
__weak void wm_audio_wavplay_test()
{
    uint8_t uRes = 0;
    
    struct wav_ctx ctx;
    uRes = wav_read(&ctx, "0:/music/arcs.wav");
    if (uRes) {
        LOG(EERROR, "wav_read failed, error:%d", uRes);
        return ;
    }
    
    mw_audio_config_t config;
    config.uSampleRate = ctx.chunk_fmt.sample_rate;
    config.uSampleBit = ctx.chunk_fmt.bits_per_sample;
    config.uChannel = ctx.chunk_fmt.num_channels;
    config.uVolume = 63;
    config.callback = NULL;
    config.userdata = &ctx;
    
    uRes = Audio.begin();
    uRes |= Audio.config(&config);
    Audio.play();
    
    UINT bw;
    f_close(&ctx.file);
    uRes = f_open(&ctx.file, "0:/music/arcs.wav", FA_READ);
    uRes = f_read(&ctx.file, wm_audio_play_buffer, MW_AUDIO_BUFFER_SIZE, &bw);
    LOG(EDEBUG, "bw:%d, uRes:%d", bw, uRes);
    while(E_MW_AUDIO_TYPE_PLAY == Audio.uType) {
        if(bw != MW_AUDIO_BUFFER_SIZE) {
            LOG(EDEBUG, "play end");
            break;
        }
        if (1 == wavtransferend) {
            wavtransferend = 0;
            uRes = f_read(&ctx.file, wm_audio_play_buffer, MW_AUDIO_BUFFER_SIZE, &bw); 
            //LOG(EDEBUG, "bw:%d, uRes:%d", bw, uRes);
            //LOG(EDEBUG, "bw:%x", wm_audio_play_buffer[0]);
        } else if (2 == wavtransferend) {
            wavtransferend = 0;
            uRes = f_read(&ctx.file, wm_audio_play_buffer+MW_AUDIO_BUFFER_SIZE, MW_AUDIO_BUFFER_SIZE, &bw); 
        }
    }
    Audio.stop();
    f_close(&ctx.file);
}

void test_audio_play()
{
    while(WM8978_Init()) {
        LOG(EERROR, "wm8978 init error");
        HAL_Delay(1000);
    }
    //WM8978_ADDA_Cfg(1,1);		//开启ADC
    //WM8978_Input_Cfg(1,0,0);	//开启输入通道(MIC&LINE IN)
    //WM8978_Output_Cfg(1,0);		//开启BYPASS输出 
    //WM8978_MIC_Gain(50);		//MIC增益设置

    WM8978_ADDA_Cfg(1,0);	    // 开启DAC
    WM8978_Input_Cfg(0,0,0);    // 关闭输入通道
    WM8978_Output_Cfg(1,0);	    // 开启DAC输出
    WM8978_HPvol_Set(50,50);	// 耳机音量设置
    WM8978_SPKvol_Set(50);		// 喇叭音量设置

    static uint16_t ptxData[4096];
    static uint16_t prxData[4096];

    HAL_I2SEx_TransmitReceive_DMA(&hi2s2, (uint16_t *)ptxData, (uint16_t *)prxData, sizeof(ptxData)/sizeof(ptxData[0]));
}
