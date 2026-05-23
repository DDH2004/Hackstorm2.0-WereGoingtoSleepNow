/**
 * @file time_manager.c
 * @brief Time synchronization via NTP
 */

#include "tal_api.h"
#include "tkl_output.h"

#include "time_manager.h"

// ==========================================
// TIME SYNCHRONIZATION
// ==========================================

/**
 * @brief Synchronize time via NTP
 *
 * This connects to a public NTP server and synchronizes the board's clock
 */
int time_sync_ntp(void)
{
    OPERATE_RET rt = OPRT_OK;

    PR_NOTICE("Starting NTP time synchronization...");

    // TuyaOpen handles NTP automatically when WiFi connects
    // This function is a placeholder for manual sync if needed

    // Check if time is already synchronized
    rt = tal_time_check_time_sync();
    if (rt == OPRT_OK) {
        PR_NOTICE("Time is synchronized");

        // Get current time and display
        TIME_T current_time = tal_time_get_posix();
        POSIX_TM_S tm = {0};
        tal_time_gmtime_r(&current_time, &tm);

        PR_NOTICE("Current UTC time: %04d-%02d-%02d %02d:%02d:%02d",
                  tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
                  tm.tm_hour, tm.tm_min, tm.tm_sec);

        return 0;
    } else {
        PR_WARN("Time sync failed, waiting for WiFi to sync...");
        return -1;
    }
}

/**
 * @brief Get formatted time string
 */
void time_get_string(char *buffer, size_t buffer_len)
{
    if (!buffer || buffer_len < 9) {
        return;
    }

    TIME_T current_time = tal_time_get_posix();
    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&current_time, &tm);

    snprintf(buffer, buffer_len, "%02d:%02d:%02d",
             tm.tm_hour, tm.tm_min, tm.tm_sec);
}

/**
 * @brief Get formatted date string
 */
void time_get_date_string(char *buffer, size_t buffer_len)
{
    if (!buffer || buffer_len < 11) {
        return;
    }

    TIME_T current_time = tal_time_get_posix();
    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&current_time, &tm);

    snprintf(buffer, buffer_len, "%04d-%02d-%02d",
             tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
}
