

#include <stdint.h>
#include <stdio.h>
#include "LCD.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "tim.h"
#include "ADV7611.h"

#include "usbd_hid.h"
extern USBD_HandleTypeDef hUsbDeviceFS;

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
        // HAL_Delay(3000);
        // ADV7611_Print_Debug_Status();

        // each 20 ms send usb hid report
        if (HAL_GetTick() - lastTime >= 10) {
            lastTime = HAL_GetTick();
            uint8_t report[5];



            if (HAL_GPIO_ReadPin(BTN1_GPIO_Port, BTN1_Pin) == GPIO_PIN_RESET) {
                uint16_t x = 1000;   // 固定觸控點 X
                uint16_t y = 1000;   // 固定觸控點 Y
                
                // ===== Touch Down =====
                report[0] = 0x03;    // bit0=Tip Switch, bit1=In Range
                report[1] = (uint8_t)(x & 0xFF);
                report[2] = (uint8_t)(x >> 8);
                report[3] = (uint8_t)(y & 0xFF);
                report[4] = (uint8_t)(y >> 8);
                
                USBD_HID_SendReport(&hUsbDeviceFS, report, 5);
            }
            else if (HAL_GPIO_ReadPin(BTN2_GPIO_Port, BTN2_Pin) == GPIO_PIN_RESET) {
                uint16_t x = 500;   // 固定觸控點 X
                uint16_t y = 500;   // 固定觸控點 Y
                
                // ===== Touch Down =====
                report[0] = 0x03;    // bit0=Tip Switch, bit1=In Range
                report[1] = (uint8_t)(x & 0xFF);
                report[2] = (uint8_t)(x >> 8);
                report[3] = (uint8_t)(y & 0xFF);
                report[4] = (uint8_t)(y >> 8);
                
                USBD_HID_SendReport(&hUsbDeviceFS, report, 5);
            }
            else {
                // ===== Touch Up =====
                report[0] = 0x00;    // 放開
                USBD_HID_SendReport(&hUsbDeviceFS, report, 5);
            }

                

        }
    
    
    }





}