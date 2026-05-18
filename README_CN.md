# CoreS3 AI 桌面宠物

这是一个基于 M5Stack CoreS3 的桌面宠物演示固件。当前版本已经整合表情 UI、触摸手势、Wi-Fi 状态、相机预览与拍照、IMU 番茄钟、SD 音乐播放、小智语音交互，以及通过 XiaoZhi MCP 工具触发的 AI Vision 识图流程。

英文说明见 [README.md](README.md)。

## 当前状态

- 表情界面：支持 11 种表情、眨眼、视线平滑、临时视线覆盖，摇晃设备会触发 `SICK` 表情，并支持安全的二轴舵机联动。
- 菜单界面：六个图标入口，分别是 Wi-Fi、Camera、Timer、Music、System、Servo Test。
- Camera Debug：支持 320 x 240 预览、FPS/状态叠层、Back 按钮、JPEG 拍照到 SD，以及真实检测后端可用时的拍照前居中钩子。
- 系统页：基于共享运行时状态快照，固定展示 Wi-Fi、SD、Audio、Control、Memory 五行状态，标题副标题汇总 XiaoZhi 和 Vision 状态。
- 番茄钟：通过 IMU 方向选择四个预设，支持屏幕旋转、开始、暂停、重置，结束后播放提示音并回到表情页显示临时表情。
- 音乐播放：扫描 `/music` 目录，最多播放 16 个 MP3 或 PCM WAV 文件。MP3 会混成单声道输出到 CoreS3 扬声器，并使用统一的 `quiet`、`normal`、`loud` 运行时音量档位。
- Servo Test：通过 CoreS3 PortA 控制 PCA9685，IMU X/Y 倾斜映射到水平和垂直舵机，测试范围 10 到 170 度。Servo Test 中位为水平 90 度、垂直 90 度。
- 舵机互动：表情点击、小智表情反应、小智舵机命令和跳舞彩蛋会共用限速运动层；表情和小智姿态保持水平 90 度、垂直 140 度的机械中位。
- 好感界面：表情页下滑打开 Bond 页面，显示好感值、等级、心情和最近互动。当前版本仍为纯内存态。
- 小智 AI：已实现 OTA 激活与配置、TLS WebSocket、Opus 麦克风上传、Opus TTS 播放、MCP 握手和工具处理，并新增设备状态查询与设备控制。
- AI Vision：通过小智服务下发的 vision endpoint 对相机画面进行描述，不是本地嵌入式视觉检测。
- 人脸检测：当前 Arduino / PlatformIO 构建中仍明确关闭，不启用假检测。

## 硬件

- M5Stack CoreS3，ESP32-S3
- USB 数据线，用于编译、烧录和串口监视
- FAT/FAT32 格式的 MicroSD / TF 卡，用于保存照片和 MP3/WAV 音乐
- 可选底座硬件：电池、PCA9685、二轴舵机

本固件使用的舵机接线：

```text
CoreS3 PortA -> PCA9685 I2C
PCA9685 地址: 0x40
水平舵机: PCA9685 通道 0
垂直舵机: PCA9685 通道 1
舵机电源: 外部舵机电源模块，并与 CoreS3 / PCA9685 共地
Servo Test 中位: 水平 90 度，垂直 90 度
表情 / 小智中位: 水平 90 度，垂直 140 度
```

本固件使用的 CoreS3 SD SPI 引脚：

```text
SCK  36
MISO 35
MOSI 37
CS   4
```

## 软件要求

- PlatformIO Core
- Git
- Windows、Linux 或 macOS 主机

项目配置为：

- PlatformIO board: `m5stack-cores3`
- Framework: Arduino
- MCU: ESP32-S3
- C++ standard: C++17
- PSRAM: 通过 `board_build.arduino.memory_type = qio_qspi` 启用 QSPI PSRAM

## 目录结构

```text
src/
├── main.cpp
├── app/        # 状态机、手势、好感、事件总线、共享系统状态快照
├── ai/         # 小智激活、WebSocket、Opus 音频、MCP
├── audio/      # SD MP3/WAV 音乐播放器
├── config/     # 固件常量和 Wi-Fi 密钥模板
├── network/    # Wi-Fi 管理和 AI Vision HTTP 客户端
├── power/      # 休眠状态与电池占位逻辑
├── servo/      # PCA9685 驱动与共享舵机运动层
├── storage/    # SD 探测、照片路径、文件写入
├── ui/         # 表情、菜单、相机、番茄钟、系统信息、音乐、好感 UI
└── vision/     # 相机管理、关闭的人脸检测接口、追踪器、IMU 方向
```

## Wi-Fi 配置

真实 Wi-Fi 密码不提交到 Git。

```powershell
Copy-Item src\config\wifi_secrets.example.h src\config\wifi_secrets.h
```

编辑 `src/config/wifi_secrets.h`：

```cpp
#pragma once
constexpr const char* WIFI_SSID = "你的WiFi名称";
constexpr const char* WIFI_PASSWORD = "你的WiFi密码";
```

`src/config/wifi_secrets.h` 已被 Git 忽略。

## 编译

```powershell
pio run
```

如果 Windows 上没有把 `pio` 加到 `PATH`，可以使用完整路径，例如：

```powershell
C:\Users\<you>\.platformio\penv\Scripts\platformio.exe run
```

## 烧录

```powershell
pio run -t upload
```

如果自动识别端口失败：

```powershell
pio run -t upload --upload-port COM5
```

如果烧录无法开始，长按 RESET 约 3 秒进入下载模式。烧录完成后，再按一次 RESET 运行固件。

## 串口监视

```powershell
pio device monitor -b 115200
```

指定端口：

```powershell
pio device monitor -p COM5 -b 115200
```

## 操作方式

- 表情页：右滑进入菜单；左滑或双击进入小智 AI；下滑进入 Bond；点击上、下、左、右切换表情、视线和舵机姿态；摇晃设备显示 `SICK`；长按进入睡眠。
- AI 页：单击切换监听；右滑或长按返回表情页。
- Bond 页：Back、上滑或左滑返回表情页。
- 菜单页：点击图标进入 Wi-Fi、Camera、Timer、Music、System 或 Servo；Back 返回表情页。
- Camera Debug：SHOT 保存 `/photos/IMG_####.jpg`；如果未来真实人脸检测后端可用，拍照前会短暂尝试舵机居中；Back 或左滑返回菜单。
- AI Vision：Back 或左滑关闭预览并返回小智 AI。
- 番茄钟：旋转设备选择预设；Start、Pause、Reset 控制计时；Back 返回菜单。
- 音乐页：Play/Pause、Stop、Next 控制 `/music` 中的 MP3/WAV 文件。
- 系统页：显示 Wi-Fi、SD、Audio、Control、Memory 五行状态，副标题汇总 XiaoZhi 与 Vision；Back 返回菜单。
- Servo Test：进入后舵机回到 90/90 测试中位；左右倾斜控制水平舵机，前后倾斜控制垂直舵机；点击回到 90/90；长按释放 PWM；Back 或左滑返回菜单并恢复表情 / 小智 90/140 中位。

## SD 卡内容

固件会使用以下路径：

```text
/photos/IMG_####.jpg
/music/*.mp3
/music/*.wav
```

音乐播放当前支持 MP3 文件，以及 8-bit 或 16-bit、单声道或双声道的 PCM WAV 文件。MP3 的单声道 / 双声道会先混成单声道，再送入内置扬声器。扫描时最多读取前 16 个音频文件，并按文件名排序。

## 小智 AI

核心代码位于 `src/ai/xiaozhi_client.h` 和 `src/ai/xiaozhi_client.cpp`。

流程：

1. 从表情页进入 AI 页。
2. 等待 Wi-Fi 后，请求小智 OTA 接口获取激活与配置数据。
3. 使用返回的 URL 和 token 建立 TLS WebSocket。
4. 发送带 Opus 音频参数并启用 MCP 的 `hello`。
5. 响应 MCP 初始化请求。
6. MCP 就绪后开始监听。
7. 采集 16 kHz 单声道 PCM，编码成 Opus，并通过 WebSocket binary frame 上传。
8. 解码服务端返回的 Opus TTS，并通过 CoreS3 扬声器播放。

CoreS3 麦克风和扬声器按半双工处理：

- 监听时停止扬声器并启动麦克风。
- 说话时停止麦克风并启动扬声器。
- TTS 结束后，如果 AI 页仍请求监听，会重新开始监听。

当前暴露的 MCP 工具：

```text
self.camera.open
self.camera.close
self.camera.capture_photo
self.vision.describe_scene
self.pomodoro.open
self.music.control
self.pet.react
self.servo.control
self.device.status
self.device.control
```

AI Vision 依赖小智服务在 MCP 初始化中提供的 vision endpoint 和 token，不是本地嵌入式视觉检测。

`self.servo.control` 支持 `center`、`left`、`right`、`up`、`down`、`nod`、`shake`、`dance`、`release`。这些动作只控制舵机姿态，不切换页面，也不重新打开小智音频连接。`dance` 会先启动舵机动作，再尝试配合 `/music` 中的音频播放；音乐或 SD 异常不会取消舵机动作。

`self.device.status` 支持 `detail=brief|full`。`brief` 固定返回两句中文自然语句，先讲 Wi-Fi 与 SD，再讲音乐、AI 与 Vision。`full` 会额外补充 heap 和 PSRAM 信息。

`self.device.control` 支持可选的 `page`、`brightness`、`volume`、`sleep` 参数。页面支持 `face`、`menu`、`wifi`、`system`、`camera`、`music`、`pomodoro`、`ai`。亮度支持 `dim`、`normal`、`bright`。音量支持 `quiet`、`normal`、`loud`。休眠控制支持 `wake` 和 `sleep`。唤醒后会恢复最近一次非睡眠亮度偏好，不会写死回到满亮。

Transcript fallback 也支持常见中文指令，例如系统状态查询、打开系统页 / 音乐页 / 相机页 / 番茄钟、回到主页、调亮一点、调暗一点、大声一点、小声一点、睡觉、唤醒。

## 已知限制

- 真实人脸检测仍然关闭。当前入口在 `src/vision/face_detector.*`，其中 `FaceDetector::backendAvailable()` 返回 false，`detect()` 不返回人脸框。ESP-DL 或 ESP-WHO 仍需要验证 ESP-IDF 或 Arduino-as-IDF-component 集成路径。
- 拍照人脸追踪只有在 `FaceDetector::detect()` 返回真实人脸框后才会形成闭环；当前 Arduino 构建会明确提示 unavailable，但仍会正常保存照片。
- AI Vision 依赖小智服务端提供的 vision endpoint。
- CoreS3 内置麦克风和扬声器是半双工，当前没有实现全双工打断和回声消除。
- 电池电压读取仍然是占位实现，需结合最终 PMU API / 硬件路径验证。
- 亮度和音量档位都是运行时设置，不做持久化；重启后恢复为 `bright` + `normal`。
- SD 卡识别受卡格式、接触和供电影响。固件会按 25、10、4、1 MHz 逐级重试。
- 舵机表情 / 小智控制是带限速的开环姿态控制，不是 PID 云台控制或自主底座运动。
- Bond / 好感值仍是纯内存态，重启后回到默认值 35。

## 故障排查

- 烧录失败：长按 RESET 约 3 秒后重新烧录，烧录完成后再按一次 RESET。
- Wi-Fi 显示 `Not configured`：从示例创建 `src/config/wifi_secrets.h`。
- 小智 OTA 返回 `400`：检查 `XIAOZHI_BOARD_TYPE`、`XIAOZHI_FIRMWARE_VERSION` 和串口中的 OTA identity。
- 小智 WebSocket 在 MCP 后断开：确认串口中依次出现 `initialize`、`tools/list` 和 `sent listen start`。
- 没有 AI 回复：确认 Wi-Fi 已连接，并检查 `WS connected`、`session_id`、`sent listen start`、`stt`、`tts` 日志。
- AI Vision 提示 endpoint missing：当前小智会话没有下发 vision capability。
- SD 未识别：格式化为 FAT/FAT32，重新插入，并查看串口中的 `SD.begin failed`、`CARD_NONE`、`Root open failed` 或 `Probe write failed`。
- Servo 页显示 `PCA9685 not found @0x40`：检查 PortA 接线、PCA9685 地址跳线、外部舵机供电和共地。
- PCA9685 灯短暂熄灭或舵机在快速双轴动作下嗡鸣：使用更强的外部舵机电源，缩短或加粗供电线，确认共地，并避免机械撞限位。
- MP3 无声或播放几秒后重启：查看串口中的曲目路径、MP3 采样率 / 声道信息和 speaker 队列失败日志。如果下次启动显示 `BROWNOUT` 或 `POWERON`，优先按供电掉压处理；如果显示 `WDT` 或 `PANIC`，再继续做软件崩溃定位。
