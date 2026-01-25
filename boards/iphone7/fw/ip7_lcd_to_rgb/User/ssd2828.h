#ifndef __SSD2828_H__
#define __SSD2828_H__

#include <stdint.h>
#include <stdio.h>
#include "spi.h"

// display parameters
#define DISP_HSA    3
#define DISP_HBP    3
#define DISP_HACT   750
#define DISP_HFP    600

#define DISP_VSA    3
#define DISP_VBP    3
#define DISP_VACT   1334
#define DISP_VFP    500

// mipi timing parameters
#define HZD     32
#define HPD     9
#define CZD     75
#define CPD     8
#define CPED    2
#define CPTD    31
#define CTD     18
#define HTD     19

// #define HZD     15
// #define HPD     2
// #define CZD     33
// #define CPD     2
// #define CPED    3
// #define CPTD    21
// #define CTD     8
// #define HTD     9


#define SSD_CS_LOW() HAL_GPIO_WritePin(SSD2828_CS_GPIO_Port, SSD2828_CS_Pin, GPIO_PIN_RESET)
#define SSD_CS_HIGH() HAL_GPIO_WritePin(SSD2828_CS_GPIO_Port, SSD2828_CS_Pin, GPIO_PIN_SET)
#define SSD_DC_CMD() HAL_GPIO_WritePin(SSD2828_DC_GPIO_Port, SSD2828_DC_Pin, GPIO_PIN_RESET)
#define SSD_DC_DATA() HAL_GPIO_WritePin(SSD2828_DC_GPIO_Port, SSD2828_DC_Pin, GPIO_PIN_SET)

void ssd2828_init();
void ssd2828_init_test_txclk();



#endif /* __SSD2828_H__ */