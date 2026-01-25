#ifndef _ADV7611_H_
#define _ADV7611_H_

#include "i2c.h"

/*
98 F4 80 CEC Map I2C address
98 F5 7C INFOFRAME Map I2C address
98 F8 4C DPLL Map I2C address
98 F9 64 KSV Map I2C address
98 FA 6C EDID Map I2C address
98 FB 68 HDMI Map I2C address
98 FD 44 CP Map I2C address
*/

#define ADV7611_IO_ADDR        0x98
#define ADV7611_CEC_ADDR       0x80
#define ADV7611_INFOFRAME_ADDR 0x7C
#define ADV7611_DPLL_ADDR      0x4C
#define ADV7611_KSV_ADDR       0x64
#define ADV7611_EDID_ADDR      0x6C
#define ADV7611_HDMI_ADDR      0x68
#define ADV7611_CP_ADDR        0x44



void ADV7611_Init();







#endif /* _ADV7611_H_ */
