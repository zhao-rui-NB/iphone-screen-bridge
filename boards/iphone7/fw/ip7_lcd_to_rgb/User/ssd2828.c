

#include "ssd2828.h"
#include "spi.h"

/*
static void delay_us(uint32_t mdelay)
{
  __IO uint32_t Delay = mdelay * (SystemCoreClock / 8U);
  do
  {
    __NOP();
  }
  while (Delay --);
}
*/

uint16_t ssd2828_read_reg(uint8_t reg){
    SSD_CS_LOW(); // CS low
    
    // send address
    uint8_t cmd[2] = {reg, 0xFA};
    SSD_DC_CMD(); // cmd mode
    HAL_SPI_Transmit(&hspi1, cmd, sizeof(cmd), HAL_MAX_DELAY);

    // read data
    uint8_t data[2] = {0};
    SSD_DC_DATA(); // data mode
    HAL_SPI_Receive(&hspi1, data, sizeof(data), HAL_MAX_DELAY);

    SSD_CS_HIGH(); // CS high
    
    return (data[0]) | (data[1]<<8);
}

void ssd2828_write_reg(uint8_t reg, uint16_t data){
    SSD_CS_LOW(); // CS low
    HAL_Delay(1);

    // send address
    uint8_t cmd[1] = {reg};
    SSD_DC_CMD(); // cmd mode
    HAL_SPI_Transmit(&hspi1, cmd, sizeof(cmd), HAL_MAX_DELAY);

    // send data
    uint8_t tx_data[2] = {data & 0xFF, (data >> 8) & 0xFF};
    SSD_DC_DATA(); // data mode
    HAL_SPI_Transmit(&hspi1, tx_data, sizeof(tx_data), HAL_MAX_DELAY);

    SSD_CS_HIGH(); // CS high
    HAL_Delay(1);


    // check if written
    uint16_t check = ssd2828_read_reg(reg);
    if(check != data){
        printf("Reg 0x%02X write fail: 0x%04X != 0x%04X\r\n", reg, check, data);
    } else
    {
        printf("Reg 0x%02X write success: 0x%04X\r\n", reg, check);
    }

}

void ssd2828_mipi_write_short_1para(uint8_t para){
    SSD_CS_LOW(); // CS low

    ssd2828_write_reg(0xB7, 0x0050);
    ssd2828_write_reg(0xB8, 0x0000);


    ssd2828_write_reg(0xBC, 0x01); // set packet size
    
    SSD_CS_HIGH(); // CS high
    HAL_Delay(1);
    SSD_CS_LOW(); // CS low
    HAL_Delay(1);

    // send address
    uint8_t cmd[1] = {0xBF}; // packet drop register
    SSD_DC_CMD(); // cmd mode
    HAL_SPI_Transmit(&hspi1, cmd, sizeof(cmd), HAL_MAX_DELAY);

    // send data
    uint8_t tx_data[1] = {para};
    SSD_DC_DATA(); // data mode
    HAL_SPI_Transmit(&hspi1, tx_data, sizeof(tx_data), HAL_MAX_DELAY);
    
    HAL_Delay(1);

    SSD_CS_HIGH(); // CS high
    HAL_Delay(1);

}


void ssd2828_init(){
    printf("SSD2828 disable spi pull resistor\r\n");
    // disable spi pull resistor, sck miso mosi
    ssd2828_write_reg(0xE2, 0x05); // only vaid at sync pulse mode

    printf("SSD2828 init\r\n");

    // read a reg
    uint16_t reg_val = ssd2828_read_reg(0xB0);
    printf("Reg 0xB0: 0x%04X\r\n", reg_val);


    ssd2828_write_reg(0xB1, (DISP_VSA << 8) | DISP_HSA); // only vaid at sync pulse mode
    // ssd2828_write_reg(0xB2, (DISP_VBP << 8) | DISP_HBP)
    
    // DISP_VBP += DISP_VSA
    // DISP_HBP += DISP_HSA
    ssd2828_write_reg(0xB2, ((DISP_VBP+DISP_VSA) << 8) | (DISP_HBP+DISP_HSA));
    ssd2828_write_reg(0xB3, (uint16_t)(DISP_VFP << 8) | DISP_HFP);
    ssd2828_write_reg(0xB4, DISP_HACT);
    ssd2828_write_reg(0xB5, DISP_VACT);


    //rgb mode // burst, bit 3-2
    // 00 – Non burst mode with sync pulses, 01 – Non burst mode with sync events, 10 – Burst mode
    // ssd2828_write_reg(0xB6, (0b00000000<<8) | (0b00000111)) // 24bit rgb, pclk posedge, video MODE
    ssd2828_write_reg(0xB6, (0b00000000<<8) | (0b00001011)); // 24bit rgb, pclk posedge, video MODE ////////// test
    
    // bit9:EOT, bit8:ECD, bit6:DCS, bit5:CSS, bit4:HCLK, bit3:VEN, bit2:SLP, bit1:CKE, bit0:HS
    // ssd2828_write_reg(0xB7, 1<<5) // CSS ref pclk, set before pll on
    ssd2828_write_reg(0xB7, (0b00000000)<<8 | (0b01100010)); // hs enable //////////// test
    ssd2828_write_reg(0xB8, 0x0000); // all channel id set to 0
    // B9 PLL control reg
    
    // pll pclk 50mhz*10 = 500mbps
    // bit15-14: 00 - 62.5-125, 01 - 126-250, 10 - 251-500, 11 - 501-1000
    ssd2828_write_reg(0xBA,(0b11000000)<<8 | 13); // pll speed
    // ssd2828_write_reg(0xBA,(0b11000000)<<8 | 13) // pll speed //////// test
    
    // div 12 840mbps/8/LPD = 8.75 MHZ // div 7, 500mbps/8/LPD = 8.92 MHZ
    ssd2828_write_reg(0xBB,0x000C); // LP clock div

    // BC-BF packet control
    
    ssd2828_write_reg(0xC9, HZD<<8  | HPD   );
    ssd2828_write_reg(0xCA, CZD<<8  | CPD   );
    ssd2828_write_reg(0xCB, CPED<<8 | CPTD  );
    ssd2828_write_reg(0xCC, CTD<<8  | HTD   );
    // CD : wake up time, CE : ta-go and ta-get time
    
    ssd2828_write_reg(0xDE, 0x0001); // 2 lane

    // ////// exit sleep mode, set display on
    ssd2828_write_reg(0xB9, 0xC001); // PLL ON
    // ssd2828_write_reg(0xB7, (0b00000000)<<8 | (0b00100011)) // hs enable

    ssd2828_write_reg(0xBC, 0x0001); // 1 byte data
    ssd2828_write_reg(0xBF, 0x0011); // DCS command 0x11
    HAL_Delay(300);
    
    ssd2828_write_reg(0xBC, 0x0001); // 1 byte data
    ssd2828_write_reg(0xBF, 0x0029); // DCS command 0x29
    HAL_Delay(300);
    

    // bit9:EOT, bit8:ECD, bit6:DCS, bit5:CSS, bit3:VEN, bit2:SLP, bit1:CKE, bit0:HS
    ssd2828_write_reg(0xb7, (0b00000000)<<8 | (0b01101011)); // ven

    // time.sleep(3)
    // ssd2828_write_reg(0xee,0x0600); //ssd2828 bist

    printf("SSD2828 init done\r\n");

}

// ssd2828_init_test_txclk

void ssd2828_init_test_txclk(){

    // bit9:EOT, bit8:ECD, bit6:DCS, bit5:CSS, bit3:VEN, bit2:SLP, bit1:CKE, bit0:HS
    ssd2828_write_reg(0xb7, (0b00000000)<<8 | (0b01001011)); // ven // css off test

    // time.sleep(3)
    ssd2828_write_reg(0xee,0x0600); //ssd2828 bist


    printf("ssd2828_init_test_txclk \r\n");
    
    // read a reg
    uint16_t reg_val = ssd2828_read_reg(0xB0);
    printf("Reg 0xB0: 0x%04X\r\n", reg_val);
    return ;

    ssd2828_write_reg(0xB1, (DISP_VSA << 8) | DISP_HSA); // only vaid at sync pulse mode
    // ssd2828_write_reg(0xB2, (DISP_VBP << 8) | DISP_HBP)
    
    // DISP_VBP += DISP_VSA
    // DISP_HBP += DISP_HSA
    ssd2828_write_reg(0xB2, ((DISP_VBP+DISP_VSA) << 8) | (DISP_HBP+DISP_HSA));
    ssd2828_write_reg(0xB3, (uint16_t)(DISP_VFP << 8) | DISP_HFP);
    ssd2828_write_reg(0xB4, DISP_HACT);
    ssd2828_write_reg(0xB5, DISP_VACT);


    //rgb mode // burst, bit 3-2
    // 00 – Non burst mode with sync pulses, 01 – Non burst mode with sync events, 10 – Burst mode
    // ssd2828_write_reg(0xB6, (0b00000000<<8) | (0b00000111)) // 24bit rgb, pclk posedge, video MODE
    ssd2828_write_reg(0xB6, (0b00000000<<8) | (0b00001011)); // 24bit rgb, pclk posedge, video MODE ////////// test
    
    // bit9:EOT, bit8:ECD, bit6:DCS, bit5:CSS, bit4:HCLK, bit3:VEN, bit2:SLP, bit1:CKE, bit0:HS
    // ssd2828_write_reg(0xB7, 1<<5) // CSS ref pclk, set before pll on
    // ssd2828_write_reg(0xB7, (0b00000000)<<8 | (0b01100010)); // hs enable //////////// test
    ssd2828_write_reg(0xB8, 0x0000); // all channel id set to 0
    // B9 PLL control reg
    
    // pll pclk 50mhz*10 = 500mbps
    // bit15-14: 00 - 62.5-125, 01 - 126-250, 10 - 251-500, 11 - 501-1000
    ssd2828_write_reg(0xBA,(0b11000000)<<8 | 80); // pll speed
    // ssd2828_write_reg(0xBA,(0b11000000)<<8 | 13) // pll speed //////// test
    
    // div 12 840mbps/8/LPD = 8.75 MHZ // div 7, 500mbps/8/LPD = 8.92 MHZ
    ssd2828_write_reg(0xBB,0x000C); // LP clock div

    // BC-BF packet control
    
    ssd2828_write_reg(0xC9, HZD<<8  | HPD   );
    ssd2828_write_reg(0xCA, CZD<<8  | CPD   );
    ssd2828_write_reg(0xCB, CPED<<8 | CPTD  );
    ssd2828_write_reg(0xCC, CTD<<8  | HTD   );
    // CD : wake up time, CE : ta-go and ta-get time
    
    ssd2828_write_reg(0xDE, 0x0001); // 2 lane

    // ////// exit sleep mode, set display on
    ssd2828_write_reg(0xB9, 0xC001); // PLL ON
    // ssd2828_write_reg(0xB7, (0b00000000)<<8 | (0b00100011)) // hs enable

    ssd2828_write_reg(0xBC, 0x0001); // 1 byte data
    ssd2828_write_reg(0xBF, 0x0011); // DCS command 0x11
    HAL_Delay(300);
    
    ssd2828_write_reg(0xBC, 0x0001); // 1 byte data
    ssd2828_write_reg(0xBF, 0x0029); // DCS command 0x29
    HAL_Delay(300);
    

    // bit9:EOT, bit8:ECD, bit6:DCS, bit5:CSS, bit3:VEN, bit2:SLP, bit1:CKE, bit0:HS
    ssd2828_write_reg(0xb7, (0b00000000)<<8 | (0b01001011)); // ven // css off test

    // time.sleep(3)
    ssd2828_write_reg(0xee,0x0600); //ssd2828 bist

    printf("SSD2828 init done\r\n");

}