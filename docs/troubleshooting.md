# Troubleshooting

Chinese version: [troubleshooting_CN.md](troubleshooting_CN.md).

Use the serial monitor at 115200 baud when diagnosing startup, camera, power, or AI behavior.

```powershell
pio device monitor -b 115200
```

## Camera Page Shows `Camera not ready`

Expected logs to check:

- `Foreground camera start failed`
- `esp_camera_init FAILED`
- `Camera: beginInternal() PSRAM free=...`
- `Camera: Touch task suspended for I2C`

Likely causes:

- CoreS3 internal I2C contention between touch/IMU and camera SCCB.
- Low free PSRAM or fragmented PSRAM.
- Camera bus initialization failure after a previous deinit.
- Power instability during camera startup.

Current mitigations in code:

- Foreground Camera and AI Vision startup is deferred out of the state callback.
- User-opened camera clears failure cooldown.
- `M5.In_I2C.release()` is called before camera init.
- Touch task is temporarily suspended during camera init.
- A failed foreground open gets one clean `end()` plus retry.

## Brownout Or Unexpected Reboot

If boot logs show `Reset: BROWNOUT`, treat it as a power drop first.

Common triggers:

- Camera and servo motion active together.
- Two servos moving quickly from USB power.
- Music playback plus servo motion plus AI network/audio load.
- Weak USB cable or unstable external 5V supply.

Code-side mitigations:

- Brownout safe mode suppresses automatic Face-page camera bursts and startup chime for a short window.
- Face-page companion servo motion pauses during vision bursts.
- Servo motion speeds are conservative and rate-limited.
- Low battery handling stops Face-page vision capture and switches the face to `SLEEPY`.

Hardware recommendation:

- Power servos from an external 5V supply.
- Share ground between CoreS3, PCA9685, and servo power.
- Avoid powering two moving servos directly from the CoreS3 USB rail.

## PCA9685 Or Servo Does Not Respond

Check:

- PCA9685 address is `0x40`.
- PortA wiring is correct.
- Servo power supply is present.
- CoreS3, PCA9685, and servo power share ground.

The firmware disables PCA9685 polling after three missing-device scans in one boot. Reset the board after fixing wiring.

## SD Card Not Found

Check:

- FAT/FAT32 formatting.
- Good card contact.
- Serial logs for `SD.begin failed`, `CARD_NONE`, `Root open failed`, or `Probe write failed`.

The firmware retries SD init at 25, 10, 4, and 1 MHz.

## No AI Reply

Check serial logs for this sequence:

```text
WS connected
session_id
initialize
tools/list
sent listen start
stt
tts
```

If AI Vision says endpoint missing, the current XiaoZhi session did not provide a vision URL/token.
