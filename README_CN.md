# CoreS3 AI 桌面宠物交互系统

基于 M5Stack CoreS3 (ESP32-S3) 的 AI 桌面宠物固件，面向嵌入式比赛演示。项目聚焦于触控驱动表情交互、摄像头调试拍照、Wi-Fi 状态显示、SD 卡照片存储、IMU 番茄钟交互、摇晃触发"晕"表情以及小智 AI 占位界面。

## 当前状态

- **表情 UI**：触控表情切换、眼球视线、眨眼动画、摇晃机身触发"晕"表情
- **功能菜单**：Wi-Fi 状态、摄像头调试、番茄钟、系统信息
- **摄像头调试**：实时预览、返回按钮、拍照按钮、JPEG 保存到 SD 卡
- **番茄钟**：IMU 姿态选择预设时长、四方向屏幕旋转
- **小智 AI**：仅占位界面。真实设备验证、WebSocket、OPUS、麦克风和扬声器流式传输尚未实现
- **人脸检测**：已禁用。没有启用人脸检测和肤色检测

## 硬件要求

- M5Stack CoreS3
- MicroSD/TF 卡（FAT/FAT32 格式）
- 可选底座硬件：电池、PCA9685、舵机

本固件使用的 CoreS3 SD 卡 SPI 引脚：

```text
SCK  36
MISO 35
MOSI 37
CS   4
```

## 软件要求

- Windows 系统，已安装 PlatformIO
- USB 数据线连接 CoreS3
- Git

本项目使用本地 PlatformIO 可执行文件构建：

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe
```

## 克隆项目

```powershell
git clone https://github.com/sfj330/cores3.git
cd cores3
```

## Wi-Fi 配置

真实凭据不入 Git 仓库。

1. 复制示例文件：

```powershell
Copy-Item src\config\wifi_secrets.example.h src\config\wifi_secrets.h
```

2. 编辑 `src\config\wifi_secrets.h`：

```cpp
#pragma once
constexpr const char* WIFI_SSID = "你的WiFi名称";
constexpr const char* WIFI_PASSWORD = "你的WiFi密码";
```

`src/config/wifi_secrets.h` 已被 Git 忽略。

## 编译

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run
```

预期结果：`m5stack-cores3` 平台编译成功。

## 烧录

先尝试自动烧录：

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run -t upload
```

如果自动检测端口失败，手动指定当前已知端口：

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run -t upload --upload-port COM5
```

如果烧录无法启动，长按 RESET 键约 3 秒进入下载模式。烧录完成后按一次 RESET 运行固件。

## 串口监视

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe device monitor -p COM5 -b 115200
```

正常启动日志应包含：

- `CoreS3 AI Pet Boot`
- `FaceDetector: disabled by configuration`
- `Face detection off`
- 各任务创建日志
- Wi-Fi 连接尝试（如果已配置 Wi-Fi）
- SD 卡探测日志，如 `SD try 25 MHz`

## 操作说明

- **表情页面**：
  - 右滑：打开功能菜单
  - 左滑或双击：打开小智 AI 占位界面
  - 点击上方：开心
  - 点击下方：害羞
  - 点击左/右：好奇 + 视线方向
  - 摇晃机身："晕"表情约 2 秒
  - 长按：休眠
- **AI 页面**：
  - 显示小智占位界面和设备验证提示
  - 右滑或长按：返回表情页面
- **功能菜单**：
  - 点击当前卡片进入对应功能
  - 右滑切换卡片
  - 返回按钮回到表情页面
- **摄像头调试**：
  - 点击 SHOT 保存 `/photos/IMG_####.jpg` 到 SD 卡
  - 返回按钮或左滑回到菜单
- **番茄钟**：
  - 旋转设备选择计时预设，界面同步旋转
  - 开始/暂停和重置按钮控制计时

## 系统架构

```
src/
├── main.cpp                       // 入口、任务创建、事件分发
├── app/
│   ├── app_state.h/.cpp           // 主状态机
│   ├── gesture_manager.h/.cpp     // 手势识别（单击/双击/长按/滑动）
│   └── event_bus.h/.cpp           // 模块间事件总线
├── ui/
│   ├── face_ui.h/.cpp             // 表情渲染（8 种表情 + 眼球视线）
│   ├── menu_ui.h/.cpp             // 右滑功能菜单
│   ├── camera_debug_ui.h/.cpp     // 摄像头调试页面
│   └── pomodoro_ui.h/.cpp         // 番茄钟页面
├── vision/
│   ├── camera_manager.h/.cpp      // 摄像头采集管理
│   ├── face_detector.h/.cpp       // 人脸检测接口
│   ├── face_tracker.h/.cpp        // 人脸中心平滑追踪
│   └── imu_orientation.h/.cpp     // IMU 姿态识别
├── ai/
│   ├── xiaozhi_client.h/.cpp      // 小智 AI 客户端（占位）
│   └── voice_state.h              // 语音状态定义
├── network/
│   └── wifi_manager.h/.cpp        // Wi-Fi 连接管理
├── storage/
│   └── storage_manager.h/.cpp     // SD 卡照片存储
├── power/
│   └── power_manager.h/.cpp       // 电池/电源管理
└── config/
    └── app_config.h               // 全局参数配置
```

### FreeRTOS 任务

| 任务 | 功能 | 频率 |
|------|------|------|
| UI | 表情/菜单/摄像头/番茄钟渲染 | 20 FPS |
| Touch | 触控点读取 + 手势识别 | 50 Hz |
| Camera | 摄像头帧采集 | 15 FPS |
| Vision | 人脸检测 / 视线追踪 | 5 FPS |
| AI | 小智 AI 交互处理 | 20 Hz |
| Power | 电池电压 / 低电量保护 | 1 Hz |
| Network | Wi-Fi 状态更新 | 5 秒间隔 |

### 状态机

```
FACE ←→ MENU ←→ CAMERA_DEBUG
  │
  ├── AI
  ├── POMODORO
  └── SLEEP
```

### 手势优先级

双击 > 右滑/左滑 > 长按 > 单击

- 双击窗口：250-350ms
- 长按阈值：800ms+

## 关于小智 AI

本固件不生成真实的小智验证码。在官方小智流程中，设备固件连接小智服务后会显示或播报一个验证码，用户在控制台"添加设备"流程中输入该码完成绑定。

参考资料：

- [M5Stack CoreS3 XiaoZhi 指南](https://docs.m5stack.switch-science.com/en/guide/realtime/xiaozhi/m5cores3)
- [XiaoZhi WebSocket 协议](https://home.xiaozhi.me/xz-docs/docs/tutorial-comm/websocket-comm/)

未来真实接入需要：OTA 激活、WebSocket 协议处理、OPUS 音频编解码、麦克风采集、扬声器播放。

## 关于人脸检测

人脸检测在当前 Arduino/PlatformIO 构建中有意禁用。之前的肤色检测方案会产生误检，不能代表真实的人脸识别效果，因此不再使用。

未来真实人脸检测应使用经过验证的 ESP-DL/ESP-WHO 集成方案，建议通过 ESP-IDF 或 Arduino-as-IDF-component 方式实现。

## 开发路线

| 阶段 | 内容 |
|------|------|
| 1 | UI 原型 — 表情、触控手势、右滑菜单（无摄像头/AI） |
| 2 | 外设验证 — 屏幕、触摸、扬声器、麦克风、Wi-Fi、摄像头调试 |
| 3 | 背景视觉 — 人脸检测 + 视线追踪（摄像头隐藏在表情 UI 后方） |
| 4 | 小智 AI — 双击进入语音交互，AI 回复时同步表情 |
| 5 | 舵机追踪 — PCA9685 + 2 轴舵机与眼球视线同步 |
| 6 | 番茄钟完善 & 比赛封装 |

## 故障排查

- **烧录失败或循环重试**：长按 RESET 约 3 秒后重新烧录，烧录完成后按一次 RESET
- **SD 卡未识别**：格式化为 FAT/FAT32，重新插入，检查串口日志中的 `SD.begin failed`、`CARD_NONE`、`Root open failed` 或 `Probe write failed`
- **无 Wi-Fi**：检查 `src/config/wifi_secrets.h` 中的 SSID、密码和信号强度
- **摄像头预览黑屏或闪烁**：系统完全启动后再进入摄像头调试，检查串口日志确认 PSRAM 已识别

## 技术栈

- **平台**：ESP32-S3 (M5Stack CoreS3)
- **框架**：Arduino + FreeRTOS
- **构建工具**：PlatformIO
- **核心库**：M5CoreS3、M5Unified、M5GFX
- **编程语言**：C++17
