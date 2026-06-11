# ESP32-S3 外设资源利用文档

> CoreS3 AI Pet Firmware — 外设利用完整统计

## 📊 总体统计

| 统计项 | 数量 | 说明 |
|--------|------|------|
| **GPIO 引脚** | **20** | 相机 17 个 + SD 卡 3 个 |
| **I2C 总线** | **2** | I2C_NUM_0 (内部) + I2C_NUM_1 (相机 SCCB) |
| **I2C 外设地址** | **1** | PCA9685 (0x40) 舵机驱动 |
| **SPI 设备** | **2** | SD 卡 + LCD |
| **I2S 通道** | **2** | 麦克风 + 扬声器（半双工） |
| **ADC 通道** | **1** | GPIO1 环境光传感器 |
| **LEDC PWM** | **1** | 相机 XCLK 时钟 |
| **其他外设** | 7 | WiFi, RTC, DMA, NVS, RNG, PCNT, 触摸屏 |
| **FreeRTOS 任务** | **7** | UI, Touch, Camera, Vision, AI, Power, Network |
| **代码规模** | ~12000 | C++ 代码行数 |

---

## 🔌 GPIO 引脚详细清单（20 个）

### 相机接口（17 个 GPIO）

| GPIO | 功能 | 说明 |
|------|------|------|
| **11** | SCCB SCL | 相机 I2C 时钟（I2C_NUM_1） |
| **12** | SCCB SDA | 相机 I2C 数据（I2C_NUM_1） |
| **39** | CAM_D0 | 相机数据线 0 |
| **40** | CAM_D1 | 相机数据线 1 |
| **41** | CAM_D2 | 相机数据线 2 |
| **42** | CAM_D3 | 相机数据线 3 |
| **15** | CAM_D4 | 相机数据线 4 |
| **16** | CAM_D5 | 相机数据线 5 |
| **47** | CAM_D6 | 相机数据线 6 |
| **48** | CAM_D7 | 相机数据线 7 |
| **45** | CAM_PCLK | 相机像素时钟（行时钟） |
| **46** | CAM_VSYNC | 相机垂直同步（场信号） |
| **38** | CAM_HREF | 相机行参考信号 |

### SD 卡 SPI 接口（3 个 GPIO）

| GPIO | 功能 | 说明 |
|------|------|------|
| **36** | SPI_SCK | SD 卡时钟 |
| **37** | SPI_MOSI | SD 卡数据输出 |
| **35** | SPI_MISO | SD 卡数据输入 |
| **4** | SPI_CS | SD 卡片选 |

### 其他 GPIO

| GPIO | 功能 | 说明 |
|------|------|------|
| **1** | ADC1_CH0 | 环境光传感器 (PortA) |
| **8** | PCNT_IN | 脉冲计数器输入 (PortB) |
| **9** | PCNT_CTRL | 脉冲计数器控制 (PortB) |

**相机配置代码** ([src/vision/camera_manager.cpp](src/vision/camera_manager.cpp))：
```cpp
camera_config_t camera_config = {
    .pin_pwdn = -1,
    .pin_reset = -1,
    .pin_xclk = -1,           // 内部 LEDC 生成 XCLK
    .pin_sscb_sda = 12,       // I2C SDA
    .pin_sccb_scl = 11,       // I2C SCL
    .pin_d7 = 48,  .pin_d6 = 47, .pin_d5 = 16, .pin_d4 = 15,
    .pin_d3 = 42,  .pin_d2 = 41, .pin_d1 = 40, .pin_d0 = 39,
    .pin_vsync = 46,          // VSYNC
    .pin_href = 38,           // HREF
    .pin_pclk = 45,           // PCLK
    .xclk_freq_hz = 10000000, // 10 MHz XCLK
};
```

---

## 🔗 I2C 总线配置（2 条总线）

### I2C_NUM_0（内部总线 / PortA I2C）

**状态**：相机初始化时**释放**

| 地址 | 芯片 | 用途 | 备注 |
|------|------|------|------|
| 0x68 | BMI270 | IMU（加速度+陀螺仪） | 姿态检测、摇晃识别 |
| 0x77 | AXP2101 | 电源管理 IC | 电池电压读取、低电量检测 |
| - | FT5x06 | 触摸屏控制器 | M5Unified 管理 |
| - | LCD 控制器 | 显示驱动 | M5Unified 管理 |

**关键代码** ([src/main.cpp](src/main.cpp))：
```cpp
// 相机初始化时释放内部 I2C，避免竞争
m5gfx::M5.In_I2C.release();        // 行号 ~3561
// ... 相机初始化 ...
// 相机关闭时恢复 I2C
m5gfx::M5.In_I2C.begin();          // 行号 ~103-104
```

### I2C_NUM_1（相机 SCCB 接口 / PortA 扩展）

**速率**：100 kHz（相机标准）

| 地址 | 芯片 | 用途 | 代码位置 |
|------|------|------|---------|
| 0x30 | 相机传感器 | 图像参数配置 | [src/vision/camera_manager.cpp:53](src/vision/camera_manager.cpp) |

**初始化代码** ([src/vision/camera_manager.cpp](src/vision/camera_manager.cpp))：
```cpp
s_config.sccb_i2c_port = I2C_NUM_1;  // 使用 I2C_NUM_1
s_config.pin_sccb_sda = 12;          // GPIO12
s_config.pin_sccb_scl = 11;          // GPIO11
```

---

### ⚠️ I2C 竞争处理

**问题**：CoreS3 内部 I2C 与相机 SCCB 共享一条总线，可能导致初始化失败

**解决方案** ([src/vision/camera_manager.cpp](src/vision/camera_manager.cpp))：

1. **触摸任务暂停**（避免读取触摸数据）
   ```cpp
   vTaskSuspend(xTaskTouchHandle);    // 行号 ~18
   ```

2. **释放内部 I2C**（Release 总线）
   ```cpp
   m5gfx::M5.In_I2C.release();        // 行号 ~24
   ```

3. **初始化相机**
   ```cpp
   esp_camera_init(&s_config);        // 行号 ~56
   ```

4. **恢复 I2C 和触摸**
   ```cpp
   m5gfx::M5.In_I2C.begin();          // 行号 ~103
   vTaskResume(xTaskTouchHandle);     // 行号 ~104
   ```

5. **多速率重试**（失败时）
   ```cpp
   // 25 MHz → 10 MHz → 4 MHz → 1 MHz
   ```

---

## 🎤 I2S 音频接口配置

### 麦克风通道（采集）

| 参数 | 值 | 说明 |
|------|-----|------|
| **采样率** | 16 kHz | XiaoZhi Opus 标准 |
| **位深度** | 16-bit | PCM 格式 |
| **声道** | 单声道 | Mono |
| **缓冲数** | 8 | DMA 缓冲块数 |
| **缓冲长度** | 256 样本 | 每块 256 样本 |
| **内存类型** | MALLOC_CAP_INTERNAL | RAM 必须（DMA 稳定性） |
| **帧时长** | 60 ms | Opus 编码帧 |
| **每帧样本数** | 960 | 16000 Hz × 60 ms |
| **目标比特率** | 24 kbps | Opus 编码参数 |

**代码位置** ([src/ai/xiaozhi_client.cpp](src/ai/xiaozhi_client.cpp))：
```cpp
// 麦克风启动
i2s_read(I2S_NUM_0, mic_buffer, frame_size);  // 行号 ~1328

// 麦克风缓冲分配
void* mic_buf = heap_caps_malloc(size, MALLOC_CAP_INTERNAL);  // 内部 RAM
```

### 扬声器通道（播放）

| 参数 | 值 | 说明 |
|------|-----|------|
| **采样率** | 16-24 kHz | 服务器返回速率 |
| **位深度** | 16-bit | PCM 格式 |
| **声道** | 单声道 | 转换为 Mono |
| **缓冲数** | 8 | DMA 缓冲块数 |
| **缓冲长度** | 256 样本 | 每块 256 样本 |
| **内存类型** | MALLOC_CAP_SPIRAM | PSRAM（可用性强） |
| **模式** | 半双工 | 不可同时采集和播放 |

**代码位置** ([src/ai/xiaozhi_client.cpp](src/ai/xiaozhi_client.cpp))：
```cpp
// 扬声器启动（DMA 配置）
i2s_write(I2S_NUM_0, speaker_buffer, write_len);  // 行号 ~1199

// 播放缓冲分配
void* speaker_buf = heap_caps_malloc(size, MALLOC_CAP_SPIRAM);  // PSRAM
```

### 音乐播放器配置 ([src/audio/music_manager.cpp](src/audio/music_manager.cpp))

| 参数 | 值 |
|------|-----|
| 采样率 | 44.1 kHz (MP3) / 16 kHz (WAV) |
| 混音模式 | Stereo → Mono（M5Unified） |
| DMA 缓冲 | 8 块 × 256 样本 |
| 格式 | MP3 / WAV |

---

## 💾 SPI 设备配置

### SPI #0 - SD 卡（FSPI）

| 参数 | 值 | 说明 |
|------|-----|------|
| **CS 引脚** | GPIO 4 | 片选 |
| **SCK** | GPIO 36 | 时钟 |
| **MOSI** | GPIO 37 | 主输出从输入 |
| **MISO** | GPIO 35 | 主输入从输出 |
| **频率** | 25 MHz（主频） | 初始速率 |
| **降速重试** | 10 / 4 / 1 MHz | 容错序列 |
| **用途** | 照片存储、音乐读取 | 所有文件 I/O |

**代码位置** ([src/storage/storage_manager.cpp](src/storage/storage_manager.cpp))：
```cpp
// SPI 初始化
spi_bus_config_t bus_cfg = {
    .mosi_io_num = 37,  // GPIO37
    .miso_io_num = 35,  // GPIO35
    .sclk_io_num = 36,  // GPIO36
};
```

### SPI #1 - LCD 显示屏

**管理者**：M5Unified / M5GFX 库

| 参数 | 值 |
|------|-----|
| **控制器** | ILI9341 |
| **分辨率** | 320 × 240 px |
| **色深** | 16-bit RGB565 |
| **内存** | PSRAM 中 M5Canvas |
| **用途** | 所有 UI 渲染 |

---

## ⚡ 其他外设详情

### ADC (12-bit)

| 参数 | 值 | 说明 |
|------|-----|------|
| **通道** | ADC1_CH0 | |
| **引脚** | GPIO 1 | PortA |
| **用途** | 环境光检测 | 自动亮度调节 |
| **采样周期** | 1 Hz | Power 任务间隔 |
| **亮度阈值** | 800 / 200 | 亮 / 暗 |

**代码位置** ([src/power/power_manager.cpp](src/power/power_manager.cpp))：
```cpp
int light = analogRead(1);  // GPIO1，行号 ~78
```

### LEDC PWM（LED PWM Controller）

| 参数 | 值 | 说明 |
|------|-----|------|
| **定时器** | LEDC_TIMER_0 | |
| **通道** | LEDC_CHANNEL_0 | |
| **用途** | 相机 XCLK 时钟 | 10 MHz |
| **频率** | 10 MHz | 必须固定频率 |

**代码位置** ([src/vision/camera_manager.cpp](src/vision/camera_manager.cpp))：
```cpp
ledc_timer_config_t ledc_timer = {
    .speed_mode = LEDC_HIGH_SPEED_MODE,
    .timer_num = LEDC_TIMER_0,
    .duty_resolution = LEDC_TIMER_1_BIT,
    .freq_hz = 10000000,  // 10 MHz，行号 ~45
};
```

### RTC (实时时钟) - 深度睡眠

| 功能 | 参数 | 说明 |
|------|-----|------|
| **定时唤醒** | 60 秒 | 低电量保护睡眠 |
| **触发条件** | 电压 < 3.3V | 棕色复位安全模式 |
| **代码位置** | [src/power/power_manager.cpp](src/power/power_manager.cpp):85 | `esp_sleep_enable_timer_wakeup()` |

### WiFi (802.11 b/g/n)

| 参数 | 值 | 说明 |
|------|-----|------|
| **协议** | WiFi 6 (802.11 a/b/g/n/ac/ax) | |
| **频段** | 2.4 GHz | |
| **功能** | STA 模式 | 连接到 WiFi 路由器 |
| **用途** | XiaoZhi AI、OTA 升级、NTP 时同 | |
| **自动重连** | 5 秒间隔 | WiFi Manager |
| **安全性** | WPA2-PSK / WPA3 | |

**代码位置** ([src/network/wifi_manager.cpp](src/network/wifi_manager.cpp))：
```cpp
// WiFi 初始化
WiFi.mode(WIFI_STA);        // 行号 ~18
WiFi.begin(SSID, PASSWORD); // 行号 ~19
```

### NVS Flash（非易失性存储）

| 数据项 | 大小 | 用途 |
|--------|------|------|
| **亲密度分数** | 4 bytes | Bond 值持久化 |
| **心情状态** | 1 byte | 宠物情绪 |
| **备忘录条目** | ~480 bytes | 8 项 × 60 字符 |
| **设置参数** | ~50 bytes | 用户偏好 |

**代码位置** ([src/app/affinity_manager.cpp](src/app/affinity_manager.cpp))：
```cpp
Preferences prefs;
prefs.begin("app");
affinity_score = prefs.getUShort("affinity", 0);  // 行号 ~9
```

### DMA (GDMA - General DMA)

| 用途 | 配置 | 说明 |
|------|------|------|
| **I2S 音频** | 8 缓冲块 | 麦克风采集 / 扬声器播放 |
| **SPI 传输** | DMA 链表 | SD 卡 / LCD 刷屏 |
| **缓冲长度** | 256 样本 | 每块 DMA 描述符 |

**代码位置** ([src/ai/xiaozhi_client.cpp](src/ai/xiaozhi_client.cpp))：
```cpp
dma_buf_count = 8;          // 行号 ~1136
dma_buf_len = 256;          // 行号 ~1137
```

### RNG (随机数生成器)

**用途**：表情动画随机效果

| 效果 | 代码位置 |
|------|---------|
| 眨眼间隔随机 | [src/ui/face_ui.cpp:84](src/ui/face_ui.cpp) |
| 嘴巴张合时机 | [src/ui/face_ui.cpp:112](src/ui/face_ui.cpp) |
| 粒子效果随机 | [src/ui/face_ui.cpp:432-474](src/ui/face_ui.cpp) |
| 陪伴动作间隔 | [src/main.cpp:845](src/main.cpp) |

### PCNT (脉冲计数器)

| 参数 | 值 | 说明 |
|------|-----|------|
| **单元** | PCNT_UNIT_0 | |
| **输入引脚** | GPIO 8 (PortB) | 脉冲输入 |
| **控制引脚** | GPIO 9 (PortB) | 控制信号 |
| **用途** | 外部事件计数 | PortB 拓展接口 |

---

## 📋 FreeRTOS 任务配置

| 任务 | 优先级 | 核心 | 周期 | 用途 |
|------|--------|------|------|------|
| **UI** | 2 | 0 | 20 FPS (50ms) | 屏幕渲染、动画 |
| **Touch** | 3 | 0 | 50 Hz (20ms) | 触摸输入、手势识别 |
| **Camera** | 2 | 0 | 15 FPS (67ms) | 相机帧采集 |
| **Vision** | 1 | 0 | 5 FPS (200ms) | 人脸检测、追踪 |
| **AI** | 2 | 0 | 50 Hz (20ms) | XiaoZhi WebSocket、音频处理 |
| **Power** | 1 | 1 | 1 Hz (1000ms) | 电池监测、ADC 采样 |
| **Network** | - | 1 | 5s 间隔 | WiFi 重连、菜单状态更新 |
| **Music** | 5 | - | 事件驱动 | MP3/WAV 流式播放 |

**代码位置** ([src/main.cpp](src/main.cpp))：
```cpp
xTaskCreatePinnedToCore(ui_task, "UI", 8192, NULL, 2, &xTaskUIHandle, 0);
xTaskCreatePinnedToCore(touch_task, "Touch", 4096, NULL, 3, &xTaskTouchHandle, 0);
// ... 更多任务 ...
```

---

## 💾 内存配置

| 内存类型 | 大小 | 用途 |
|---------|------|------|
| **SRAM** | ~520 KB | 任务栈、代码执行 |
| **PSRAM** | 8 MB | 帧缓冲、音频播放缓冲、堆数据 |
| **NVS** | ~16 KB | 持久化数据（亲密度、备忘录） |

**分配策略**：
- **麦克风缓冲**：`MALLOC_CAP_INTERNAL` （DMA 稳定性）
- **Opus 编码**：`MALLOC_CAP_INTERNAL` （I2S DMA 要求）
- **播放缓冲**：`MALLOC_CAP_SPIRAM` （充足空间）
- **UI Canvas**：`MALLOC_CAP_SPIRAM` （大尺寸缓冲）

---

## 🔧 关键配置文件

| 文件 | 用途 |
|------|------|
| [src/config/app_config.h](src/config/app_config.h) | 所有硬件引脚和常量定义 |
| [src/vision/camera_manager.cpp](src/vision/camera_manager.cpp) | 相机 GPIO、I2C、LEDC 配置 |
| [src/servo/servo_controller.cpp](src/servo/servo_controller.cpp) | PCA9685 I2C 舵机驱动 |
| [src/audio/music_manager.cpp](src/audio/music_manager.cpp) | I2S 音乐播放、DMA 配置 |
| [src/ai/xiaozhi_client.cpp](src/ai/xiaozhi_client.cpp) | I2S 麦克风、Opus 编码、WebSocket |
| [src/storage/storage_manager.cpp](src/storage/storage_manager.cpp) | SPI 和 SD 卡配置 |
| [src/network/wifi_manager.cpp](src/network/wifi_manager.cpp) | WiFi 初始化和管理 |
| [src/power/power_manager.cpp](src/power/power_manager.cpp) | ADC、RTC、低电量处理 |
| [src/main.cpp](src/main.cpp) | 集成初始化、任务循环、中断处理 |

---

## ⚠️ 间接使用（驱动层内部）

| 外设 | 用途 | 管理者 |
|------|------|--------|
| **LCD_CAM (DVP)** | 相机 8-bit 并行接口 | esp_camera 驱动 |
| **Crypto/AES** | TLS/WSS 加密 | mbedtls (WiFiClientSecure) |
| **GPIO 背光 PWM** | LCD 亮度调节 | M5Unified (setBrightness) |
| **AXP2101** | PMU / 电池电压 | M5Unified (Power 模块) |
| **FT5x06** | 触摸控制器 | M5Unified (Touch 模块) |
| **BMI270** | IMU 加速度/陀螺仪 | M5Unified / 手势识别 |

---

## ❌ 未使用外设

| 外设 | 理由 | 潜在用途 |
|------|------|----------|
| **UART0/1** | USB 调试占用 UART0 | GPS、串口传感器 |
| **MCPWM** | 使用 PCA9685 PWM | 直接驱动 DC 电机 |
| **TWAI (CAN)** | 不需要 CAN 通信 | 机器人底盘 CAN 通信 |
| **USB OTG** | 不需要 USB 主设备 | USB 外设、键盘鼠标 |
| **蓝牙 / BLE** | 使用 WiFi 替代 | BLE 手机配对 |
| **以太网** | ESP32-S3 无集成 PHY | （不可用） |

---

## 📍 功能-外设映射速查表

| 用户功能 | 涉及外设 | 模块 |
|----------|---------|------|
| **宠物表情动画** | SPI(LCD), RNG, ADC | face_ui.cpp |
| **触摸交互** | I2C#1(触摸), I2C#1(IMU), FreeRTOS | gesture_manager.cpp |
| **AI 语音对话** | WiFi, I2S(Mic+Speaker), DMA, NVS, Crypto | xiaozhi_client.cpp |
| **音乐播放** | SPI(SD), I2S(Speaker), DMA, RNG | music_manager.cpp |
| **相机拍照** | I2C#1(SCCB), LEDC(XCLK), LCD_CAM(DVP), SPI(SD) | camera_manager.cpp |
| **舵机控制** | I2C#0(PCA9685@0x40) | servo_controller.cpp |
| **备忘录存储** | NVS(Flash), RTC(NTP) | memo_manager.cpp |
| **亲密度系统** | NVS(Flash), RNG, I2C#1(IMU) | affinity_manager.cpp |
| **环境光检测** | ADC(GPIO1) | power_manager.cpp |
| **低电量保护** | I2C#1(PMU), RTC(深度睡眠) | power_manager.cpp |
| **网络通信** | WiFi, Crypto(TLS) | wifi_manager.cpp |
| **系统信息显示** | SPI(LCD), ADC, I2C#1(PMU) | info_ui.cpp |

---

## 📈 性能指标

| 指标 | 值 | 说明 |
|------|-----|------|
| **相机帧率** | 15 FPS | DVP 直连采集 |
| **UI 刷新率** | 20 FPS | 流畅动画 |
| **音频采样率** | 16 kHz | Opus 编码优化 |
| **音频延迟** | ~100-200 ms | 半双工、网络延迟 |
| **WiFi 重连时间** | ~3-5 s | 自动管理 |
| **I2C 频率** | 100 kHz | 标准速率 |
| **SPI 频率** | 25 MHz (初始) | 支持降速容错 |
| **任务切换延迟** | <1 ms | FreeRTOS 调度 |

---

## 📝 设计原则

✅ **已应用的优化**：
- I2C 竞争解决方案（释放 + 暂停触摸）
- PSRAM 用于缓冲，INTERNAL RAM 用于 DMA
- 半双工音频避免回声干扰
- 多速率 SD 卡降速重试
- 低电量自动睡眠保护
- 棕色复位安全模式（60s 不捕获相机）

❌ **未实现的功能**：
- ~~实时 ML 面部识别~~ （仅使用皮肤色启发式）
- ~~PID 舵机反馈控制~~ （开环设计）
- ~~自主基础移动~~ （不支持）
- ~~全双工音频~~ （半双工）
