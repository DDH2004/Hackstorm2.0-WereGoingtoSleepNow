/**
 * @file main.c
 * @brief HackStorm Smart Alarm Clock - Minimal Board Application
 *
 * Features:
 * - Displays current time on T5 display
 * - HTTP server on port 8080 for app communication
 * - WiFi connectivity with mDNS discovery
 * - NTP time synchronization
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "netmgr.h"
#include "netconn_wifi.h"
#include "board_com_api.h"

#include "display_manager.h"
#include "http_server.h"
#include "time_manager.h"

// ==========================================
// CONFIGURATION
// ==========================================

// WiFi credentials - UPDATE THESE FOR YOUR NETWORK
#define WIFI_SSID     "JJ Lake"
#define WIFI_PASSWORD "20220315"

// Device info
#define DEVICE_ID "t5_clock_001"
#define DEVICE_NAME "HackStorm Clock"

// ==========================================
// GLOBALS
// ==========================================

static THREAD_HANDLE g_display_thread = NULL;
static THREAD_HANDLE g_http_thread = NULL;

static volatile bool g_wifi_connected = false;

// ==========================================
// CALLBACK FUNCTIONS
// ==========================================

/**
 * @brief WiFi status callback
 */
static void wifi_status_callback(GW_WIFI_STAT_E stat)
{
    PR_NOTICE("WiFi Status: %d", stat);

    // Connected to WiFi (status >= 4 means connected)
    if (stat >= 4) {
        g_wifi_connected = true;
        PR_NOTICE("WiFi Connected! Starting services...");
    } else {
        g_wifi_connected = false;
        PR_NOTICE("WiFi Disconnected");
    }
}

// ==========================================
// DISPLAY UPDATE THREAD
// ==========================================

/**
 * @brief Thread that updates display every second with current time
 */
static void display_update_thread(void *arg)
{
    PR_NOTICE("Display Update Thread Started");

    while (1) {
        // Get current time
        TIME_T current_time = tal_time_get_posix();

        // Update display
        display_show_time(current_time);

        // Update every second
        tal_system_sleep(1000);
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
    PR_NOTICE("HackStorm Smart Alarm Clock - Starting");
    PR_NOTICE("Device ID: %s", DEVICE_ID);
    PR_NOTICE("========================================");

    // Give system a moment to breathe
    tal_system_sleep(1000);

    // ==========================================
    // Step 1: Initialize Core System Services
    // ==========================================
    PR_NOTICE("[1/5] Initializing system services...");

    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "hackstorm_seed_12345",
        .key  = "hackstorm_key_67890",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_time_service_init();
    tal_cli_init();

    PR_NOTICE("System services initialized");

    // ==========================================
    // Step 2: Initialize Display
    // ==========================================
    PR_NOTICE("[2/5] Initializing display...");

    if (display_init() != 0) {
        PR_ERR("Display initialization failed!");
        return;
    }

    // Show startup message
    display_show_startup();

    PR_NOTICE("Display initialized (320x480)");

    // ==========================================
    // Step 3: Initialize WiFi & Time
    // ==========================================
    PR_NOTICE("[3/5] Initializing WiFi...");

    // Initialize network manager
    netmgr_init(NETCONN_WIFI);

    // Register WiFi callback
    netmgr_register_conn_cb(NETCONN_WIFI, wifi_status_callback);

    // Set WiFi credentials
    netconn_wifi_info_t wifi_info = {0};
    strcpy(wifi_info.ssid, WIFI_SSID);
    strcpy(wifi_info.pswd, WIFI_PASSWORD);

    netmgr_conn_set(NETCONN_WIFI, NETCONN_CMD_SSID_PSWD, &wifi_info);

    PR_NOTICE("WiFi initialized, connecting to: %s", WIFI_SSID);

    // Give WiFi some time to connect
    tal_system_sleep(5000);

    // ==========================================
    // Step 4: Sync Time
    // ==========================================
    PR_NOTICE("[4/5] Synchronizing time via NTP...");

    if (time_sync_ntp() != 0) {
        PR_WARN("NTP sync failed, using local time");
    } else {
        PR_NOTICE("Time synchronized");
    }

    // Display current time
    TIME_T current_time = tal_time_get_posix();
    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&current_time, &tm);

    PR_NOTICE("Current time: %04d-%02d-%02d %02d:%02d:%02d",
              tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
              tm.tm_hour, tm.tm_min, tm.tm_sec);

    // ==========================================
    // Step 5: Start Display & HTTP Threads
    // ==========================================
    PR_NOTICE("[5/5] Starting application threads...");

    // Start display update thread (updates every second)
    THREAD_CFG_T display_cfg = {4096, 3, "display_update"};
    rt = tal_thread_create_and_start(&g_display_thread, NULL, NULL,
                                     display_update_thread, NULL, &display_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create display thread");
        return;
    }

    PR_NOTICE("Display thread started");

    // Give WiFi a bit more time to fully connect before starting HTTP server
    tal_system_sleep(2000);

    // Start HTTP server (handles API requests from app)
    THREAD_CFG_T http_cfg = {8192, 4, "http_server"};
    rt = tal_thread_create_and_start(&g_http_thread, NULL, NULL,
                                     http_server_thread, NULL, &http_cfg);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to create HTTP server thread");
        return;
    }

    PR_NOTICE("HTTP server thread started (port 8080)");

    // ==========================================
    // Main Loop - Just keep system alive
    // ==========================================
    PR_NOTICE("========================================");
    PR_NOTICE("HackStorm Smart Alarm Clock - Ready!");
    PR_NOTICE("WiFi SSID: %s", WIFI_SSID);
    PR_NOTICE("HTTP Server: http://<board_ip>:8080");
    PR_NOTICE("========================================");

    while (1) {
        // Print WiFi status periodically
        if (g_wifi_connected) {
            PR_DEBUG("WiFi Connected");
        } else {
            PR_DEBUG("WiFi Disconnected - trying to reconnect...");
        }

        tal_system_sleep(10000); // Check every 10 seconds
    }
}

// ==========================================
// OS-Specific Entry Points
// ==========================================

#if OPERATING_SYSTEM == SYSTEM_LINUX
void main(int argc, char *argv[])
{
    user_main();
}
#else
// ESP-IDF / embedded systems
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
