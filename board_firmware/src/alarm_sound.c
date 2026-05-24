/**
 * alarm_sound.c
 *
 * Plays a beep through the board speaker using the low-level TKL audio
 * layer directly — the same path proven to work by cmd_test_audio.
 *
 * IMPORTANT: tkl_ai_init() MUST be called once at boot BEFORE the LCD
 * display is initialised, otherwise the display breaks. So:
 *   - alarm_sound_init()  : call once at boot, before display_init()
 *   - alarm_sound_start() : just pushes frames; never re-inits audio
 */

#include "alarm_sound.h"
#include "tal_api.h"
#include "tkl_audio.h"
#include <string.h>

#define SAMPLE_RATE       16000
#define FRAME_SAMPLES     320          // 20 ms at 16 kHz
#define FRAME_BYTES       (FRAME_SAMPLES * 2)

#define BEEP_FREQ_HZ      880          // A5
#define BEEP_ON_FRAMES    20           // 400 ms beep
#define BEEP_OFF_FRAMES   10           // 200 ms gap
#define MAX_BEEP_CYCLES   45           // ~30 s auto-stop

static THREAD_HANDLE  g_sound_thread = NULL;
static volatile bool  g_sound_stop   = false;
static bool           g_audio_inited = false;

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

void alarm_sound_init(void)
{
    if (g_audio_inited) return;

    TKL_AUDIO_CONFIG_T cfg;
    memset(&cfg, 0, sizeof(cfg));
    cfg.enable             = 1;
    cfg.card               = TKL_AUDIO_TYPE_BOARD;
    cfg.ai_chn             = TKL_AI_0;
    cfg.sample             = TKL_AUDIO_SAMPLE_16K;
    cfg.datebits           = TKL_AUDIO_DATABITS_16;
    cfg.channel            = TKL_AUDIO_CHANNEL_MONO;
    cfg.codectype          = TKL_CODEC_AUDIO_PCM;
    cfg.spk_sample         = TKL_AUDIO_SAMPLE_16K;
    cfg.spk_gpio           = TUYA_GPIO_NUM_28;
    cfg.spk_gpio_polarity  = 0;
    cfg.spk_volume         = 80;

    OPERATE_RET rt = tkl_ai_init(&cfg, 0);
    if (rt != OPRT_OK) { PR_ERR("[alarm] tkl_ai_init failed: %d", rt); return; }

    rt = tkl_ai_start(TKL_AUDIO_TYPE_BOARD, TKL_AI_0);
    if (rt != OPRT_OK) { PR_ERR("[alarm] tkl_ai_start failed: %d", rt); return; }

    tkl_ai_set_vol(TKL_AUDIO_TYPE_BOARD, TKL_AI_0, 80);
    tkl_ao_set_vol(TKL_AUDIO_TYPE_BOARD, TKL_AO_0, NULL, 80);

    build_tone();
    g_audio_inited = true;
    PR_NOTICE("[alarm] ============================================");
    PR_NOTICE("[alarm] AUDIO INIT OK — speaker ready");
    PR_NOTICE("[alarm] ============================================");

    // Play a single 200 ms confirmation chirp so we can hear that init worked.
    TKL_AUDIO_FRAME_INFO_T f = { .pbuf = (char *)g_tone, .used_size = FRAME_BYTES };
    for (int i = 0; i < 10; i++) tkl_ao_put_frame(0, 0, NULL, &f);
}

static void alarm_sound_thread(void *arg)
{
    if (!g_audio_inited) {
        PR_ERR("[alarm] audio not initialised — call alarm_sound_init() at boot!");
        g_sound_thread = NULL;
        return;
    }

    TKL_AUDIO_FRAME_INFO_T tone_frame    = { .pbuf = (char *)g_tone,    .used_size = FRAME_BYTES };
    TKL_AUDIO_FRAME_INFO_T silence_frame = { .pbuf = (char *)g_silence, .used_size = FRAME_BYTES };

    int cycles = 0;
    while (!g_sound_stop && cycles < MAX_BEEP_CYCLES) {
        for (int i = 0; i < BEEP_ON_FRAMES && !g_sound_stop; i++)
            tkl_ao_put_frame(0, 0, NULL, &tone_frame);
        for (int i = 0; i < BEEP_OFF_FRAMES && !g_sound_stop; i++)
            tkl_ao_put_frame(0, 0, NULL, &silence_frame);
        cycles++;
    }

    PR_NOTICE("[alarm] sound thread exiting");
    g_sound_thread = NULL;
}

void alarm_sound_start(void)
{
    if (g_sound_thread) return;
    g_sound_stop = false;
    // PRIO_4 so we don't starve the CLI / display threads while pushing frames.
    THREAD_CFG_T cfg = {.stackDepth = 4096, .priority = THREAD_PRIO_4, .thrdname = "alarm_snd"};
    tal_thread_create_and_start(&g_sound_thread, NULL, NULL, alarm_sound_thread, NULL, &cfg);
    PR_NOTICE("[alarm] sound started");
}

void alarm_sound_stop(void)
{
    g_sound_stop = true;
    PR_NOTICE("[alarm] sound stop requested");
}
