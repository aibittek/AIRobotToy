#include "main.h"

void factory_test()
{
    // test sdcard
    extern void test_sdcard();
    //test_sdcard();

    // test audio
    extern void wm_audio_record_test();
    wm_audio_record_test();
    extern void wm_audio_play_test();
    wm_audio_play_test();
    extern void wm_audio_wavplay_test();
    while(1) {
        //wm_audio_wavplay_test();
    }

    // test wifi
    extern void test_net(void);
    //test_net();
}