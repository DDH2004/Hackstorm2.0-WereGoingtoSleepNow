# Displaying Time on T5 Devboard Display

Based on the AIoTHackStorm VR Avatar project, here's how to display the current time on the T5 Devboard embedded display.

## Architecture Overview

The T5 Devboard uses:
- **Display System**: `TuyaOpen`'s display management API (`tdl_display_manage.h`, `tdl_display_draw.h`)
- **Graphics**: Pixel-by-pixel drawing using `tdl_disp_draw_point()`
- **Time Service**: `tal_time_service.h` for getting current time
- **Double Buffering**: Two frame buffers for smooth rendering

---

## Step 1: Get Current Time

Use the TuyaOpen time service to fetch the current time:

```c
#include "tal_api.h"
#include "tal_time_service.h"

// Get current time in POSIX format
POSIX_TM_S current_time;
OPERATE_RET rt = tal_time_get(&current_time);

if (rt == OPRT_OK) {
    int hour = current_time.tm_hour;
    int minute = current_time.tm_min;
    int second = current_time.tm_sec;
    
    // Format as string: "HH:MM:SS"
    char time_str[16];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hour, minute, second);
}
```

### Alternative: Get Unix timestamp

```c
// Get time in seconds since epoch
TIME_T unix_time = tal_time_get_posix();

// Get time in milliseconds
SYS_TICK_T ms_time = tal_time_get_posix_ms();
```

---

## Step 2: Initialize the Display

```c
#include "tdl_display_manage.h"
#include "board_com_api.h"

#define DISPLAY_NAME "lcd_disp"

// Global display handles
TDL_DISP_HANDLE_T   sg_tdl_disp_hdl = NULL;
TDL_DISP_DEV_INFO_T sg_display_info;
TDL_DISP_FRAME_BUFF_T *sg_p_display_fb = NULL;

static int init_display(void)
{
    OPERATE_RET rt = OPRT_OK;
    
    // Register hardware
    board_register_hardware();
    
    // Find display device
    sg_tdl_disp_hdl = tdl_disp_find_dev(DISPLAY_NAME);
    if (NULL == sg_tdl_disp_hdl)
        return -1;
    
    // Get display info
    rt = tdl_disp_dev_get_info(sg_tdl_disp_hdl, &sg_display_info);
    if (rt != OPRT_OK)
        return -1;
    
    // Open display
    rt = tdl_disp_dev_open(sg_tdl_disp_hdl);
    if (rt != OPRT_OK)
        return -1;
    
    // Set brightness (0-100)
    tdl_disp_set_brightness(sg_tdl_disp_hdl, 100);
    
    // Create frame buffer
    uint32_t screen_width = sg_display_info.width;   // Usually 320
    uint32_t screen_height = sg_display_info.height; // Usually 480
    uint32_t frame_len = screen_width * screen_height * 2; // 2 bytes per pixel
    
    sg_p_display_fb = tdl_disp_create_frame_buff(DISP_FB_TP_PSRAM, frame_len);
    if (sg_p_display_fb) {
        sg_p_display_fb->fmt = sg_display_info.fmt;
        sg_p_display_fb->width = screen_width;
        sg_p_display_fb->height = screen_height;
    }
    
    return 0;
}
```

---

## Step 3: Draw Text/Time on Display

Since TuyaOpen doesn't have a built-in text rendering function, you have three options:

### Option A: Use Pre-rendered Bitmap Digits (Recommended)

Pre-render digit images (0-9) and draw them based on the time string:

```c
#include "tdl_display_draw.h"

// Assuming you have pre-rendered images for digits
extern const unsigned char gImage_digit_0[];  // "0"
extern const unsigned char gImage_digit_1[];  // "1"
// ... etc for digits 2-9
extern const unsigned char gImage_colon[];    // ":"

const unsigned char *digit_images[] = {
    gImage_digit_0, gImage_digit_1, gImage_digit_2, gImage_digit_3,
    gImage_digit_4, gImage_digit_5, gImage_digit_6, gImage_digit_7,
    gImage_digit_8, gImage_digit_9
};

void draw_time_bitmap(int x, int y, const char *time_str)
{
    // Draw each character as a pre-rendered image
    int current_x = x;
    for (int i = 0; time_str[i] != '\0'; i++) {
        char c = time_str[i];
        if (c == ':') {
            tdl_disp_draw_bitmap(sg_p_display_fb, current_x, y, gImage_colon, 
                                 32, 64, sg_display_info.is_swap);
            current_x += 32;
        } else if (c >= '0' && c <= '9') {
            int digit = c - '0';
            tdl_disp_draw_bitmap(sg_p_display_fb, current_x, y, digit_images[digit],
                                 32, 64, sg_display_info.is_swap);
            current_x += 32;
        }
    }
}
```

### Option B: Draw Digits Pixel-by-Pixel (Slower, But Works)

```c
void draw_simple_digit(int x, int y, int digit, uint32_t color)
{
    // Draw a simple 7-segment style digit using lines
    // This is slow but requires no pre-rendered assets
    // See emotion_manager.c for line drawing examples
}
```

### Option C: Use TFT Library (If Available)

If your T5 board has TFT/LCD drivers, check for higher-level APIs like:
- `lvgl` (Light and Versatile Graphics Library)
- `gfxlib` (if integrated in your board config)

---

## Step 4: Main Display Loop

```c
#include "tal_system.h"

void display_time_loop(void)
{
    while (1) {
        // Clear frame buffer (black background)
        memset(sg_p_display_fb->frame, 0x00, sg_p_display_fb->len);
        
        // Get current time
        POSIX_TM_S current_time;
        tal_time_get(&current_time);
        
        // Format time string
        char time_str[16];
        snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d",
                 current_time.tm_hour, current_time.tm_min, current_time.tm_sec);
        
        // Draw time on display (screen center)
        int center_x = sg_display_info.width / 2;
        int center_y = sg_display_info.height / 2;
        draw_time_bitmap(center_x - 48, center_y - 32, time_str);
        
        // Flush display (send frame buffer to hardware)
        tdl_disp_dev_flush(sg_tdl_disp_hdl, sg_p_display_fb);
        
        // Update once per second (or more frequently for smoother animation)
        tal_system_sleep(1000); // 1000ms = 1 second
    }
}
```

---

## Step 5: Initialize in user_main()

```c
void user_main(void)
{
    // Initialize logging
    tal_log_init(TAL_LOG_LEVEL_DEBUG, 4096, (TAL_LOG_OUTPUT_CB)tkl_log_output);
    
    // Initialize system services
    tal_kv_init(&(tal_kv_cfg_t){
        .seed = "your_seed",
        .key = "your_key",
    });
    tal_sw_timer_init();
    tal_workq_init();
    tal_time_service_init();  // <-- CRITICAL for time functions
    tal_cli_init();
    
    // Initialize WiFi (if needed for NTP sync)
    netmgr_init(NETCONN_WIFI);
    
    // Initialize display
    if (init_display() != 0) {
        PR_ERR("Display Init Failed");
        return;
    }
    
    // Start display loop
    display_time_loop();
}
```

---

## Drawing Primitives Available

From `tdl_display_draw.h`:

```c
// Draw a single pixel
void tdl_disp_draw_point(TDL_DISP_FRAME_BUFF_T *p_frame_buff, 
                         int16_t x, int16_t y, 
                         uint32_t color, uint8_t is_swap);

// Draw a line
void tdl_disp_draw_line(TDL_DISP_FRAME_BUFF_T *p_frame_buff,
                        int16_t x0, int16_t y0,
                        int16_t x1, int16_t y1,
                        uint32_t color, uint8_t is_swap);

// Draw a rectangle
void tdl_disp_draw_rect(TDL_DISP_FRAME_BUFF_T *p_frame_buff,
                        int16_t x, int16_t y,
                        int16_t width, int16_t height,
                        uint32_t color, uint8_t is_swap);

// Draw a filled rectangle
void tdl_disp_draw_fill_rect(TDL_DISP_FRAME_BUFF_T *p_frame_buff,
                             int16_t x, int16_t y,
                             int16_t width, int16_t height,
                             uint32_t color, uint8_t is_swap);

// Draw a bitmap image
void tdl_disp_draw_bitmap(TDL_DISP_FRAME_BUFF_T *p_frame_buff,
                          int16_t x, int16_t y,
                          const unsigned char *image,
                          int16_t width, int16_t height,
                          uint8_t is_swap);
```

---

## Color Format

T5 typically uses RGB565 format (16-bit):
- **Red**: `(R << 11) | 0x0000`
- **Green**: `(G << 5) | 0x0000`
- **Blue**: `B | 0x0000`
- **White**: `0xFFFF`
- **Black**: `0x0000`

Example colors:
```c
#define COLOR_RED    0xF800   // (255 << 11)
#define COLOR_GREEN  0x07E0   // (255 << 5)
#define COLOR_BLUE   0x001F   // (255)
#define COLOR_WHITE  0xFFFF
#define COLOR_BLACK  0x0000
```

---

## Performance Tips

1. **Update Rate**: Update display only when time changes (once per second) to reduce power
2. **Double Buffering**: Always use two frame buffers to avoid flicker (already shown above)
3. **Pre-render Assets**: Use bitmap digits instead of pixel-by-pixel drawing
4. **Clear Only Changed Area**: Instead of `memset()`, only clear the time display region
5. **Brightness**: Reduce brightness at night to save power

---

## Reference

See also:
- `AIoTHackStorm/apps/vr_avatar/src/vr_avatar_main.c` - Full working example
- `AIoTHackStorm/examples/e-Paper/` - E-ink display examples
- `AIoTHackStorm/src/tal_system/include/tal_time_service.h` - Time API docs
