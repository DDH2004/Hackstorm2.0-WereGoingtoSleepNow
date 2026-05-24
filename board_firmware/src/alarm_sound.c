/**
 * alarm_sound.c
 *
 * Plays a repeating beep through the board speaker using the TDL audio layer.
 * The codec ("audio_codec") is registered by board_register_hardware() with
 * speaker enable on GPIO 28, active-LOW — we don't need to re-specify that here.
 *
 * Flow: tdl_audio_find → tdl_audio_open → tdl_audio_play (loop) → tdl_audio_close
 */

#include "alarm_sound.h"
#include "tal_api.h"
#include "tdl_audio_manage.h"
#include <string.h>

#ifndef AUDIO_CODEC_NAME
#define AUDIO_CODEC_NAME "audio_codec"
#endif

#define SAMPLE_RATE      16000
#define FRAME_SAMPLES    320        // 20 ms at 16 kHz
#define FRAME_BYTES      (FRAME_SAMPLES * 2)

#define BEEP_FREQ_HZ     880        // A5
#define BEEP_ON_FRAMES   20         // 400 ms on
#define BEEP_OFF_FRAMES  10         // 200 ms off
#define MAX_BEEP_CYCLES  450        // ~3 min auto-stop

static THREAD_HANDLE  g_sound_thread = NULL;
static volatile bool  g_sound_stop   = false;

static int16_t g_tone[FRAME_SAMPLES];
static int16_t g_silence[FRAME_SAMPLES];

static void build_tone(void)
{
    int half = SAMPLE_RATE / (BEEP_FREQ_HZ * 2);
    if (half < 1) half = 1;
    for (int i = 0; i < FRAME_SAMPLES; i++)
        g_tone[i] = ((i / half) & 1) ? -16000 : 16000;
    memset(g_silence, 0, sizeof(g_silence));
}

static void alarm_sound_thread(void *arg)
{
    build_tone();

    TDL_AUDIO_HANDLE_T hdl = NULL;
    OPERATE_RET rt = tdl_audio_find(AUDIO_CODEC_NAME, &hdl);
    if (rt != OPRT_OK || hdl == NULL) {
        PR_ERR("[alarm] tdl_audio_find failed: %d", rt);
        g_sound_thread = NULL;
        return;
    }

    rt = tdl_audio_open(hdl, NULL);
    if (rt != OPRT_OK) {
        PR_ERR("[alarm] tdl_audio_open failed: %d", rt);
        g_sound_thread = NULL;
        return;
    }

    tdl_audio_volume_set(hdl, 80);

    int cycles = 0;
    while (!g_sound_stop && cycles < MAX_BEEP_CYCLES) {
        for (int i = 0; i < BEEP_ON_FRAMES && !g_sound_stop; i++)
            tdl_audio_play(hdl, (uint8_t *)g_tone, FRAME_BYTES);
        for (int i = 0; i < BEEP_OFF_FRAMES && !g_sound_stop; i++)
            tdl_audio_play(hdl, (uint8_t *)g_silence, FRAME_BYTES);
        cycles++;
    }

    tdl_audio_play_stop(hdl);
    tdl_audio_close(hdl);
    g_sound_thread = NULL;
}

void alarm_sound_start(void)
{
    if (g_sound_thread) return;
    g_sound_stop = false;
    THREAD_CFG_T cfg = {.stackDepth = 4096, .priority = THREAD_PRIO_2, .thrdname = "alarm_snd"};
    tal_thread_create_and_start(&g_sound_thread, NULL, NULL, alarm_sound_thread, NULL, &cfg);
    PR_NOTICE("[alarm] sound started");
}

void alarm_sound_stop(void)
{
    g_sound_stop = true;
    PR_NOTICE("[alarm] sound stop requested");
}
