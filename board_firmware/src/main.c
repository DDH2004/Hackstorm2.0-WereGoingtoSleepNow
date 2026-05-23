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

#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

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
// DISPLAY UPDATE THREAD
// ==========================================

static void display_show_time(TIME_T timestamp)
{
    if (!g_disp_fb) return;

    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&timestamp, &tm);

    uint32_t color_white = 0xFFFF;
    uint32_t color_black = 0x0000;

    // Black background
    tdl_disp_draw_fill_full(g_disp_fb, color_black, g_disp_info.is_swap);

    // Draw hour tens digit area
    TDL_DISP_RECT_T rect = {50, 150, 110, 300};
    tdl_disp_draw_fill(g_disp_fb, &rect, color_white, g_disp_info.is_swap);

    // Draw hour ones digit area
    rect = (TDL_DISP_RECT_T){140, 150, 200, 300};
    tdl_disp_draw_fill(g_disp_fb, &rect, color_white, g_disp_info.is_swap);

    // Draw colon
    rect = (TDL_DISP_RECT_T){220, 190, 230, 240};
    tdl_disp_draw_fill(g_disp_fb, &rect, color_white, g_disp_info.is_swap);
    rect = (TDL_DISP_RECT_T){220, 260, 230, 310};
    tdl_disp_draw_fill(g_disp_fb, &rect, color_white, g_disp_info.is_swap);

    // Draw minute tens digit area
    rect = (TDL_DISP_RECT_T){250, 150, 310, 300};
    tdl_disp_draw_fill(g_disp_fb, &rect, color_white, g_disp_info.is_swap);

    // Draw minute ones digit area
    rect = (TDL_DISP_RECT_T){340, 150, 400, 300};
    tdl_disp_draw_fill(g_disp_fb, &rect, color_white, g_disp_info.is_swap);

    (void)tm; // used for future digit rendering

    // Flush to display
    tdl_disp_dev_flush(g_disp_hdl, g_disp_fb);

    // Swap buffers
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

    // Simple HTTP server - for now just a placeholder
    // The real implementation would handle socket connections
    // See VR Avatar's avatar_mcp.c for reference

    while (1) {
        tal_system_sleep(10000);
    }
}

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
