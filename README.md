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

## 使用流程

```
HDMI 輸入 → ADV7611 → RGB → SSD2828 → MIPI DSI → iPhone 7 螢幕
    或
FPGA 輸出 → RGB → SSD2828 → MIPI DSI → iPhone 7 螢幕
```