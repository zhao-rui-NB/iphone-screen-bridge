#ifndef _LCD_H_
#define _LCD_H_

#include <stdint.h>

#define BTN_DEBOUNCE_MS 50         // 防抖動時間
#define BTN_LONG_PRESS_MS 500      // 長按判定時間
#define BTN_REPEAT_FAST_MS 20      // 長按時的快速重複間隔
#define BTN_REPEAT_INITIAL_MS 200  // 第一次重複前的等待時間

#define BL_DUTY_MAX 99
#define BL_DUTY_MIN 1

void lcd_check_buttons();
void lcd_reset();
void set_bl_duty(uint8_t duty);

#endif /* _LCD_H_ */
