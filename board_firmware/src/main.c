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
static volatile int  g_alarm_hour   = -1;
static volatile int  g_alarm_minute = -1;
static volatile bool g_alarm_active = false;

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

#define SEG_W  8    // segment bar thickness (px)
#define DIG_W  48   // digit cell width  (px)
#define DIG_H  80   // digit cell height (px)

// bit0=A(top) bit1=B(top-right) bit2=C(bot-right) bit3=D(bottom)
// bit4=E(bot-left) bit5=F(top-left) bit6=G(middle)
static const uint8_t SEG_MAP[10] = {
    0b0111111, // 0: A B C D E F
    0b0000110, // 1: B C
    0b1011011, // 2: A B D E G
    0b1001111, // 3: A B C D G
    0b1100110, // 4: B C F G
    0b1101101, // 5: A C D F G
    0b1111101, // 6: A C D E F G
    0b0000111, // 7: A B C
    0b1111111, // 8: all
    0b1101111, // 9: A B C D F G
};

static void draw_digit(int x, int y, int digit, uint32_t color)
{
    if (digit < 0 || digit > 9) return;
    uint8_t s   = SEG_MAP[digit];
    int     mid = DIG_H / 2;

    // A: top horizontal
    if (s & 0b0000001)
        tdl_disp_draw_fill_rect(g_disp_fb, x,            y,             DIG_W, SEG_W, color, g_disp_info.is_swap);
    // B: top-right vertical
    if (s & 0b0000010)
        tdl_disp_draw_fill_rect(g_disp_fb, x + DIG_W - SEG_W, y,       SEG_W, mid,   color, g_disp_info.is_swap);
    // C: bottom-right vertical
    if (s & 0b0000100)
        tdl_disp_draw_fill_rect(g_disp_fb, x + DIG_W - SEG_W, y + mid, SEG_W, mid,   color, g_disp_info.is_swap);
    // D: bottom horizontal
    if (s & 0b0001000)
        tdl_disp_draw_fill_rect(g_disp_fb, x,            y + DIG_H - SEG_W, DIG_W, SEG_W, color, g_disp_info.is_swap);
    // E: bottom-left vertical
    if (s & 0b0010000)
        tdl_disp_draw_fill_rect(g_disp_fb, x,            y + mid,     SEG_W, mid,   color, g_disp_info.is_swap);
    // F: top-left vertical
    if (s & 0b0100000)
        tdl_disp_draw_fill_rect(g_disp_fb, x,            y,           SEG_W, mid,   color, g_disp_info.is_swap);
    // G: middle horizontal
    if (s & 0b1000000)
        tdl_disp_draw_fill_rect(g_disp_fb, x,            y + mid - SEG_W/2, DIG_W, SEG_W, color, g_disp_info.is_swap);
}

static void draw_colon(int x, int y, uint32_t color)
{
    int dot = SEG_W + 2;
    int mid = DIG_H / 2;
    tdl_disp_draw_fill_rect(g_disp_fb, x, y + mid/2,       dot, dot, color, g_disp_info.is_swap);
    tdl_disp_draw_fill_rect(g_disp_fb, x, y + mid + mid/2, dot, dot, color, g_disp_info.is_swap);
}

// ==========================================
// DISPLAY UPDATE THREAD
// ==========================================

static void display_show_time(TIME_T timestamp)
{
    if (!g_disp_fb) return;

    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&timestamp, &tm);

    // Clear framebuffer directly (matches vr_avatar pattern)
    memset(g_disp_fb->frame, 0x00, g_disp_fb->len);

    // Centre HH:MM on 320x480 screen
    // Total width: 4*DIG_W + 1 colon(16) + 3 gaps(8) = 192 + 16 + 24 = 232
    int gap   = 8;
    int cgap  = 16;
    int total = 4 * DIG_W + cgap + 3 * gap;
    int x     = (g_screen_width  - total) / 2;
    int y     = (g_screen_height - DIG_H) / 2;

    draw_digit(x,                         y, tm.tm_hour / 10, 0xFFFF);
    draw_digit(x + DIG_W + gap,           y, tm.tm_hour % 10, 0xFFFF);
    draw_colon(x + 2*(DIG_W+gap),         y, 0x07E0);  // green colon
    draw_digit(x + 2*(DIG_W+gap) + cgap,  y, tm.tm_min  / 10, 0xFFFF);
    draw_digit(x + 3*(DIG_W+gap) + cgap,  y, tm.tm_min  % 10, 0xFFFF);

    tdl_disp_dev_flush(g_disp_hdl, g_disp_fb);
    g_disp_fb = (g_disp_fb == g_disp_fb_1) ? g_disp_fb_2 : g_disp_fb_1;
}

static void display_update_thread(void *arg)
{
    PR_NOTICE("Display thread started");

    while (1) {
        TIME_T current_time = tal_time_get_posix();
        display_show_time(current_time);

        tal_system_sleep(1000); // Update every second
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
    g_alarm_active = false;
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
