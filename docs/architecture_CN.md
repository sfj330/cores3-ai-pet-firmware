# 架构说明

## 运行时结构

固件由一个小型 App 状态机和多个 FreeRTOS 后台任务组成。

```text
Touch task -> GestureManager -> AppState -> stateChangeHandler()
UI task    -> 当前 UI 类绘制页面
Camera task -> CameraManager 帧 -> Camera Debug / AI Vision UI
Vision task -> 启发式 FaceDetector -> FaceTracker / 拍照居中
AI task     -> XiaoZhi WebSocket、Opus 音频、MCP 工具
Power task  -> 电池/休眠状态与低电压回调
Network task -> Wi-Fi 重连与状态更新
Music task  -> 音乐页激活时进行 SD MP3/WAV 流式播放
```

## 主要状态

- `FACE`：默认表情页，短时启发式人脸检测、触摸和 IMU 反应。
- `MENU`：五个入口，分别是 Wi-Fi、Camera、Timer、Music、System。
- `CAMERA_DEBUG`：前台相机预览和 SD 拍照。
- `AI_VISION`：小智视觉描述使用的前台相机预览。
- `AI`：小智语音交互页面。
- `POMODORO`、`MUSIC`、`SYSTEM_INFO`、`AFFINITY`、`SETTINGS`、`SLEEP`：各自独立的功能页。

## 资源所有权

- 相机采用懒启动。表情页只做短时检测窗口，Camera Debug 和 AI Vision 才拥有前台采集。
- 前台相机启动会延迟到主循环执行，避免触摸/状态回调被卡住。
- 小智语音在监听/说话时拥有麦克风和扬声器；音乐播放会在独占音频前停止或暂停。
- 舵机动作统一经过 `ServoMotionController`，避免大角度瞬间跳转。
- PCA9685 是可选底座。单次启动中连续 3 次扫描不到后，驱动会禁用自身，避免一直轮询。

## 目录职责

- `src/app`：状态机、手势、事件总线、好感系统、共享系统状态。
- `src/ui`：只负责显示的页面类。
- `src/vision`：相机管理、启发式检测、追踪器、IMU 方向。
- `src/ai`：小智激活、WebSocket、Opus 音频、MCP 工具。
- `src/audio`：SD 音乐播放。
- `src/servo`：PCA9685 驱动和安全运动规划。
- `src/power`：电池电压、低电压和休眠辅助。
- `src/storage`：SD 卡探测和文件写入。
- `src/network`：Wi-Fi 与远程 AI Vision HTTP 客户端。

## 扩展建议

- 新增页面时，先扩展 `AppStateEnum`，再添加 `src/ui` 页面类，最后在 `stateChangeHandler()` 和 `gestureEventHandler()` 中接入。
- 新增小智工具时，在 `xiaozhi_client.*` 注册工具，再通过 `main.cpp` 的 pending-tool 处理流程执行。
- 新的开关、时间、速度、阈值优先放进 `src/config/app_config.h`，方便比赛演示时快速调参。
