#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>

#define BTN_DEBOUNCE_MS 50         // 防抖動時間
#define BTN_LONG_PRESS_MS 500      // 長按判定時間
#define BTN_REPEAT_FAST_MS 20      // 長按時的快速重複間隔
#define BTN_REPEAT_INITIAL_MS 200  // 第一次重複前的等待時間

#define BL_DUTY_MAX 99
#define BL_DUTY_MIN 1

// touch panel
#define TOUCH_MAX_POINTS 5



#pragma pack(push, 1)

typedef struct {
    uint8_t  magic;             // [0] 0xAA 起始標記
    uint8_t  finger_count;      // [1] 實際手指數量 (0-5)
    uint32_t scan_count;        // [2-5] 掃描計數器
    uint8_t  reserved[2];       // [6-7] 保留
    
    // 固定5個點的空間，沒用到的填0
    struct {
        uint8_t  finger_id;     // 手指ID (0xFF表示無效)
        uint16_t x;             // X座標
        uint16_t y;             // Y座標  
        uint8_t  contact_state; // 狀態 (0=up, 1=down, 2=move)
    } points[TOUCH_MAX_POINTS];                // [8-37] 5×6=30 bytes
    
    uint8_t  crc;               // [38] CRC校驗
    uint8_t  end_marker;        // [39] 0x55 結束標記
} spi_touch_packet_t;  // 總共 40 bytes

#pragma pack(pop)


void lcd_check_buttons();
void lcd_reset();
void set_bl_duty(uint8_t duty);
void lcd_process_touch(void);

#endif /* _LCD_H_ */
