# 故障排查

排查启动、相机、电源或 AI 问题时，优先打开 115200 波特率串口。

```powershell
pio device monitor -b 115200
```

## Camera 页显示 `Camera not ready`

重点看这些串口日志：

- `Foreground camera start failed`
- `esp_camera_init FAILED`
- `Camera: beginInternal() PSRAM free=...`
- `Camera: Touch task suspended for I2C`

常见原因：

- CoreS3 内部 I2C 上触摸/IMU 和相机 SCCB 抢总线。
- 可用 PSRAM 不足或碎片化。
- 上一次相机反初始化后，总线状态还没恢复。
- 相机启动瞬间供电不稳。

当前代码的缓解措施：

- Camera 和 AI Vision 的前台启动从状态回调延迟到主循环执行。
- 用户主动打开相机会清除失败冷却。
- 相机初始化前调用 `M5.In_I2C.release()`。
- 相机初始化期间短暂停止触摸任务。
- 前台打开失败后，会执行一次干净 `end()` 并重试。

## Brownout 或异常重启

如果启动日志出现 `Reset: BROWNOUT`，优先按供电掉压处理。

常见触发：

- 相机和舵机动作同时开启。
- 两个舵机只靠 USB 供电快速运动。
- 音乐播放、舵机动作、AI 网络/音频负载叠加。
- USB 线质量差或外部 5V 电源不稳。

代码侧缓解：

- Brownout 后短时间进入安全模式，抑制表情页自动相机扫描和开机提示音。
- 表情页视觉检测窗口内暂停 companion 舵机动作。
- 舵机速度保守并经过限速控制。
- 低电压时停止表情页视觉检测，并切到 `SLEEPY` 表情。

硬件建议：

- 舵机使用独立外部 5V 供电。
- CoreS3、PCA9685、舵机电源必须共地。
- 不要让两个运动中的舵机直接吃 CoreS3 的 USB 电源。

## PCA9685 或舵机无响应

检查：

- PCA9685 地址是否为 `0x40`。
- PortA 接线是否正确。
- 舵机外部供电是否接入。
- CoreS3、PCA9685、舵机电源是否共地。

单次启动中连续 3 次扫描不到 PCA9685 后，固件会停止继续轮询。修好接线后需要重启。

## SD 卡未识别

检查：

- 是否 FAT/FAT32 格式。
- 卡槽接触是否可靠。
- 串口是否有 `SD.begin failed`、`CARD_NONE`、`Root open failed`、`Probe write failed`。

固件会按 25、10、4、1 MHz 逐级重试 SD 初始化。

如果相册页或网页照片列表是空的，但 SD 已经挂载成功，检查 `/photos` 下文件名是否为标准 `.jpg` 或 `.JPG`。

## 没有 AI 回复

串口里应看到类似流程：

```text
WS connected
session_id
initialize
tools/list
sent listen start
stt
tts
```

如果 AI Vision 提示 endpoint missing，说明当前小智会话没有下发 vision URL/token。

如果前台相机、音乐或页面工具动作期间 TTS 播放被截断，检查是否出现 `pauseForForegroundTool` 流程和 `speaker queue timeout` 日志。近期代码已经把播放队列等待时间调长，并在前台暂停期间直接丢弃解码后的播放片段，以降低音频资源冲突。

## 网页控制页无法访问

检查：

- Wi-Fi 是否已连接，Wi-Fi 页面是否显示了有效设备 IP。
- 串口中是否出现 `WebServer: started on port 80`。
- 手机或电脑是否与 CoreS3 处于同一局域网。
- 当前是否有别的服务占用了 `80` 端口。

如果 Wi-Fi 断开，固件会自动停止网页服务；重连后会再次启动。
