# USB HID Multi-Touch 關鍵問題解決

## 問題
STM32F103 USB HID 觸控螢幕：**單點正常，多點完全失效**

## 根本原因
**缺少 Contact Count Maximum Feature Report**

Windows 初始化觸控設備時會發送 `Get_Report(Feature)` 請求查詢設備最大觸控點數。
- ❌ 沒有 Feature Report → Windows 只處理第一個觸控點
- ✅ 有 Feature Report → Windows 根據回傳值啟用多點觸控

## 解決方案（兩步驟）

### 1. 在 HID Descriptor 加入 Feature Report

在 `usbd_hid.c` 的 `HID_TOUCH_ReportDesc[]` **最後的 `0xC0` 之前**加入：

```c
// Contact Count Maximum Feature Report
0x05, 0x0D,        //   Usage Page (Digitizers)
0x09, 0x55,        //   Usage (Contact Count Maximum)
0x25, 0x7F,        //   Logical Maximum (127)
0x75, 0x08,        //   Report Size (8)
0x95, 0x01,        //   Report Count (1)
0xB1, 0x02,        //   Feature (Data,Var,Abs)

0xC0               // End Collection
```

定義 Feature Report 資料：

```c
// Feature Report: Contact Count Maximum
#define MAX_CONTACT_COUNT 5  // 支援 5 個手指
__ALIGN_BEGIN static uint8_t HID_Feature_Report[1] __ALIGN_END = {
    MAX_CONTACT_COUNT
};
```

### 2. 處理 GET_REPORT 請求

在 `USBD_HID_Setup()` 的 `USB_REQ_TYPE_CLASS` case 中加入：

```c
case HID_REQ_GET_REPORT:
    // Windows 查詢 Contact Count Maximum
    if ((req->wValue >> 8) == 0x03)  // Feature Report (type=3)
    {
        USBD_CtlSendData(pdev, (uint8_t *)HID_Feature_Report, 
                         sizeof(HID_Feature_Report));
    }
    else
    {
        USBD_CtlError(pdev, req);
        ret = USBD_FAIL;
    }
    break;
```

## 關鍵知識

### Feature Report 的作用
- **不會自動傳送**：Host 主動用 `Get_Report(Feature)` 請求
- **設備能力宣告**：告訴 Windows 支援幾個觸控點
- **初始化查詢**：設備枚舉時 Windows 會讀取一次

### Report Type
- `0x01` = Input (Device → Host, 觸控資料)
- `0x02` = Output (Host → Device, 控制命令)  
- `0x03` = Feature (雙向, 設備能力查詢) ⭐

## 參考資料
ST Community: https://community.st.com/t5/stm32-mcus-products/implement-multi-touch-usb-hid-for-windows-7/td-p/492255

關鍵引述：
> "Maybe, you don't respond to the 'Contact Count Maximum' query over Get_Report(Feature) Request."

---

**總結**：加入 8 行 descriptor + 處理 `HID_REQ_GET_REPORT` = 解決多點觸控問題
