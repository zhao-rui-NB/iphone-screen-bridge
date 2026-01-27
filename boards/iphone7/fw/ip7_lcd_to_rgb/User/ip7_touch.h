#ifndef __IP7_TOUCH_H__
#define __IP7_TOUCH_H__

#include <stdint.h>
#include <stdio.h>
#include "main.h"
#include "spi.h"


#define TOUCH_CS_LOW()    HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_RESET)
#define TOUCH_CS_HIGH()   HAL_GPIO_WritePin(TOUCH_CS_GPIO_Port, TOUCH_CS_Pin, GPIO_PIN_SET)

#define TOUCH_MAX_POINTS 5


// command type is used to indicate if we need to wait for touch INT pin after sending the command
typedef enum {
    NORMAL = 0,
    WAIT_INT_HIGH = 1
} CommandType;


// decode touch data
#pragma pack(push, 1)

typedef struct {
    uint8_t  magic;        // 0xEA
    uint8_t  type;         // 0x01 / 0x02
    uint16_t length;       // payload + packet checksum
    uint8_t  hdr_checksum; // header checksum
} touch_header_t;

typedef struct {
    uint8_t  unknown0;
    uint8_t  packet_seq;        // [1]
    uint8_t  unknown2[2];
    uint32_t scan_count;        // [4~7]
    uint8_t  unknown8[4];
    uint16_t finger_area;       // [12~13]
    uint8_t  unknown14[2];
    uint8_t  finger_count;      // [16]
    uint8_t  unknown17;
    int16_t  rel_move;          // [18~19]
    uint8_t  contact_state;     // [20]
    uint8_t  unknown21[3];
} touch_state_t;

typedef struct {
    uint8_t  finger_id;          // [0]
    uint8_t  distance_state;     // [1]
    uint8_t  unknown2[2];
    uint16_t x;                  // [4~5]
    uint16_t y;                  // [6~7]
    int16_t  vx;                 // [8~9] // 副廠不支援
    int16_t  vy;                 // [10~11] // 副廠不支援
    uint16_t range[5];           // [12~21] // 橢圓長短軸 // 副廠不支援
    uint8_t  unknown22[4];
    uint16_t pressure;           // [26~27] // 沒接3D Touch時，壓力值固定是0
    uint8_t  unknown28[2];
} touch_point_t;



typedef struct {
    uint8_t  magic;             // [0] 0xAA 起始標記
    uint8_t  finger_count;      // [1] 實際手指數量 (0-5)
    uint32_t scan_count;        // [2-5] 掃描計數器
    uint8_t  reserved[2];       // [6-7] 保留
    
    // 固定5個點的空間，沒用到的填0
    struct {
        uint8_t  finger_id;     // 手指ID (0xFF表示無效)
        uint16_t x;             // X座標
        uint16_t y;             // Y座標  
        uint8_t  contact_state; // 狀態 (0=up, 1=down, 2=move)
    } points[TOUCH_MAX_POINTS];                // [8-37] 5×6=30 bytes
    
    uint8_t  crc;               // [38] CRC校驗
    uint8_t  end_marker;        // [39] 0x55 結束標記
} spi_touch_packet_t;  // 總共 40 bytes


#pragma pack(pop)

// Public API
void ip7_touch_init(void);
void ip7_touch_poll(void);

#endif /* __IP7_TOUCH_H__ */
