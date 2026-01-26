#include <stdio.h>
#include "main.h"
#include "tim.h"
#include "LCD.h"

// 按鍵狀態追蹤
typedef struct {
    uint8_t is_pressed;           // 當前是否按下
    uint8_t was_pressed;          // 上次檢查時是否按下
    uint32_t press_start_time;    // 按下開始時間
    uint32_t last_repeat_time;    // 上次重複觸發時間
    uint8_t is_long_press;        // 是否已進入長按模式
} ButtonState;

ButtonState btn1_state = {0};
ButtonState btn2_state = {0};
volatile uint8_t bl_duty = 40;

void lcd_reset() {
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
}

void set_bl_duty(uint8_t duty) {
    if (duty > BL_DUTY_MAX) {
        duty = BL_DUTY_MAX;
    } else if (duty < BL_DUTY_MIN) {
        duty = BL_DUTY_MIN;
    }
    
    bl_duty = duty;
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, bl_duty);
}

// 處理單個按鍵的狀態和重複邏輯
static void process_button(ButtonState *btn, GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, int8_t direction) {
    uint32_t current_time = HAL_GetTick();
    btn->is_pressed = (HAL_GPIO_ReadPin(GPIOx, GPIO_Pin) == GPIO_PIN_RESET);
    
    if (btn->is_pressed && !btn->was_pressed) {
        // 按鍵剛按下 - 短按第一次觸發
        btn->press_start_time = current_time;
        btn->last_repeat_time = current_time;
        btn->is_long_press = 0;
        
        // 立即調整亮度
        if (direction > 0 && bl_duty < BL_DUTY_MAX) {
            bl_duty += 1;
            set_bl_duty(bl_duty);
            printf("BTN+ Short: duty=%d\n", bl_duty);
        } else if (direction < 0 && bl_duty > BL_DUTY_MIN) {
            bl_duty -= 1;
            set_bl_duty(bl_duty);
            printf("BTN- Short: duty=%d\n", bl_duty);
        }
    } 
    else if (btn->is_pressed && btn->was_pressed) {
        // 按鍵持續按下
        uint32_t press_duration = current_time - btn->press_start_time;
        
        if (!btn->is_long_press && press_duration >= BTN_LONG_PRESS_MS) {
            // 進入長按模式
            btn->is_long_press = 1;
            printf("Long press detected\n");
        }
        
        if (btn->is_long_press) {
            // 長按模式 - 快速連續調整
            uint32_t since_last_repeat = current_time - btn->last_repeat_time;
            if (since_last_repeat >= BTN_REPEAT_FAST_MS) {
                btn->last_repeat_time = current_time;
                
                if (direction > 0 && bl_duty < BL_DUTY_MAX) {
                    bl_duty += 1;
                    set_bl_duty(bl_duty);
                    printf("BTN+ Long: duty=%d\n", bl_duty);
                } else if (direction < 0 && bl_duty > BL_DUTY_MIN) {
                    bl_duty -= 1;
                    set_bl_duty(bl_duty);
                    printf("BTN- Long: duty=%d\n", bl_duty);
                }
            }
        }
    } 
    else if (!btn->is_pressed && btn->was_pressed) {
        // 按鍵剛釋放
        if (btn->is_long_press) {
            printf("Long press released\n");
        }
        btn->is_long_press = 0;
    }
    
    btn->was_pressed = btn->is_pressed;
}

void lcd_check_buttons() {
    // BTN1 - 增加亮度
    process_button(&btn1_state, BTN1_GPIO_Port, BTN1_Pin, 1);
    
    // BTN2 - 減少亮度
    process_button(&btn2_state, BTN2_GPIO_Port, BTN2_Pin, -1);
}














