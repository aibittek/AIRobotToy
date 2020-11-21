#ifndef MW_AUDIO_H
#define MW_AUDIO_H

#include <stdint.h>

#define MW_AUDIO_BUFFER_SIZE    (4096)                          // 缓存大小
extern uint8_t wm_audio_record_buffer[MW_AUDIO_BUFFER_SIZE*2];  // 录音缓存
extern uint8_t wm_audio_play_buffer[MW_AUDIO_BUFFER_SIZE*2];    // 播放缓存

typedef enum E_MW_AUDIO_TYPE {
    E_MW_AUDIO_TYPE_NONE = 0,
    E_MW_AUDIO_TYPE_RECORD,
    E_MW_AUDIO_TYPE_PLAY,
}E_MW_AUDIO_TYPE_T;

typedef struct mw_audio_config {
    uint32_t    uSampleRate;                                // 采样率
    uint8_t     uSampleBit;                                 // 采样位数
    uint8_t     uChannel;                                   // 采样通道
    uint8_t     uVolume;                                    // 音量
    uint8_t     (*callback)(void *pvData, int32_t iLen);    // 音频回调
    void        *userdata;                                  // 回调私有变量
}mw_audio_config_t;

typedef struct mw_audio {
    uint8_t uType;              // 0: NONE, 1:record, 2:play
    mw_audio_config_t audio_config;
    
    uint8_t (*begin)(void);     // 打开音频设备
    uint8_t (*end)(void);       // 关闭音频设备
    uint8_t (*reset)(void);     // 重启设备
    
    uint8_t (*config)(mw_audio_config_t *pstConfig);    // 录音播放参数
    uint8_t (*record)();                    // 开始录音
    uint8_t (*play)();                      // 开始播放
    uint8_t (*wavplay)(const char *path);   // 播放wav
    void    (*stop)();                      // 停止
}mw_audio_t;
extern mw_audio_t Audio;

#endif

