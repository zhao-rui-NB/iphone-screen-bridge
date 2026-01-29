

#include <stdint.h>
#include <stdio.h>
#include "LCD.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "tim.h"
#include "ADV7611.h"

#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

// ===== 觸控校正參數 =====
// 這些值需要根據實際測試調整
#define TOUCH_RAW_MIN_X  -231      // 原始觸控 X 最小值
#define TOUCH_RAW_MAX_X  5573      // 原始觸控 X 最大值
#define TOUCH_RAW_MIN_Y  -312      // 原始觸控 Y 最小值（修正：MIN < MAX）
#define TOUCH_RAW_MAX_Y  10116     // 原始觸控 Y 最大值

#define TOUCH_HID_MAX_X  32767   // HID 報告 X 最大值（根據 HID 描述符：Logical Max = 32767）
#define TOUCH_HID_MAX_Y  32767   // HID 報告 Y 最大值

// 座標映射函數
int16_t map_coordinate(int16_t value, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    if (value < in_min) value = in_min;
    if (value > in_max) value = in_max;
    
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void app() {
    // Application code can be added here if needed
    printf("\n\n\n--------- app() ---------\n");

    ADV7611_Init();

    HAL_Delay(10);

    lcd_reset();


    // enable PWM for backlight
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    // set initial backlight duty cycle
    set_bl_duty(60);

    // last time
    static uint32_t lastTime = 0;


    while(1){
        lcd_check_buttons();  // 在迴圈中持續檢查按鍵
        lcd_process_touch();  // 處理接收到的觸控數據
        
        // 獲取最新的觸控數據
        const spi_touch_packet_t* touch = lcd_get_touch_data();
        
        // 每 10ms 發送 USB HID report
        if (HAL_GetTick() - lastTime >= 10) {
            lastTime = HAL_GetTick();
            uint8_t report[5];
            
            // 檢查是否有有效的觸控點
            if (touch->finger_count > 0 && touch->points[0].finger_id != 0xFF) {
                // 使用第一個觸控點的數據
                int16_t raw_x = touch->points[0].x;
                int16_t raw_y = touch->points[0].y;
                uint8_t contact = touch->points[0].contact_state;
                
                // 座標映射與校正
                uint16_t mapped_x = map_coordinate(raw_x, TOUCH_RAW_MIN_X, TOUCH_RAW_MAX_X, 0, TOUCH_HID_MAX_X);
                uint16_t mapped_y = map_coordinate(raw_y, TOUCH_RAW_MIN_Y, TOUCH_RAW_MAX_Y, TOUCH_HID_MAX_Y, 0);  // Y軸反轉：輸出範圍從 4095 到 0


                // 調試輸出：顯示原始座標和映射後座標
                // static uint32_t debug_time = 0;
                // if (HAL_GetTick() - debug_time >= 70) {  // 每 200ms 輸出一次
                //     debug_time = HAL_GetTick();
                //     printf("[Touch Debug] Raw: (%4d, %4d) -> Mapped: (%4d, %4d) | State: %d\n", 
                //            raw_x, raw_y, mapped_x, mapped_y, contact);
                // }

                // 如果觸控按下或移動
                if (contact != 0) {
                    report[0] = 0x03;    // bit0=Tip Switch, bit1=In Range
                    report[1] = (uint8_t)(mapped_x & 0xFF);
                    report[2] = (uint8_t)(mapped_x >> 8);
                    report[3] = (uint8_t)(mapped_y & 0xFF);
                    report[4] = (uint8_t)(mapped_y >> 8);
                    
                    USBD_HID_SendReport(&hUsbDeviceFS, report, 5);
                } else {
                    // 觸控放開
                    report[0] = 0x00;
                    report[1] = 0;
                    report[2] = 0;
                    report[3] = 0;
                    report[4] = 0;
                    USBD_HID_SendReport(&hUsbDeviceFS, report, 5);
                }
            } else {
                // 沒有觸控點，發送 Touch Up
                report[0] = 0x00;
                report[1] = 0;
                report[2] = 0;
                report[3] = 0;
                report[4] = 0;
                USBD_HID_SendReport(&hUsbDeviceFS, report, 5);
            }
        }
    
    
    }





}