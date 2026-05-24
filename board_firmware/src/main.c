/**
 * @file main.c
 * @brief HackStorm Smart Alarm Clock - Simplified from VR Avatar
 *
 * Features:
 * - Display current time on 320x480 LCD
 * - WiFi connectivity with automatic connection
 * - NTP time synchronization
 * - HTTP API server on port 8080
 * - mDNS service advertisement
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"
#include <string.h>
#include <stdio.h>

// ==========================================
// CONFIGURATION
// ==========================================

#define WIFI_SSID     "Shyam's iPhone"
#define WIFI_PASSWORD "your_hotspot_password"

#ifndef DISPLAY_NAME
#define DISPLAY_NAME "lcd_disp"
#endif
#define HTTP_SERVER_PORT 8080

// ==========================================
// GLOBALS
// ==========================================

static TDL_DISP_HANDLE_T   g_disp_hdl = NULL;
static TDL_DISP_DEV_INFO_T g_disp_info;
static TDL_DISP_FRAME_BUFF_T *g_disp_fb_1 = NULL;
static TDL_DISP_FRAME_BUFF_T *g_disp_fb_2 = NULL;
static TDL_DISP_FRAME_BUFF_T *g_disp_fb = NULL;

static uint16_t g_screen_width = 0;
static uint16_t g_screen_height = 0;
static THREAD_HANDLE g_display_thread = NULL;
static THREAD_HANDLE g_http_thread = NULL;

static volatile bool g_wifi_connected = false;

// Alarm state set via serial bridge
static volatile int  g_alarm_hour    = -1;
static volatile int  g_alarm_minute  = -1;
static volatile bool g_alarm_active  = false;
static volatile bool g_alarm_ringing = false;


// ==========================================
// DISPLAY INITIALIZATION
// ==========================================

static int display_init(void)
{
    OPERATE_RET rt = OPRT_OK;
    PR_NOTICE("Initializing Display...");

    board_register_hardware();

    g_disp_hdl = tdl_disp_find_dev(DISPLAY_NAME);
    if (NULL == g_disp_hdl) {
        PR_ERR("Display device not found");
        return -1;
    }

    rt = tdl_disp_dev_get_info(g_disp_hdl, &g_disp_info);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to get display info");
        return -1;
    }

    rt = tdl_disp_dev_open(g_disp_hdl);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to open display");
        return -1;
    }

    g_screen_width = g_disp_info.width;
    g_screen_height = g_disp_info.height;
    tdl_disp_set_brightness(g_disp_hdl, 100);

    PR_NOTICE("Display: %dx%d", g_screen_width, g_screen_height);

    // Create frame buffers
    uint32_t frame_len = g_screen_width * g_screen_height * 2;

    g_disp_fb_1 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (g_disp_fb_1) {
        g_disp_fb_1->fmt = g_disp_info.fmt;
        g_disp_fb_1->width = g_screen_width;
        g_disp_fb_1->height = g_screen_height;
    }

    g_disp_fb_2 = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (g_disp_fb_2) {
        g_disp_fb_2->fmt = g_disp_info.fmt;
        g_disp_fb_2->width = g_screen_width;
        g_disp_fb_2->height = g_screen_height;
    }

    g_disp_fb = g_disp_fb_1;

    PR_NOTICE("Display initialized");
    return 0;
}

// ==========================================
// 7-SEGMENT DIGIT RENDERING
// Matches proven pattern from vr_avatar_main.c:
//   tdl_disp_draw_fill_rect(fb, x, y, w, h, color, swap)  ← (x,y,width,height)
//   memset(fb->frame, 0, fb->len)                          ← clear screen
// ==========================================

// ==========================================
// 7-SEGMENT RENDERING — LANDSCAPE (90° CCW)
//
// Physical display is 320×480 portrait.
// We draw in logical 480×320 landscape coordinates.
// Transform: landscape(lx,ly) → portrait(319-ly, lx)
// No framebuffer rotation needed — just flush the single buffer.
// ==========================================

#define PORT_H 480   // physical portrait height

// Fill a landscape rect given landscape corners (lx0,ly0)→(lx1,ly1)
// 90° CW: landscape(lx,ly) → portrait(ly, PORT_H-1-lx)
#define FILL_L(lx0, ly0, lx1, ly1, col) do { \
    TDL_DISP_RECT_T _r = {(ly0), PORT_H-1-(lx1), (ly1), PORT_H-1-(lx0)}; \
    tdl_disp_draw_fill(g_disp_fb, &_r, (col), g_disp_info.is_swap); \
} while(0)

#define SEG_W  6    // segment thickness (landscape px)
#define DIG_W  44   // digit width  (landscape px)
#define DIG_H  70   // digit height (landscape px)

// bit0=A(top) bit1=B(top-right) bit2=C(bot-right) bit3=D(bottom)
// bit4=E(bot-left) bit5=F(top-left) bit6=G(middle)
static const uint8_t SEG_MAP[10] = {
    0x3F, // 0
    0x06, // 1
    0x5B, // 2
    0x4F, // 3
    0x66, // 4
    0x6D, // 5
    0x7D, // 6
    0x07, // 7
    0x7F, // 8
    0x6F, // 9
};

static void draw_digit(int lx, int ly, int digit, uint32_t color)
{
    if (digit < 0 || digit > 9) return;
    uint8_t s   = SEG_MAP[digit];
    int     mid = DIG_H / 2;

    if (s & 0x01) FILL_L(lx,            ly,            lx+DIG_W,       ly+SEG_W,        color); // A top
    if (s & 0x02) FILL_L(lx+DIG_W-SEG_W,ly,            lx+DIG_W,       ly+mid,          color); // B top-right
    if (s & 0x04) FILL_L(lx+DIG_W-SEG_W,ly+mid,        lx+DIG_W,       ly+DIG_H,        color); // C bot-right
    if (s & 0x08) FILL_L(lx,            ly+DIG_H-SEG_W,lx+DIG_W,       ly+DIG_H,        color); // D bottom
    if (s & 0x10) FILL_L(lx,            ly+mid,         lx+SEG_W,       ly+DIG_H,        color); // E bot-left
    if (s & 0x20) FILL_L(lx,            ly,             lx+SEG_W,       ly+mid,          color); // F top-left
    if (s & 0x40) FILL_L(lx,            ly+mid-SEG_W/2, lx+DIG_W,       ly+mid+SEG_W/2,  color); // G middle
}

static void draw_colon(int lx, int ly, uint32_t color)
{
    int dot = SEG_W + 2;
    int mid = DIG_H / 2;
    FILL_L(lx, ly+mid/2,       lx+dot, ly+mid/2+dot,     color);
    FILL_L(lx, ly+mid+mid/2,   lx+dot, ly+mid+mid/2+dot, color);
}

// ==========================================
// DISPLAY UPDATE THREAD
// ==========================================

static void display_show_time(TIME_T timestamp, uint32_t digit_color)
{
    if (!g_disp_fb) return;

    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&timestamp, &tm);

    memset(g_disp_fb->frame, 0x00, g_disp_fb->len);

    // Landscape 480×320 — HH:MM:SS centred
    // total = 6*DIG_W + 5*gap + 2*cgap = 322
    int gap   = 6;
    int cgap  = 14;
    int total = 6 * DIG_W + 5 * gap + 2 * cgap;
    int lx    = (480 - total) / 2;   // ~79
    int ly    = (320 - DIG_H) / 2;   // 125

    draw_digit(lx,                             ly, tm.tm_hour / 10, digit_color);
    draw_digit(lx +   DIG_W +   gap,           ly, tm.tm_hour % 10, digit_color);
    draw_colon(lx + 2*(DIG_W +  gap),          ly, 0x07E0);
    draw_digit(lx + 2*(DIG_W +  gap) +  cgap,  ly, tm.tm_min  / 10, digit_color);
    draw_digit(lx + 3*(DIG_W +  gap) +  cgap,  ly, tm.tm_min  % 10, digit_color);
    draw_colon(lx + 4*(DIG_W +  gap) +  cgap,  ly, 0x07E0);
    draw_digit(lx + 4*(DIG_W +  gap) + 2*cgap, ly, tm.tm_sec  / 10, digit_color);
    draw_digit(lx + 5*(DIG_W +  gap) + 2*cgap, ly, tm.tm_sec  % 10, digit_color);

    tdl_disp_dev_flush(g_disp_hdl, g_disp_fb);
    g_disp_fb = (g_disp_fb == g_disp_fb_1) ? g_disp_fb_2 : g_disp_fb_1;
}

static void display_update_thread(void *arg)
{
    PR_NOTICE("Display thread started");
    static int flash_toggle = 0;

    while (1) {
        TIME_T current_time = tal_time_get_posix();
        POSIX_TM_S tm = {0};
        tal_time_gmtime_r(&current_time, &tm);

        // Trigger alarm at exactly HH:MM:00
        if (g_alarm_active && !g_alarm_ringing &&
            tm.tm_hour == g_alarm_hour && tm.tm_min == g_alarm_minute && tm.tm_sec == 0) {
            g_alarm_ringing = true;
            PR_NOTICE("ALARM TRIGGERED: %02d:%02d", g_alarm_hour, g_alarm_minute);
        }

        // Flash red/white while ringing; white otherwise
        uint32_t color = 0xFFFF;
        if (g_alarm_ringing) {
            flash_toggle ^= 1;
            color = flash_toggle ? 0xF800 : 0xFFFF; // red 565 / white 565
        }

        display_show_time(current_time, color);
        tal_system_sleep(1000);
    }
}

// ==========================================
// HTTP SERVER
// ==========================================

static void http_server_thread(void *arg)
{
    PR_NOTICE("HTTP Server thread started (port %d)", HTTP_SERVER_PORT);

    // Placeholder — communication handled via serial bridge (see serial_bridge.py)
    while (1) {
        tal_system_sleep(10000);
    }
}

// ==========================================
// SERIAL BRIDGE COMMAND HANDLERS
// Registered with tal_cli so the Mac-side
// serial_bridge.py can drive the board.
// ==========================================

// cmd: set_time <unix_ts>  →  TIME_SET
// Bridge calls this on startup with the Mac's current UTC epoch time.
// We store the delta between real time and what the chip thinks it is.
static void cmd_set_time(int argc, char *argv[])
{
    if (argc < 2) { tal_cli_echo("ERR: usage: set_time <unix_ts>\r\n"); return; }
    TIME_T provided = (TIME_T)atol(argv[1]);
    tal_time_set_posix(provided, 2);
    PR_NOTICE("Time set via CLI: %lu", (unsigned long)provided);
    tal_cli_echo("TIME_SET\r\n");
}

// cmd: set_alarm <HH> <MM>  →  ALARM_SET:<HH>:<MM>
static void cmd_set_alarm(int argc, char *argv[])
{
    char buf[32];
    if (argc < 3) {
        tal_cli_echo("ERR: usage: set_alarm <HH> <MM>\r\n");
        return;
    }
    g_alarm_hour   = atoi(argv[1]);
    g_alarm_minute = atoi(argv[2]);
    g_alarm_active = true;
    PR_NOTICE("Alarm set: %02d:%02d", g_alarm_hour, g_alarm_minute);
    snprintf(buf, sizeof(buf), "ALARM_SET:%02d:%02d\r\n", g_alarm_hour, g_alarm_minute);
    tal_cli_echo(buf);
}

// cmd: dismiss_alarm  →  ALARM_DISMISSED
static void cmd_dismiss_alarm(int argc, char *argv[])
{
    g_alarm_active  = false;
    g_alarm_ringing = false;
    PR_NOTICE("Alarm dismissed");
    tal_cli_echo("ALARM_DISMISSED\r\n");
}

// cmd: get_status  →  STATUS:<alarm_h>:<alarm_m>:<active>:<cur_h>:<cur_m>
static void cmd_get_status(int argc, char *argv[])
{
    char buf[64];
    TIME_T now = tal_time_get_posix();
    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&now, &tm);
    snprintf(buf, sizeof(buf), "STATUS:%02d:%02d:%d:%02d:%02d\r\n",
             g_alarm_hour, g_alarm_minute, (int)g_alarm_active,
             tm.tm_hour, tm.tm_min);
    tal_cli_echo(buf);
}

static const cli_cmd_t g_cli_cmds[] = {
    { "set_time",     "set_time <unix_ts>", cmd_set_time     },
    { "set_alarm",    "set_alarm <HH> <MM>", cmd_set_alarm    },
    { "dismiss_alarm","dismiss_alarm",        cmd_dismiss_alarm},
    { "get_status",   "get_status",           cmd_get_status   },
};

// ==========================================
// WiFi CALLBACKS
// ==========================================

static void wifi_status_callback(netmgr_type_e type, netmgr_status_e stat)
{
    PR_NOTICE("WiFi Status: %d", stat);

    if (stat == NETMGR_LINK_UP || stat == NETMGR_LINK_UP_SWITH) {
        g_wifi_connected = true;
        PR_NOTICE("WiFi Connected!");
    } else {
        g_wifi_connected = false;
        PR_NOTICE("WiFi Disconnected");
    }
}

// ==========================================
// COMPILE-TIME CLOCK SEED
// Converts __DATE__ / __TIME__ to a POSIX timestamp so the board
// shows approximately correct UTC time without WiFi or serial input.
// ==========================================

static TIME_T get_compile_time(void)
{
    // __DATE__ format: "May 24 2026"   __TIME__ format: "10:30:45"
    static const char *mon_names = "JanFebMarAprMayJunJulAugSepOctNovDec";
    const char *d = __DATE__;
    const char *t = __TIME__;

    char mon_buf[4] = {d[0], d[1], d[2], '\0'};
    int  month = 1;
    for (int i = 0; i < 12; i++) {
        if (strncmp(mon_buf, mon_names + i * 3, 3) == 0) { month = i + 1; break; }
    }
    int day  = atoi(d + 4);
    int year = atoi(d + 7);
    int hour = atoi(t);
    int min  = atoi(t + 3);
    int sec  = atoi(t + 6);

    // Days from 1970-01-01 to compile date (Gregorian, good enough for 2020-2100)
    int y = year - 1970;
    static const int mdays[12] = {0,31,59,90,120,151,181,212,243,273,304,334};
    long days = (long)y * 365 + (y + 1) / 4 + mdays[month - 1] + (day - 1);
    if (month > 2 && (year % 4 == 0 && (year % 100 != 0 || year % 400 == 0)))
        days++;

    return (TIME_T)(days * 86400L + hour * 3600 + min * 60 + sec);
}

// ==========================================
// MAIN APPLICATION
// ==========================================

void user_main(void)
{
    OPERATE_RET rt = OPRT_OK;

    // Initialize logging
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);

    PR_NOTICE("========================================");
    PR_NOTICE("HackStorm Smart Alarm Clock v1.0");
    PR_NOTICE("========================================");

    tal_system_sleep(1000);

    // Step 1: Initialize System Services
    PR_NOTICE("[1/5] Initializing system services...");

    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "hackstorm_seed_01",
        .key = "hackstorm_key_01",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_time_service_init();
    tal_cli_init();

    tal_cli_cmd_register(g_cli_cmds, sizeof(g_cli_cmds) / sizeof(g_cli_cmds[0]));

    // Seed clock from compile time (build+flash takes ~60-90s, add that as offset)
    TIME_T compile_ts = get_compile_time() + 90;
    tal_time_set_posix(compile_ts, 2);
    PR_NOTICE("Clock seeded from compile time: %lu", (unsigned long)compile_ts);

    PR_NOTICE("System services initialized");

    // Step 2: Initialize Display
    PR_NOTICE("[2/5] Initializing display...");

    if (display_init() != 0) {
        PR_ERR("Display init failed");
        return;
    }

    // Step 3: Initialize WiFi
    PR_NOTICE("[3/5] Initializing WiFi...");

    netmgr_init(NETCONN_WIFI);
    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SET_STATUS_CB, wifi_status_callback);

    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);

    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);

    PR_NOTICE("WiFi initialized, connecting to: %s", WIFI_SSID);
    tal_system_sleep(5000);

    // Step 4: Sync Time
    PR_NOTICE("[4/5] Synchronizing time via NTP...");

    rt = tal_time_check_time_sync();
    if (rt == OPRT_OK) {
        PR_NOTICE("Time synchronized");

        TIME_T current_time = tal_time_get_posix();
        POSIX_TM_S tm = {0};
        tal_time_gmtime_r(&current_time, &tm);

        PR_NOTICE("Current time: %04d-%02d-%02d %02d:%02d:%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);
    } else {
        PR_WARN("Time sync pending...");
    }

    // Step 5: Start Display & HTTP Threads
    PR_NOTICE("[5/5] Starting application threads...");

    THREAD_CFG_T display_cfg = {4096, 3, "display_update"};
    rt = tal_thread_create_and_start(&g_display_thread, NULL, NULL,
                                     display_update_thread, NULL, &display_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create display thread");
        return;
    }

    PR_NOTICE("Display thread started");

    tal_system_sleep(2000);

    THREAD_CFG_T http_cfg = {8192, 4, "http_server"};
    rt = tal_thread_create_and_start(&g_http_thread, NULL, NULL,
                                     http_server_thread, NULL, &http_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create HTTP server thread");
        return;
    }

    PR_NOTICE("HTTP server thread started");

    // Main loop
    PR_NOTICE("========================================");
    PR_NOTICE("HackStorm Smart Alarm Clock - Ready!");
    PR_NOTICE("========================================");

    while (1) {
        if (g_wifi_connected) {
            PR_DEBUG("WiFi connected");
        } else {
            PR_DEBUG("Waiting for WiFi...");
        }

        tal_system_sleep(10000);
    }
}

// ==========================================
// OS ENTRY POINTS
// ==========================================

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
}
#else
static THREAD_HANDLE g_app_thread = NULL;

static void tuya_app_thread(void *arg)
{
    user_main();
    tal_thread_delete(g_app_thread);
    g_app_thread = NULL;
}

void tuya_app_main(void)
{
    THREAD_CFG_T thrd_param = {8192, 4, "tuya_app_main"};
    tal_thread_create_and_start(&g_app_thread, NULL, NULL,
                                tuya_app_thread, NULL, &thrd_param);
}
#endif
