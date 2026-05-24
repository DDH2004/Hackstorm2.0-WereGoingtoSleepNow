/**
 * alarm_sound.c — Plays a repeating beep tone through the T5AI board speaker
 * using the tkl_ao audio output API.
 *
 * The board speaker enable pin is TUYA_GPIO_NUM_28, active-LOW.
 * Audio is PCM 16kHz mono 16-bit, matching the registered codec config.
 */

#include "alarm_sound.h"
#include "tal_api.h"
#include "tkl_audio.h"
#include <string.h>

#define SAMPLE_RATE    16000
#define FRAME_SAMPLES  640          // 40 ms per frame at 16 kHz
#define FRAME_BYTES    (FRAME_SAMPLES * 2)

#define BEEP_FREQ_HZ   880          // A5 — piercing but not harsh
#define BEEP_ON_FRAMES 10           // 400 ms on
#define BEEP_OFF_FRAMES 5           // 200 ms off

#define SPEAKER_GPIO      28        // TUYA_GPIO_NUM_28
#define SPEAKER_POLARITY  1         // 1 = low-enable (active LOW)

static THREAD_HANDLE  g_sound_thread = NULL;
static volatile bool  g_sound_stop   = false;

static int16_t g_pcm_tone[FRAME_SAMPLES];
static int16_t g_pcm_silence[FRAME_SAMPLES];

static void build_square_wave(void)
{
    int half = SAMPLE_RATE / (BEEP_FREQ_HZ * 2);
    if (half < 1) half = 1;
    for (int i = 0; i < FRAME_SAMPLES; i++)
        g_pcm_tone[i] = ((i / half) & 1) ? -16000 : 16000;
    memset(g_pcm_silence, 0, sizeof(g_pcm_silence));
}

static void push_frame(int16_t *samples)
{
    TKL_AUDIO_FRAME_INFO_T frame;
    memset(&frame, 0, sizeof(frame));
    frame.pbuf      = (char *)samples;
    frame.used_size = FRAME_BYTES;
    tkl_ao_put_frame(0, 0, NULL, &frame);
}

static void alarm_sound_thread(void *arg)
{
    build_square_wave();

    TKL_AUDIO_CONFIG_T cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enable             = 1;
    cfg.card               = TKL_AUDIO_TYPE_BOARD;
    cfg.ai_chn             = TKL_AI_0;
    cfg.sample             = TKL_AUDIO_SAMPLE_16K;
    cfg.datebits           = TKL_AUDIO_DATABITS_16;
    cfg.channel            = TKL_AUDIO_CHANNEL_MONO;
    cfg.codectype          = TKL_CODEC_AUDIO_PCM;
    cfg.fps                = 25;
    cfg.mic_volume         = 0x2d;
    cfg.spk_volume         = 80;
    cfg.spk_gpio           = SPEAKER_GPIO;
    cfg.spk_gpio_polarity  = SPEAKER_POLARITY;
    cfg.spk_sample         = TKL_AUDIO_SAMPLE_16K;

    OPERATE_RET rt = tkl_ai_init(&cfg, 1);
    if (rt != OPRT_OK) {
        PR_ERR("[alarm] tkl_ai_init failed: %d", rt);
        return;
    }
    tkl_ai_start(TKL_AUDIO_TYPE_BOARD, TKL_AI_0);
    tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, TKL_AO_0, NULL, 80);

    // Auto-stop after 3 minutes if dismiss never arrives
    int max_cycles = (3 * 60 * 1000) / (BEEP_ON_FRAMES + BEEP_OFF_FRAMES) / 40 + 1;
    int cycles = 0;
    while (!g_sound_stop && cycles < max_cycles) {
        for (int i = 0; i < BEEP_ON_FRAMES && !g_sound_stop; i++)
            push_frame(g_pcm_tone);
        for (int i = 0; i < BEEP_OFF_FRAMES && !g_sound_stop; i++)
            push_frame(g_pcm_silence);
        cycles++;
    }

    tkl_ao_stop(0, TKL_AO_0, NULL);
    tkl_ai_stop(TKL_AUDIO_TYPE_BOARD, TKL_AI_0);
    tkl_ai_uninit();
    g_sound_thread = NULL;
}

void alarm_sound_start(void)
{
    if (g_sound_thread) return;
    g_sound_stop = false;
    THREAD_CFG_T cfg = {.stackDepth = 4096, .priority = THREAD_PRIO_2, .thrdname = "alarm_snd"};
    tal_thread_create_and_start(&g_sound_thread, NULL, NULL, alarm_sound_thread, NULL, &cfg);
}

void alarm_sound_stop(void)
{
    g_sound_stop = true;
}
