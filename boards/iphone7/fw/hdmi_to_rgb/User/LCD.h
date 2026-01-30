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

// HID 報告中最大支援的觸控點數
#define MAX_REPORT_FINGERS 5
#define FG_NONE          0x00
#define FG_TIP_SWITCH    0x01
#define FG_IN_RANGE      0x02



// ===== 觸控校正參數 =====
// 這些值需要根據實際測試調整
#define TOUCH_RAW_MIN_X  -231      // 原始觸控 X 最小值
#define TOUCH_RAW_MAX_X  5573      // 原始觸控 X 最大值
#define TOUCH_RAW_MIN_Y  -312      // 原始觸控 Y 最小值（修正：MIN < MAX）
#define TOUCH_RAW_MAX_Y  10116     // 原始觸控 Y 最大值

#define TOUCH_HID_MAX_X  32767   // HID 報告 X 最大值（根據 HID 描述符：Logical Max = 32767）
#define TOUCH_HID_MAX_Y  32767   // HID 報告 Y 最大值


#pragma pack(push, 1)
// SPI 觸控數據包結構
typedef struct {
    uint8_t  magic;             // [0] 0xAA 起始標記
    uint8_t  finger_count;      // [1] 實際手指數量 (0-5)
    uint32_t scan_count;        // [2-5] 掃描計數器
    uint8_t  reserved[2];       // [6-7] 保留
    
    // 固定5個點的空間，沒用到的填0
    struct {
        uint8_t  finger_id;     // 手指ID (0xFF表示無效)
        int16_t x;             // X座標
        int16_t y;             // Y座標  
        uint8_t  contact_state; // 狀態 (0=up, 1=down, 2=move)
    } points[TOUCH_MAX_POINTS];                // [8-37] 5×6=30 bytes
    
    uint8_t  crc;               // [38] CRC校驗
    uint8_t  end_marker;        // [39] 0x55 結束標記
} spi_touch_packet_t;  // 總共 40 bytes

#pragma pack(pop)


#pragma pack(push, 1)
// HID 觸控報告結構
typedef struct {
    uint8_t contant_count;      // 實際手指數量 (0-5)

    struct {
        uint8_t  fg_state;        // 狀態標誌位
        uint8_t fg_id;           // 手指ID
        uint16_t x;              // X座標
        uint16_t y;              // Y座標
    } points[MAX_REPORT_FINGERS];
} touch_hid_report_t;
#pragma pack(pop)


extern spi_touch_packet_t spi_touch_packet;
extern touch_hid_report_t touch_hid_report;

void lcd_check_buttons();
void lcd_reset();
void set_bl_duty(uint8_t duty);
void lcd_process_touch(void);

// 獲取最新的觸控數據
const spi_touch_packet_t* lcd_get_touch_data();
void map_coordinate_to_hid(uint16_t raw_x, uint16_t raw_y, uint16_t* hid_x, uint16_t* hid_y);
void fill_touch_hid_report(spi_touch_packet_t* touch_data, touch_hid_report_t* touch_hid_report);


#endif /* _LCD_H_ */
