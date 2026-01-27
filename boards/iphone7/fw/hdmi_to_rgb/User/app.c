

#include <stdio.h>
#include "LCD.h"
#include "stm32f1xx_hal.h"
#include "tim.h"
#include "ADV7611.h"

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

    while(1){
      lcd_check_buttons();  // 在迴圈中持續檢查按鍵
      lcd_process_touch();  // 處理接收到的觸控數據
      // HAL_Delay(3000);
      // ADV7611_Print_Debug_Status();
    }





}