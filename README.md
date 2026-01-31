# iPhone Screen Bridge

將 iPhone 7 螢幕轉換為通用顯示器的硬體驅動方案。

## 專案簡介

本專案實現了將 iPhone 7 MIPI DSI 螢幕轉換為標準 RGB 接口的完整解決方案，可透過 HDMI 或 FPGA 輸入來驅動螢幕。

## 專案結構

### `boards/iphone7/`
iPhone 7 螢幕驅動相關硬體設計

- **`fw/ip7_lcd_to_rgb/`** - STM32 韌體：將 RGB 信號轉換為 MIPI DSI (使用 SSD2828 橋接晶片)
- **`fw/hdmi_to_rgb/`** - STM32 韌體：將 HDMI 信號轉換為 RGB (使用 ADV7611 晶片)
- **`fw/edid/`** - EDID 資訊檔案，用於 HDMI 顯示器識別
- **`pcb/`** - PCB 設計檔案與互動式 BOM

### `platforms/altera_fpga/`
FPGA 平台實現

- **`rgb_fpga/`** - Altera FPGA 專案：產生測試圖案的 RGB 信號輸出

## 硬體架構

本專案使用 **兩塊 PCB** 協同工作：

### PCB1 - RGB to MIPI 轉接板 (`ip7_lcd_to_rgb`)

**功能：** 將 RGB 並列信號轉換為 iPhone 7 螢幕的 MIPI DSI 信號

**核心晶片：**
- STM32F103 微控制器
- SSD2828 MIPI DSI 橋接晶片

**接口：**
- 輸入：24-bit RGB + HSYNC + VSYNC + DEN + PCLK
- 輸出：MIPI DSI 4-lane 至 iPhone 7 螢幕 FPC 連接器
- 電源：提供 LCD 背光與觸控所需電源

**韌體：** [ip7_lcd_to_rgb](boards/iphone7/fw/ip7_lcd_to_rgb/)

**互動式 BOM：** [PCB1 BOM](boards/iphone7/pcb/ip7_lcd_to_rgb/InteractiveBOM_PCB1_2025-9-3.html)

---

### PCB2 - HDMI to RGB 轉接板 (`hdmi_rgb_adv7611`)

**功能：** 將 HDMI 數位信號解碼為 RGB 並列信號（可選板）

**核心晶片：**
- STM32F103 微控制器
- ADV7611 HDMI 接收晶片

**接口：**
- 輸入：HDMI (支援 EDID 設定)
- 輸出：24-bit RGB + HSYNC + VSYNC + DEN + PCLK
- USB：可選 USB HID 多點觸控輸出功能

**韌體：** [hdmi_to_rgb](boards/iphone7/fw/hdmi_to_rgb/)

**互動式 BOM：** [PCB2 BOM](boards/iphone7/pcb/hdmi_rgb_adv7611/InteractiveBOM_PCB2_2026-1-5.html)

---

## 使用流程

### 方案 A：HDMI 輸入（需要兩塊板）
```
HDMI 訊號源 → [PCB2: ADV7611] → RGB 信號 → [PCB1: SSD2828] → MIPI DSI → iPhone 7 螢幕
```

### 方案 B：FPGA 直接輸入（僅需一塊板）
```
FPGA RGB 輸出 → [PCB1: SSD2828] → MIPI DSI → iPhone 7 螢幕
```

### 方案 C：其他 RGB 信號源（僅需一塊板）
```
任何 RGB 信號源 → [PCB1: SSD2828] → MIPI DSI → iPhone 7 螢幕
```

**說明：**
- **PCB1 是必需的**，負責最終的 RGB → MIPI DSI 轉換
- **PCB2 是可選的**，僅在需要 HDMI 輸入時使用
- 如使用 FPGA 或其他可直接產生 RGB 信號的設備，可以跳過 PCB2

## LCD 時序參數與觸控重要說明

### iPhone 7 原廠 LCD 時序

iPhone 7 螢幕解析度為 **1334x750**，支援的時序參數如下：

| 參數 | 數值 | 說明 |
|------|------|------|
| H_SYNC_CYCLES | 3 | 水平同步周期 |
| H_BACK_PORCH | 3 | 水平後膺 |
| H_ACTIVE_VIDEO | 750 | 水平有效顯示區 |
| H_FRONT_PORCH | 40 | 水平前膺 |
| V_SYNC_CYCLES | 3 | 垂直同步周期 |
| V_BACK_PORCH | 3 | 垂直後膺 |
| V_ACTIVE_VIDEO | 1334 | 垂直有效顯示區 |
| V_FRONT_PORCH | 500 | 垂直前膺 |

**總時序：** H_Total = 796, V_Total = 1840  
**像素時鐘：** 796 × 1840 × 60Hz ≈ **87.9 MHz**

### ⚠️ 原廠 LCD 觸控問題與解決方案

#### 問題描述

**如果使用蘋果原廠 LCD，觸控功能與顯示時序存在關聯性**：

- 原廠螢幕的觸控控制器需要 **V_FRONT_PORCH ≥ 500** 才能正常工作
- 觸控訊號的掃描與垂直消隱期（V-Blanking）有時序依賴關係

#### HDMI 方案的限制

通過 HDMI 輸入時會遇到 **EDID 限制**：

- HDMI EDID 標準中，Front Porch 欄位只有 **6-bit (0-63)**
- 無法在 EDID 中設定 V_FRONT_PORCH = 500
- 因此使用 HDMI 輸入時，**原廠 LCD 的觸控功能無法正常運作**

#### 解決方案

1. **使用 FPGA 直接產生 RGB 信號**（推薦）
   - 直接依照 [LcdDriver.v](platforms/altera_fpga/rgb_fpga/LcdDriver.v) 的參數設定
   - 完全自訂時序，可設定 V_FRONT_PORCH = 500
   - **原廠 LCD 的觸控功能可正常運作**

2. **使用副廠 LCD**
   - 副廠螢幕通常沒有觸控與時序的關聯性
   - 可正常使用 HDMI 輸入
   - V_FRONT_PORCH 可設定在 EDID 範圍內（≤ 63）

### 開發建議

- 如需使用**原廠螢幕 + 觸控功能**：選擇 **FPGA 方案**
- 如僅需顯示功能：**HDMI 方案**即可
- 如使用**副廠螢幕**：兩種方案均可

## 技術細節

### FPGA 實現

參考 [LcdDriver.v](platforms/altera_fpga/rgb_fpga/LcdDriver.v) 模組：
- 產生符合 iPhone 7 時序的 HSYNC、VSYNC、DEN 信號
- 輸出 24-bit RGB 像素資料
- 支援自訂測試圖案或外部影像輸入

### STM32 韌體

- **hdmi_to_rgb**：ADV7611 控制，EDID 設定
- **ip7_lcd_to_rgb**：SSD2828 MIPI 橋接晶片初始化與控制

