/**
 * @file display_manager.c
 * @brief Display management - show time on T5 LCD
 */

#include "tal_api.h"
#include "tkl_output.h"
#include "tdl_display_manage.h"
#include "tdl_display_draw.h"
#include "board_com_api.h"

#include "display_manager.h"

// ==========================================
// GLOBALS
// ==========================================

static TDL_DISP_HANDLE_T   g_disp_hdl = NULL;
static TDL_DISP_DEV_INFO_T g_disp_info;
static TDL_DISP_FRAME_BUFF_T *g_frame_buff = NULL;

// ==========================================
// IMPLEMENTATION
// ==========================================

/**
 * @brief Initialize display (called once at startup)
 */
int display_init(void)
{
    OPERATE_RET rt = OPRT_OK;

    // Register hardware (required before using display)
    board_register_hardware();

    // Find display device by name
    g_disp_hdl = tdl_disp_find_dev("lcd_disp");
    if (NULL == g_disp_hdl) {
        PR_ERR("Display device not found");
        return -1;
    }

    // Get display information (resolution, format, etc.)
    rt = tdl_disp_dev_get_info(g_disp_hdl, &g_disp_info);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to get display info: %d", rt);
        return -1;
    }

    // Open display
    rt = tdl_disp_dev_open(g_disp_hdl);
    if (rt != OPRT_OK) {
        PR_ERR("Failed to open display: %d", rt);
        return -1;
    }

    PR_NOTICE("Display opened: %dx%d, fmt=%d",
              g_disp_info.width, g_disp_info.height, g_disp_info.fmt);

    // Set brightness (0-100)
    tdl_disp_set_brightness(g_disp_hdl, 100);

    // Create frame buffer for rendering
    uint32_t frame_size = g_disp_info.width * g_disp_info.height * 2; // 2 bytes per pixel (RGB565)

    g_frame_buff = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_size);
    if (!g_frame_buff) {
        PR_ERR("Failed to create frame buffer");
        return -1;
    }

    g_frame_buff->fmt = g_disp_info.fmt;
    g_frame_buff->width = g_disp_info.width;
    g_frame_buff->height = g_disp_info.height;

    PR_NOTICE("Frame buffer created: %u bytes", frame_size);

    return 0;
}

/**
 * @brief Show startup message
 */
void display_show_startup(void)
{
    if (!g_frame_buff) return;

    // Clear frame buffer (black background)
    memset(g_frame_buff->frame, 0x00, g_frame_buff->len);

    // Draw simple startup message
    // For now, just a black screen (we'll add text later)
    // Real implementation would use a font library to render text

    // Flush to display
    tdl_disp_dev_flush(g_disp_hdl, g_frame_buff);
}

/**
 * @brief Display current time on the screen
 *
 * This draws the time in large digits on the center of the screen
 */
void display_show_time(TIME_T timestamp)
{
    if (!g_frame_buff) return;

    // Convert timestamp to human-readable time
    POSIX_TM_S tm = {0};
    tal_time_gmtime_r(&timestamp, &tm);

    int hour = tm.tm_hour;
    int minute = tm.tm_min;
    int second = tm.tm_sec;

    // Clear frame buffer (black background)
    memset(g_frame_buff->frame, 0x00, g_frame_buff->len);

    // Draw time string: "HH:MM:SS"
    // For minimal implementation, we'll just draw rectangles to represent digits
    // A real implementation would use a font library (like FreeType or built-in fonts)

    // Center position
    int center_x = g_disp_info.width / 2;
    int center_y = g_disp_info.height / 2;

    // Color: white (RGB565 = 0xFFFF)
    uint32_t color_white = 0xFFFF;
    uint32_t color_dim = 0x8408; // dim white for date

    // Simple approach: draw rectangles for each digit
    // Position for first digit (hours tens place)
    int x_start = center_x - 140;
    int y_start = center_y - 50;

    // Draw hour digits (simplified - just colored rectangles)
    int h_tens = hour / 10;
    int h_ones = hour % 10;
    int m_tens = minute / 10;
    int m_ones = minute % 10;
    int s_tens = second / 10;
    int s_ones = second % 10;

    // Each digit box is roughly 40x80 pixels, separated by 10 pixels
    int digit_width = 40;
    int digit_height = 80;
    int spacing = 10;

    // Hours tens
    tdl_disp_draw_fill_rect(g_frame_buff, x_start, y_start, digit_width, digit_height,
                            color_white, g_disp_info.is_swap);

    // Hours ones
    tdl_disp_draw_fill_rect(g_frame_buff, x_start + digit_width + spacing, y_start,
                            digit_width, digit_height, color_white, g_disp_info.is_swap);

    // Colon separator
    tdl_disp_draw_fill_rect(g_frame_buff, x_start + (digit_width + spacing) * 2, y_start + 30,
                            8, 20, color_white, g_disp_info.is_swap);

    // Minutes tens
    tdl_disp_draw_fill_rect(g_frame_buff, x_start + (digit_width + spacing) * 2 + 20, y_start,
                            digit_width, digit_height, color_white, g_disp_info.is_swap);

    // Minutes ones
    tdl_disp_draw_fill_rect(g_frame_buff, x_start + (digit_width + spacing) * 3 + 20, y_start,
                            digit_width, digit_height, color_white, g_disp_info.is_swap);

    // Show date below in smaller font (using boxes)
    int date_y = y_start + digit_height + 30;
    tdl_disp_draw_rect(g_frame_buff, center_x - 80, date_y, 160, 30,
                       color_dim, g_disp_info.is_swap);

    // Flush frame buffer to display
    tdl_disp_dev_flush(g_disp_hdl, g_frame_buff);
}

/**
 * @brief Set display brightness
 */
void display_set_brightness(uint8_t brightness)
{
    if (g_disp_hdl && brightness <= 100) {
        tdl_disp_set_brightness(g_disp_hdl, brightness);
    }
}

/**
 * @brief Clear display (black screen)
 */
void display_clear(void)
{
    if (!g_frame_buff) return;

    memset(g_frame_buff->frame, 0x00, g_frame_buff->len);
    tdl_disp_dev_flush(g_disp_hdl, g_frame_buff);
}
