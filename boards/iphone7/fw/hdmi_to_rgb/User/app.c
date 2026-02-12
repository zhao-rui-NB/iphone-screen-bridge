

#include <stdint.h>
#include <stdio.h>
#include "LCD.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx_hal_gpio.h"
#include "tim.h"
#include "ADV7611.h"
#include "usart.h"

#include "usbd_hid.h"

extern USBD_HandleTypeDef hUsbDeviceFS;

void app() {
    printf("\n\n\n--------- app() ---------\n");

    ADV7611_Init();

    lcd_reset();

    // enable PWM for backlight
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    // set initial backlight duty cycle
    set_bl_duty(60);

    // boot complete. turn on LED1
    HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET);

    static uint32_t last_report_time = 0;
    static uint32_t last_hdmi_check_time = 0;
    static uint8_t last_hdmi_cp_lock = 0; // hdmi 輸入有沒有成功鎖定
    while(1){
        lcd_check_buttons();  // 在迴圈中持續檢查按鍵
        lcd_process_touch();  // 處理接收到的觸控數據
        // 獲取最新的觸控數據
        
        if (HAL_GetTick() - last_report_time >= 10) {
            last_report_time = HAL_GetTick();
            fill_touch_hid_report(&spi_touch_packet, &touch_hid_report);
            USBD_HID_SendReport(&hUsbDeviceFS, (uint8_t*)&touch_hid_report, sizeof(touch_hid_report_t));
        }


        if (HAL_GetTick() - last_hdmi_check_time >= 500) {
            last_hdmi_check_time = HAL_GetTick();
            // ADV7611_Print_Debug_Status();

            uint8_t cp_lock = ADV7611_CheckSignal();
            if (cp_lock != last_hdmi_cp_lock) {
                printf("HDMI CP Lock changed: %d -> %d\n", last_hdmi_cp_lock, cp_lock);
            }

            // if last not lock and now lock, means just got signal, reset lcd
            if (!last_hdmi_cp_lock && cp_lock) {
                printf("HDMI signal just locked, resetting LCD...\n");
                lcd_reset();
                printf("LCD reset complete.\n");
            }
            last_hdmi_cp_lock = cp_lock;
        }

        // read uart input for set dll phase
        // read 'p' followed by digits, e.g., "p10" sets phase to 10
        // if (HAL_UART_Receive(&huart1, (uint8_t*)&last_hdmi_cp_lock, 1, 0) == HAL_OK) {
        //     if (last_hdmi_cp_lock == 'p') {
        //         uint8_t phase = 0;
        //         uint8_t byte;
                
        //         // Read digits and convert to integer
        //         while (HAL_UART_Receive(&huart1, &byte, 1, 10) == HAL_OK) {
        //             if (byte >= '0' && byte <= '9') {
        //                 phase = phase * 10 + (byte - '0');
        //             } else {
        //                 break;
        //             }
        //         }
        //         printf("Setting DLL phase to %d\n", phase);
        //         ADV7611_set_dll_phase(phase);

        //         HAL_Delay(10);
        //         // lcd reset
        //         lcd_reset();
        //     }
        // }
            
    }
}