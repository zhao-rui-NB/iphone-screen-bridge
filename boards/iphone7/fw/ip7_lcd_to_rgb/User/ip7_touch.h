#ifndef __IP7_TOUCH_H__
#define __IP7_TOUCH_H__

#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "spi.h"


#define TOUCH_CS_LOW()    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET)
#define TOUCH_CS_HIGH()   HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET)

// command type is used to indicate if we need to wait for touch INT pin after sending the command
typedef enum {
    NORMAL = 0,
    WAIT_INT_HIGH = 1
} CommandType;


void ip7_touch_init();
void ip7_touch_read_data();

#endif /* __IP7_TOUCH_H__ */
