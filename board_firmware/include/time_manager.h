/**
 * @file time_manager.h
 * @brief Time management header
 */

#ifndef __TIME_MANAGER_H__
#define __TIME_MANAGER_H__

#include <stddef.h>

/**
 * @brief Synchronize time via NTP
 *
 * @return 0 on success, -1 on failure
 */
int time_sync_ntp(void);

/**
 * @brief Get formatted time string "HH:MM:SS"
 *
 * @param buffer Output buffer
 * @param buffer_len Buffer size (minimum 9 bytes)
 */
void time_get_string(char *buffer, size_t buffer_len);

/**
 * @brief Get formatted date string "YYYY-MM-DD"
 *
 * @param buffer Output buffer
 * @param buffer_len Buffer size (minimum 11 bytes)
 */
void time_get_date_string(char *buffer, size_t buffer_len);

#endif /* __TIME_MANAGER_H__ */
