# CoreS3 AI 桌面宠物交互系统

这是一个基于 M5Stack CoreS3 的嵌入式比赛演示固件。项目包含表情桌宠、触摸手势、Wi-Fi、Camera Debug、SD 拍照路径、IMU 番茄钟、摇晃难受表情，以及已经接通的小智 AI 语音助手链路。

## 当前状态

- 表情界面：触摸切换表情、眨眼、眼球视线、摇晃机身触发 SICK 难受表情。
- 功能菜单：显示 Wi-Fi 状态、Camera Debug、番茄钟、系统信息。
- Camera Debug：预览、Back、SHOT 拍照按钮，SD 可用时保存 JPEG。
- 番茄钟：通过 IMU 选择预设，并支持四方向屏幕旋转。
- 小智 AI：已实现 OTA 激活请求、WebSocket、Opus 麦克风上传、Opus TTS 播放、最小 MCP 握手。
- 人脸检测：已关闭，不启用假检测或肤色检测。

## 硬件

- M5Stack CoreS3
- FAT/FAT32 格式 MicroSD/TF 卡，用于 Camera Debug 拍照保存
- 可选底座硬件：电池、PCA9685、舵机

CoreS3 SD 卡 SPI 引脚：

```text
SCK  36
MISO 35
MOSI 37
CS   4
```

## 软件环境

- Windows
- PlatformIO
- Git
- USB 数据线连接 CoreS3

本项目当前使用的 PlatformIO 路径：

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe
```

## 下载项目

```powershell
git clone https://github.com/sfj330/cores3.git
cd cores3
```

## Wi-Fi 配置

真实 Wi-Fi 密码不进入 Git。

```powershell
Copy-Item src\config\wifi_secrets.example.h src\config\wifi_secrets.h
```

编辑 `src\config\wifi_secrets.h`：

```cpp
#pragma once
constexpr const char* WIFI_SSID = "你的WiFi名称";
constexpr const char* WIFI_PASSWORD = "你的WiFi密码";
```

`src/config/wifi_secrets.h` 已被 `.gitignore` 忽略。

## 编译

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run
```

当前小智 AI 版本依赖：

- `links2004/WebSockets`
- `sh123/esp32_opus`
- `bblanchon/ArduinoJson`
- `M5CoreS3`、`M5Unified`、`M5GFX`

## 烧录

自动烧录：

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run -t upload
```

如果自动端口检测失败，可指定当前端口：

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe run -t upload --upload-port COM5
```

如果烧录无法开始，长按 RESET 约 3 秒进入下载模式。烧录完成后，再按一次 RESET 运行固件。

## 串口监视

```powershell
C:\Users\shenf\.platformio\penv\Scripts\platformio.exe device monitor -p COM5 -b 115200
```

小智 AI 正常日志应包含：

```text
AI task: requesting activation code...
XiaoZhi: OTA POST identity=m5stack-core-s3/2.2.6
XiaoZhi: device activated, got WS config
XiaoZhi: WS connected
XiaoZhi: session_id=...
XiaoZhi: MCP method=initialize id=1
XiaoZhi: MCP method=tools/list id=2
XiaoZhi: sent listen start
XiaoZhi: WS text: {"type":"stt",...}
XiaoZhi: WS text: {"type":"tts",...}
```

## 操作方式

- 表情页：
  - 右滑：进入菜单
  - 左滑或双击：进入小智 AI
  - 点击上方：HAPPY
  - 点击下方：SHY
  - 点击左/右：CURIOUS 并改变视线
  - 摇晃机身：SICK 难受表情约 2 秒
  - 长按：休眠
- AI 页：
  - Wi-Fi 可用后连接小智服务
  - 单击切换监听状态
  - 右滑或长按返回表情页
- 菜单页：
  - 点击当前卡片进入对应功能
  - 右滑切换卡片
  - Back 返回表情页
- Camera Debug：
  - SHOT 在 SD 可用时保存 `/photos/IMG_####.jpg`
  - Back 或左滑返回菜单
- 番茄钟：
  - 旋转设备选择计时预设并旋转界面
  - Start/Pause 和 Reset 控制计时器

## 小智 AI 是怎么做的

核心代码在 `src/ai/xiaozhi_client.h` 和 `src/ai/xiaozhi_client.cpp`。

整体流程：

1. 在表情页左滑或双击进入 AI 页。
2. AI 任务确认 Wi-Fi 已连接后，向小智 OTA 接口发起激活/配置请求。
3. OTA 请求带上 CoreS3 身份信息，包括设备 MAC、生成的 Client UUID、固件身份 `m5stack-core-s3/2.2.6`、芯片信息、Flash 大小和应用版本。
4. OTA 响应返回 WebSocket URL 和 token。本项目不在本地伪造验证码。
5. 设备建立 TLS WebSocket，并发送以下握手头：
   - `Authorization: Bearer <token>`
   - `Protocol-Version`
   - `Device-Id`
   - `Client-Id`
6. 设备发送 `hello` JSON：
   - transport 为 `websocket`
   - 音频格式为 `opus`
   - 麦克风输入为 16 kHz 单声道
   - 每帧 60 ms
   - 开启 MCP 能力
7. 服务端返回 session id 和 TTS 音频参数，目前观察到服务端 TTS 为 24 kHz Opus。
8. 设备完成 MCP 最小握手：
   - `initialize`：返回协议版本、空工具能力和设备信息。
   - `tools/list`：返回空工具列表。
   - `tools/call`：明确返回未实现错误，不伪造设备工具。
9. MCP 完成后设备发送 `listen start`。
10. 麦克风采集 16 kHz PCM，编码为 Opus，通过 WebSocket binary frame 上传。
11. 服务端返回的 binary Opus 音频被解码为 PCM，再通过 CoreS3 扬声器播放。

CoreS3 的麦克风和扬声器不能同时运行，所以当前实现是半双工：

- 监听时：关闭扬声器，打开麦克风，上传 Opus 音频。
- 说话时：关闭麦克风，打开扬声器，播放服务端 TTS。
- 收到 `tts stop` 后，如果仍处于 AI 监听模式，就重新发送 `listen start`。

音频细节：

- Opus 编码：16 kHz 单声道，60 ms 帧，目标码率 24 kbps。
- Opus 解码：按服务端 hello 返回的采样率创建 decoder，目前是 24 kHz。
- 扬声器输出：M5Unified Speaker，48 kHz I2S 输出，三缓冲内部 RAM 播放，不再每帧打断当前声音。
- AudioCap 任务栈已增大，避免 Opus 编码导致栈溢出。

## 已知限制

- 小智语音链路已接通，但还没有暴露自定义 MCP 设备工具。
- 音乐或长 TTS 播放效果取决于小智服务端和 CoreS3 内置扬声器能力。
- CoreS3 内置麦克风/扬声器是半双工，当前没有实现全双工打断或回声消除。
- SD 卡识别仍受卡格式、接触和供电影响；固件不应因无卡重启，但拍照保存需要可用 FAT/FAT32 卡。
- 人脸检测仍然关闭。

## 人脸检测说明

当前 Arduino/PlatformIO 构建不启用真实人脸检测。之前的肤色检测方案可能误检，不能代表真实人脸识别效果，因此已经停用。

未来如果要做真实人脸检测，应优先验证 ESP-DL/ESP-WHO 与 ESP-IDF 或 Arduino-as-IDF-component 的集成路径。

## 故障排查

- 烧录失败或重启循环：长按 RESET 约 3 秒后重新烧录，烧录完成后按一次 RESET。
- 小智 OTA 返回 `400`：检查 `XIAOZHI_BOARD_TYPE`、`XIAOZHI_FIRMWARE_VERSION` 和串口里的 OTA identity。
- 小智 WebSocket 在 MCP 后断开：检查串口里是否先出现 `initialize`、`tools/list`，再出现 `sent listen start`。
- 没有 AI 回复：确认 Wi-Fi 已连接，进入 AI 页后查看串口是否有 `WS connected`、`session_id`、`sent listen start`、`stt` 和 `tts`。
- 扬声器杂音：确认已烧录最新固件；当前版本使用内部 RAM 三缓冲和 48 kHz Speaker 输出。
- SD 未识别：格式化为 FAT/FAT32，重新插入，查看串口中的 `SD.begin failed`、`CARD_NONE`、`Root open failed` 或 `Probe write failed`。
