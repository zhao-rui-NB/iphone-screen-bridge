#include <stdio.h>
#include <string.h>
#include "main.h"
#include "spi.h"
#include "stm32f1xx_hal_gpio.h"
#include "stm32f1xx_hal_spi.h"
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
    // test SPI
    HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_SET);
    HAL_Delay(10);

    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    // 消除spi控制器殘留狀態
    uint8_t tx_data = 0x66;
    uint8_t rx_data = 0;
    HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_SPI_TransmitReceive(&hspi2, &tx_data, &rx_data, 1, HAL_MAX_DELAY);
    HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_SET);
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


// touch 


// 接收緩衝區和狀態
static spi_touch_packet_t touch_rx_buffer;
static uint8_t dummy_tx_buffer[sizeof(spi_touch_packet_t)] = {0};  // 發送假數據產生時鐘
static volatile uint8_t touch_data_ready = 0;
static volatile uint8_t spi_receiving = 0;

volatile uint8_t need_read_touch = 0; 


// CRC-8 計算函數（與 slave 端相同）
static uint8_t calc_crc8(const uint8_t *data, uint16_t len) {
    uint8_t crc = 0xFF;
    for (uint16_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t j = 0; j < 8; j++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x31;
            else
                crc <<= 1;
        }
    }
    return crc;
}

// 驗證並處理接收到的觸控數據
static void process_touch_data(void) {
    spi_touch_packet_t *packet = &touch_rx_buffer;
    
    // 驗證 magic 和 end_marker
    if (packet->magic != 0xAA || packet->end_marker != 0x55) {
        printf("[Touch] Invalid packet markers: magic=0x%02X, end=0x%02X\n", 
               packet->magic, packet->end_marker);
        return;
    }
    
    // 驗證 CRC（不包含 crc 和 end_marker）
    uint8_t calculated_crc = calc_crc8((uint8_t*)packet, 38);
    if (calculated_crc != packet->crc) {
        printf("[Touch] CRC mismatch: calc=0x%02X, recv=0x%02X\n", 
               calculated_crc, packet->crc);
        return;
    }
    
    // 數據有效，處理觸控信息
    printf("[Touch] fingers=%d\t", packet->finger_count);
    
    // for (uint8_t i = 0; i < packet->finger_count && i < TOUCH_MAX_POINTS; i++) {
    //     if (packet->points[i].finger_id != 0xFF) {
    //         printf("  [%d] id=%d, x=%d, y=%d, state=%d",
    //                i,
    //                packet->points[i].finger_id,
    //                packet->points[i].x,
    //                packet->points[i].y,
    //                packet->points[i].contact_state);
    //     }
    // }

    printf("\n");
}

// GPIO 中斷回調 - 當 INT 引腳變為低電平時觸發
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == SCREEN_SPI2_INT_Pin && !spi_receiving) {
        need_read_touch = 1;
    }
}
// SPI 接收完成回調
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        // 拉高 CS 結束傳輸
        HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_SET);
        spi_receiving = 0;
        touch_data_ready = 1;
    }
}

// SPI 接收錯誤回調
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) {
    if (hspi->Instance == SPI2) {
        // 拉高 CS 結束傳輸
        HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_SET);
        
        spi_receiving = 0;
        printf("[Touch] SPI receive error: 0x%08X\n", (unsigned int)hspi->ErrorCode);
    }
}

// 在主循環中調用此函數以處理接收到的觸控數據
void lcd_process_touch(void) {
    // 檢查是否需要啟動接收
    if (need_read_touch && !spi_receiving) {
        need_read_touch = 0;
        
        // 清空接收緩衝
        memset(&touch_rx_buffer, 0, sizeof(spi_touch_packet_t));
        
        // 拉低 CS 開始傳輸
        HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_RESET);
        for(volatile int i = 0; i < 200; i++);  // 短暫延遲確保 CS 穩定
        // 啟動 DMA 同時收發
        spi_receiving = 1;
        if (HAL_SPI_TransmitReceive_DMA(&hspi2, dummy_tx_buffer, (uint8_t*)&touch_rx_buffer, sizeof(spi_touch_packet_t)) != HAL_OK) {
            spi_receiving = 0;
            HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_SET);
            printf("[Touch] Failed to start SPI: State=%d, Err=0x%08X\n", hspi2.State, (unsigned int)hspi2.ErrorCode);
        }
    }
    
    // 處理接收完成的數據
    if (touch_data_ready) {
        touch_data_ready = 0;
        process_touch_data();
    }
}












