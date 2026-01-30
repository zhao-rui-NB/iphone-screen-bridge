#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "main.h"
#include "spi.h"
#include "stm32f1xx_hal.h"
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

    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_SET);
    HAL_Delay(10);
    HAL_GPIO_WritePin(LCD_RESET_GPIO_Port, LCD_RESET_Pin, GPIO_PIN_RESET);
    HAL_Delay(10);

    // 消除spi控制器殘留狀態
    uint8_t tx_data = 0x66;
    uint8_t rx_data = 0;
    HAL_SPI_TransmitReceive(&hspi2, &tx_data, &rx_data, 1, HAL_MAX_DELAY);
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
spi_touch_packet_t spi_touch_packet;
touch_hid_report_t touch_hid_report;
uint8_t dummy_tx_buffer[sizeof(spi_touch_packet_t)] = {0};  // 發送假數據產生時鐘

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
    spi_touch_packet_t *packet = &spi_touch_packet;
    
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

#if 0
    // 數據有效，處理觸控信息
    printf("[TP] P:%d\t", packet->finger_count);
    for (uint8_t i = 0; i < packet->finger_count && i < TOUCH_MAX_POINTS; i++) {
        if (packet->points[i].finger_id != 0xFF) {
            printf("  [%d] id=%d, x=%d, y=%d, state=%d",
                   i,
                   packet->points[i].finger_id,
                   packet->points[i].x,
                   packet->points[i].y,
                   packet->points[i].contact_state);
        }
    }
    printf("\n");
#endif

}

// GPIO 中斷回調 - 當 INT 引腳變為低電平時觸發
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
    if (GPIO_Pin == SCREEN_SPI2_INT_Pin) {
        need_read_touch = 1;
    }
}

// 在主循環中調用此函數以處理接收到的觸控數據
void lcd_process_touch(void) {
    // 檢查是否需要啟動接收
    if (need_read_touch) {
        
        // 清空接收緩衝
        memset(&spi_touch_packet, 0, sizeof(spi_touch_packet_t));
        
        // 拉低 CS 開始傳輸
        HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_RESET);
        // delay for cs setup time
        for (volatile int i = 0; i < 100; i++);
        
        HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi2, dummy_tx_buffer, (uint8_t*)&spi_touch_packet, sizeof(spi_touch_packet_t), HAL_MAX_DELAY);
        
        if (status == HAL_OK) {
            // 傳輸成功，立即處理數據
            process_touch_data();
        } else {
            printf("[Touch] SPI transfer failed: State=%d, Err=0x%08X\n", hspi2.State, (unsigned int)hspi2.ErrorCode);
        }
        
        HAL_GPIO_WritePin(SCREEN_SPI2_CS_GPIO_Port, SCREEN_SPI2_CS_Pin, GPIO_PIN_SET);
        
        need_read_touch = 0;
    }

    // LED3,4 顯示觸控狀態 
    // led low active
    // 0 points: both off
    // 1 point: LED3 off, LED4 on
    // 2 points: LED3 on, LED4 off
    // >2 points: both on

    uint8_t active_points = 0;
    for (uint8_t i = 0; i < spi_touch_packet.finger_count && i < TOUCH_MAX_POINTS; i++) {
        if (spi_touch_packet.points[i].contact_state != 0 && spi_touch_packet.points[i].finger_id != 0xFF) {
            active_points++;
        }
    }

    if (active_points == 1) {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET);
    } else if (active_points == 2) {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
    } else if (active_points > 2) {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET);
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET);
    } else {
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET);
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET);
    }
}

// 獲取最新的觸控數據
const spi_touch_packet_t* lcd_get_touch_data() {
    return &spi_touch_packet;
}

int16_t map_coordinate(int16_t value, int16_t in_min, int16_t in_max, int16_t out_min, int16_t out_max) {
    if (value < in_min) value = in_min;
    if (value > in_max) value = in_max;
    
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

void map_coordinate_to_hid(uint16_t raw_x, uint16_t raw_y, uint16_t* hid_x, uint16_t* hid_y){
    uint16_t mapped_x = map_coordinate(raw_x, TOUCH_RAW_MIN_X, TOUCH_RAW_MAX_X, 0, TOUCH_HID_MAX_X);
    uint16_t mapped_y = map_coordinate(raw_y, TOUCH_RAW_MIN_Y, TOUCH_RAW_MAX_Y, TOUCH_HID_MAX_Y, 0);  // Y軸反轉
    *hid_x = mapped_x;
    *hid_y = mapped_y;
}

// 填充 HID 觸控報告
void fill_touch_hid_report(spi_touch_packet_t* touch_data, touch_hid_report_t* touch_hid_report) {
    memset(touch_hid_report, 0, sizeof(touch_hid_report_t));
    
    uint8_t active_count = 0;  // 實際按下的點數

    for (uint8_t i = 0; i < MAX_REPORT_FINGERS; i++) {
        // 檢查是否有對應的觸控數據
        if (i < touch_data->finger_count && touch_data->points[i].finger_id != 0xFF) {
            // 獲取原始座標
            int16_t raw_x = (int16_t)touch_data->points[i].x;
            int16_t raw_y = (int16_t)touch_data->points[i].y;
            uint8_t contact = touch_data->points[i].contact_state;
            
            // 座標映射與校正
            uint16_t mapped_x = map_coordinate(raw_x, TOUCH_RAW_MIN_X, TOUCH_RAW_MAX_X, 0, TOUCH_HID_MAX_X);
            uint16_t mapped_y = map_coordinate(raw_y, TOUCH_RAW_MIN_Y, TOUCH_RAW_MAX_Y, TOUCH_HID_MAX_Y, 0);  // Y軸反轉
            
            if (contact != 0) {  // 按下或移動
                touch_hid_report->points[i].fg_state = FG_TIP_SWITCH | FG_IN_RANGE;
                touch_hid_report->points[i].fg_id = touch_data->points[i].finger_id;
                touch_hid_report->points[i].x = mapped_x;
                touch_hid_report->points[i].y = mapped_y;
                active_count++;
            } else {
                // 觸控放開，狀態為 0，但仍需填充該位置
                touch_hid_report->points[i].fg_state = 0;
                touch_hid_report->points[i].fg_id = touch_data->points[i].finger_id;
                touch_hid_report->points[i].x = 0;
                touch_hid_report->points[i].y = 0;
            }
        }
        // 否則保持默認值（全部為0）
    }
    
    // 設置實際按下的觸控點數量
    touch_hid_report->contant_count = active_count;
}













