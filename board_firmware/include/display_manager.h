/**
 * @file display_manager.h
 * @brief Display management header
 */

#ifndef __DISPLAY_MANAGER_H__
#define __DISPLAY_MANAGER_H__

#include <stdint.h>
#include <time.h>
#include "tal_time_service.h"

/**
 * @brief Initialize display (must call once at startup)
 *
 * @return 0 on success, -1 on failure
 */
int display_init(void);

/**
 * @brief Show startup message on display
 */
void display_show_startup(void);

/**
 * @brief Display current time on the screen
 *
 * @param timestamp Unix timestamp to display
 */
void display_show_time(TIME_T timestamp);

/**
 * @brief Set display brightness
 *
 * @param brightness 0-100
 */
void display_set_brightness(uint8_t brightness);

/**
 * @brief Clear display (black screen)
 */
void display_clear(void);

#endif /* __DISPLAY_MANAGER_H__ */
