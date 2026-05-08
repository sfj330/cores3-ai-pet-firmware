#include <M5CoreS3.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include "esp_system.h"

#include "config/app_config.h"
#include "app/app_state.h"
#include "app/gesture_manager.h"
#include "app/event_bus.h"
#include "ui/face_ui.h"
#include "ui/menu_ui.h"
#include "ui/camera_debug_ui.h"
#include "ui/pomodoro_ui.h"
#include "vision/camera_manager.h"
#include "vision/face_detector.h"
#include "vision/face_tracker.h"
#include "vision/imu_orientation.h"
#include "ai/xiaozhi_client.h"
#include "ai/voice_state.h"
#include "power/power_manager.h"
#include "network/wifi_manager.h"
#include "storage/storage_manager.h"

FaceUI gFaceUI;
MenuUI gMenuUI;
CameraDebugUI gCameraDebugUI;
PomodoroUI gPomodoroUI;
CameraManager gCameraManager;
FaceDetector gFaceDetector;
FaceTracker gFaceTracker;
ImuOrientation gImuOrientation;
XiaoZhiClient gXiaoZhiClient;
PowerManager gPowerManager;
WifiManager gWifiManager;
StorageManager gStorageManager;

static float gCurrentFps = 0.0f;
static unsigned long gLastFpsCalc = 0;
static int gFrameCount = 0;

static TaskHandle_t uiTaskHandle = nullptr;
static TaskHandle_t touchTaskHandle = nullptr;
static TaskHandle_t cameraTaskHandle = nullptr;
static TaskHandle_t visionTaskHandle = nullptr;
static TaskHandle_t aiTaskHandle = nullptr;
static TaskHandle_t powerTaskHandle = nullptr;
static TaskHandle_t networkTaskHandle = nullptr;
static SemaphoreHandle_t displayMutex = nullptr;
static bool gSickActive = false;
static unsigned long gSickUntil = 0;

static void gestureEventHandler(const GestureEvent& event);
static void stateChangeHandler(AppStateEnum state);
static void handleEvent(const Event& event);

static bool takeDisplayLock(TickType_t timeout = pdMS_TO_TICKS(20)) {
    return displayMutex == nullptr || xSemaphoreTake(displayMutex, timeout) == pdTRUE;
}

static void giveDisplayLock() {
    if (displayMutex != nullptr) {
        xSemaphoreGive(displayMutex);
    }
}

static void handleFaceTap(int x, int y) {
    int cx = DISPLAY_WIDTH / 2;
    int cy = DISPLAY_HEIGHT / 2;
    AppState& appState = AppState::instance();

    if (y < cy / 2) {
        appState.setEmotion(FaceEmotion::HAPPY);
        gFaceUI.setTemporaryGaze(0.0f, -0.5f, 1500);
    } else if (y > cy + cy / 2) {
        appState.setEmotion(FaceEmotion::SHY);
        gFaceUI.setTemporaryGaze(0.0f, 0.5f, 1500);
    } else if (x < cx) {
        gFaceUI.setTemporaryGaze(-1.0f, 0.0f, 1200);
        if (appState.getEmotion() != FaceEmotion::TRACKING) {
            appState.setEmotion(FaceEmotion::CURIOUS);
        }
    } else {
        gFaceUI.setTemporaryGaze(1.0f, 0.0f, 1200);
        if (appState.getEmotion() != FaceEmotion::TRACKING) {
            appState.setEmotion(FaceEmotion::CURIOUS);
        }
    }
}

void uiTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / 20);

    while (true) {
        unsigned long now = millis();
        AppStateEnum state = AppState::instance().getState();
        FaceEmotion emotion = AppState::instance().getEmotion();

        if (takeDisplayLock()) {
            switch (state) {
                case AppStateEnum::FACE:
                    gFaceUI.setExpression(emotion);
                    gFaceUI.update();
                    break;

                case AppStateEnum::MENU:
                    gMenuUI.update();
                    break;

                case AppStateEnum::CAMERA_DEBUG:
                    gCameraDebugUI.setFps(gCurrentFps);
                    gCameraDebugUI.update();
                    break;

                case AppStateEnum::POMODORO:
                    gPomodoroUI.update();
                    break;

                case AppStateEnum::AI:
                    gFaceUI.setExpression(emotion);
                    gFaceUI.update();
                    break;

                case AppStateEnum::SLEEP:
                    break;
            }
            giveDisplayLock();
        }

        gFrameCount++;
        if (now - gLastFpsCalc >= 1000) {
            gCurrentFps = gFrameCount;
            gFrameCount = 0;
            gLastFpsCalc = now;
        }

        vTaskDelay(delayTicks);
    }
}

void touchTask(void* pvParameters) {
    static GestureManager gestureManager;
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / TOUCH_SAMPLING_HZ);

    gestureManager.setCallback(gestureEventHandler);

    while (true) {
        M5.update();

        TouchPoint tp;
        tp.touched = M5.Touch.getCount() > 0;

        if (tp.touched) {
            auto t = M5.Touch.getDetail();
            tp.x = t.x;
            tp.y = t.y;
        }

        gestureManager.update(tp);

        vTaskDelay(delayTicks);
    }
}

void cameraTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / CAMERA_FPS);

    while (true) {
        if (gCameraManager.isRunning()) {
            AppStateEnum state = AppState::instance().getState();

            if (state == AppStateEnum::CAMERA_DEBUG) {
                CameraFrame frame = gCameraManager.getDisplayFrame();

                if (frame.valid) {
                    if (takeDisplayLock(pdMS_TO_TICKS(10))) {
                        gCameraDebugUI.pushCameraFrame((uint16_t*)frame.data, frame.width, frame.height);
                        giveDisplayLock();
                    }
                    gCameraManager.releaseFrame(frame);
                }
            }
        }

        vTaskDelay(delayTicks);
    }
}

void visionTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / DETECTION_FPS);

    while (true) {
        AppStateEnum state = AppState::instance().getState();

        if (state == AppStateEnum::FACE || state == AppStateEnum::CAMERA_DEBUG) {
            if (gCameraManager.isRunning() && gFaceDetector.isEnabled() && gFaceDetector.backendAvailable()) {
                CameraFrame frame = gCameraManager.getDetectionFrame();
                if (frame.valid) {
                    FaceResult face = gFaceDetector.detect(frame.data, frame.width, frame.height);
                    gCameraManager.releaseFrame(frame);
                    gFaceTracker.update(face);

                    if (gFaceTracker.hasFace()) {
                        gFaceUI.setGazeOffset(gFaceTracker.getGazeX(), gFaceTracker.getGazeY());

                        if (state == AppStateEnum::FACE) {
                            FaceEmotion currentEmotion = AppState::instance().getEmotion();
                            if (currentEmotion == FaceEmotion::NORMAL || currentEmotion == FaceEmotion::TRACKING) {
                                AppState::instance().setEmotion(FaceEmotion::TRACKING);
                            }
                        }

                        Event detectedEvent;
                        detectedEvent.type = EventType::FACE_DETECTED;
                        EventBus::instance().publish(detectedEvent);
                    } else {
                        if (state == AppStateEnum::FACE &&
                            AppState::instance().getEmotion() == FaceEmotion::TRACKING) {
                            AppState::instance().setEmotion(FaceEmotion::NORMAL);
                        }

                        gFaceUI.setGazeOffset(0, 0);

                        Event lostEvent;
                        lostEvent.type = EventType::FACE_LOST;
                        EventBus::instance().publish(lostEvent);
                    }
                }
            }
        }

        vTaskDelay(delayTicks);
    }
}

void aiTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(50);

    while (true) {
        if (gXiaoZhiClient.isConnected()) {
            gXiaoZhiClient.process();
        }

        vTaskDelay(delayTicks);
    }
}

void powerTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(POWER_TASK_INTERVAL_MS);

    while (true) {
        gPowerManager.update();

        Event powerEvent;
        powerEvent.type = EventType::POWER_EVENT;
        powerEvent.floatParam = gPowerManager.getVoltage();
        EventBus::instance().publish(powerEvent);

        vTaskDelay(delayTicks);
    }
}

void networkTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(WIFI_RECONNECT_INTERVAL_MS);

    while (true) {
        gWifiManager.update();

        if (AppState::instance().getState() == AppStateEnum::MENU) {
            gMenuUI.setWifiStatus(gWifiManager.statusText().c_str(), gWifiManager.ipString().c_str());
            gMenuUI.setVisionStatus(gFaceDetector.statusText());
        } else if (AppState::instance().getState() == AppStateEnum::AI) {
            if (gWifiManager.isConnected()) {
                gFaceUI.setAiOverlay(true, "Device verification required", "Real service not configured", true);
            } else {
                gFaceUI.setAiOverlay(true, "Wi-Fi required", "Device verification required", false);
            }
        }

        vTaskDelay(delayTicks);
    }
}

static void handleCameraShot() {
    if (!gStorageManager.forceReprobe()) {
        gCameraDebugUI.setCaptureStatus(gStorageManager.statusText().c_str());
        gCameraDebugUI.setSdReady(false);
        return;
    }

    gCameraDebugUI.setSdReady(true);
    gCameraDebugUI.setCaptureStatus("Capturing...");

    String path = gStorageManager.nextPhotoPath();
    if (path.length() == 0) {
        gCameraDebugUI.setCaptureStatus("Path error");
        return;
    }

    String status;
    bool ok = gCameraManager.captureJpegToFile(path.c_str(), status);
    if (ok) {
        gCameraDebugUI.setLastPhotoPath(path.c_str());
        gCameraDebugUI.setCaptureStatus(status.c_str());
        Serial.printf("Photo saved: %s\n", path.c_str());
    } else {
        gCameraDebugUI.setCaptureStatus(status.c_str());
        Serial.printf("Photo failed: %s\n", status.c_str());
    }
}

static void gestureEventHandler(const GestureEvent& event) {
    AppState& appState = AppState::instance();
    AppStateEnum currentState = appState.getState();

    Event gestureEvent;
    gestureEvent.type = EventType::GESTURE_EVENT;
    gestureEvent.intParam = static_cast<int>(event.type);
    EventBus::instance().publish(gestureEvent);

    switch (currentState) {
        case AppStateEnum::FACE:
            switch (event.type) {
                case GestureType::RIGHT_SWIPE:
                    appState.setState(AppStateEnum::MENU);
                    break;
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::AI);
                    break;
                case GestureType::SINGLE_TAP:
                    handleFaceTap(event.endX, event.endY);
                    break;
                case GestureType::DOUBLE_TAP:
                    appState.setState(AppStateEnum::AI);
                    break;
                case GestureType::LONG_PRESS:
                    appState.setState(AppStateEnum::SLEEP);
                    if (takeDisplayLock()) {
                        gPowerManager.enterSleep();
                        giveDisplayLock();
                    }
                    break;
                default:
                    break;
            }
            break;

        case AppStateEnum::AI:
            switch (event.type) {
                case GestureType::RIGHT_SWIPE:
                    appState.setState(AppStateEnum::FACE);
                    break;
                case GestureType::SINGLE_TAP:
                    appState.setEmotion(FaceEmotion::LISTENING);
                    gFaceUI.setTemporaryGaze(0.0f, -0.3f, 800);
                    break;
                case GestureType::LONG_PRESS:
                    appState.setState(AppStateEnum::FACE);
                    break;
                default:
                    break;
            }
            break;

        case AppStateEnum::MENU: {
            MenuHitZone hit = gMenuUI.hitTest(event.endX, event.endY);
            if (event.type == GestureType::SINGLE_TAP) {
                if (hit == MenuHitZone::MENU_HIT_BACK) {
                    appState.setState(AppStateEnum::FACE);
                    break;
                }
                if (hit == MenuHitZone::MENU_HIT_CARD) {
                    switch (gMenuUI.getActiveCard()) {
                        case 0: break;
                        case 1: appState.setState(AppStateEnum::CAMERA_DEBUG); break;
                        case 2: appState.setState(AppStateEnum::POMODORO); break;
                        case 3: break;
                    }
                    break;
                }
            }
            switch (event.type) {
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::FACE);
                    break;
                case GestureType::RIGHT_SWIPE:
                    gMenuUI.setActiveCard(gMenuUI.getActiveCard() + 1);
                    break;
                default:
                    break;
            }
            break;
        }

        case AppStateEnum::CAMERA_DEBUG: {
            CameraHitZone hit = gCameraDebugUI.hitTest(event.endX, event.endY);
            if (event.type == GestureType::SINGLE_TAP) {
                if (hit == CameraHitZone::HIT_BACK) {
                    appState.setState(AppStateEnum::MENU);
                    break;
                }
                if (hit == CameraHitZone::HIT_SHOT) {
                    handleCameraShot();
                    break;
                }
            }
            switch (event.type) {
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::MENU);
                    break;
                default:
                    break;
            }
            break;
        }

        case AppStateEnum::POMODORO: {
            PomoHitZone hit = gPomodoroUI.hitTest(event.endX, event.endY);
            if (event.type == GestureType::SINGLE_TAP) {
                if (hit == PomoHitZone::POMO_HIT_BACK) {
                    appState.setState(AppStateEnum::MENU);
                    break;
                }
                if (hit == PomoHitZone::POMO_HIT_START) {
                    gPomodoroUI.togglePause();
                    break;
                }
                if (hit == PomoHitZone::POMO_HIT_RESET) {
                    gPomodoroUI.reset();
                    gPomodoroUI.markDirty();
                    break;
                }
                gPomodoroUI.togglePause();
                break;
            }
            switch (event.type) {
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::MENU);
                    break;
                default:
                    break;
            }
            break;
        }

        case AppStateEnum::SLEEP:
            switch (event.type) {
                case GestureType::SINGLE_TAP:
                case GestureType::DOUBLE_TAP:
                    appState.setState(AppStateEnum::FACE);
                    if (takeDisplayLock()) {
                        gPowerManager.exitSleep();
                        gFaceUI.wake();
                        giveDisplayLock();
                    }
                    break;
                default:
                    break;
            }
            break;
    }
}

static void stateChangeHandler(AppStateEnum state) {
    switch (state) {
        case AppStateEnum::FACE:
            gMenuUI.hide();
            gCameraDebugUI.hide();
            gPomodoroUI.hide();
            gFaceUI.setAiOverlay(false, "", "", gWifiManager.isConnected());
            gFaceUI.setVisionStatus(gFaceDetector.statusText());
            AppState::instance().setEmotion(FaceEmotion::NORMAL);
            gSickActive = false;
            gSickUntil = 0;
            if (!gFaceDetector.backendAvailable() || !gFaceDetector.isEnabled()) {
                gCameraManager.stopCapture();
                gCameraDebugUI.setCameraReady(false);
            } else {
                if (!gCameraManager.isRunning()) {
                    bool camOk = gCameraManager.isInitialized() || gCameraManager.begin();
                    camOk = camOk && gCameraManager.startCapture();
                    Serial.println(camOk ? "Vision: camera started for face tracking" : "Vision: camera start failed");
                }
            }
            break;

        case AppStateEnum::AI:
            gMenuUI.hide();
            gCameraDebugUI.hide();
            gPomodoroUI.hide();
            gCameraManager.stopCapture();
            gCameraDebugUI.setCameraReady(false);
            gFaceUI.setVisionStatus("");
            if (gWifiManager.isConnected()) {
                AppState::instance().setEmotion(FaceEmotion::LISTENING);
                gFaceUI.setAiOverlay(true, "Device verification required", "Real service not configured", true);
                gXiaoZhiClient.startListening();
            } else {
                AppState::instance().setEmotion(FaceEmotion::SURPRISED);
                gFaceUI.setAiOverlay(true, "Wi-Fi required", "Device verification required", false);
                gXiaoZhiClient.startListening();
                Serial.println("AI: Wi-Fi not connected, placeholder mode");
            }
            break;

        case AppStateEnum::MENU:
            gCameraDebugUI.hide();
            gPomodoroUI.hide();
            gCameraManager.stopCapture();
            gCameraDebugUI.setCameraReady(false);
            gMenuUI.setWifiStatus(gWifiManager.statusText().c_str(), gWifiManager.ipString().c_str());
            gMenuUI.setVisionStatus(gFaceDetector.statusText());
            gFaceUI.setAiOverlay(false, "", "", gWifiManager.isConnected());
            gMenuUI.show();
            break;

        case AppStateEnum::CAMERA_DEBUG:
            gMenuUI.hide();
            gPomodoroUI.hide();
            gStorageManager.ensureReady();
            gCameraDebugUI.setSdReady(gStorageManager.isReady());
            gCameraDebugUI.setCaptureStatus(gStorageManager.statusText().c_str());
            gFaceUI.setAiOverlay(false, "", "", gWifiManager.isConnected());
            gCameraDebugUI.show();
            if (!gCameraManager.isRunning()) {
                bool cameraReady = gCameraManager.isInitialized() || gCameraManager.begin();
                cameraReady = cameraReady && gCameraManager.startCapture();
                gCameraDebugUI.setCameraReady(cameraReady);
                Serial.println(cameraReady ? "Camera debug started" : "Camera debug start failed");
            }
            break;

        case AppStateEnum::POMODORO:
            gMenuUI.hide();
            gCameraDebugUI.hide();
            gCameraManager.stopCapture();
            gCameraDebugUI.setCameraReady(false);
            gFaceUI.setAiOverlay(false, "", "", gWifiManager.isConnected());
            gPomodoroUI.show();
            break;

        case AppStateEnum::SLEEP:
            if (takeDisplayLock()) {
                M5.Lcd.fillScreen(TFT_BLACK);
                M5.Lcd.setBrightness(0);
                giveDisplayLock();
            }
            break;
    }
}

static void aiStateHandler(VoiceState voiceState) {
    AppState& appState = AppState::instance();
    const char* aiStatus = "IDLE";
    switch (voiceState) {
        case VoiceState::IDLE:
            appState.setEmotion(FaceEmotion::NORMAL);
            aiStatus = "IDLE";
            break;
        case VoiceState::LISTENING:
            appState.setEmotion(FaceEmotion::LISTENING);
            aiStatus = "LISTENING";
            break;
        case VoiceState::THINKING:
            appState.setEmotion(FaceEmotion::THINKING);
            aiStatus = "THINKING";
            break;
        case VoiceState::SPEAKING:
            appState.setEmotion(FaceEmotion::SPEAKING);
            aiStatus = "SPEAKING";
            break;
        case VoiceState::ERROR:
            appState.setEmotion(FaceEmotion::SURPRISED);
            aiStatus = "ERROR";
            break;
    }
    if (appState.getState() == AppStateEnum::AI) {
        gFaceUI.setAiOverlay(true, aiStatus, "Device verification required", gWifiManager.isConnected());
    }
    Event aiEvent;
    aiEvent.type = EventType::AI_STATE_CHANGE;
    aiEvent.intParam = static_cast<int>(voiceState);
    EventBus::instance().publish(aiEvent);
}

static void lowBatteryHandler(float voltage) {
    AppState& appState = AppState::instance();
    if (appState.getState() == AppStateEnum::FACE) {
        appState.setEmotion(FaceEmotion::SLEEPY);
    }
}

static void handleEvent(const Event& event) {
}

static void bootStep(const char* msg, int line) {
    M5.Lcd.fillRect(0, line * 12, DISPLAY_WIDTH, 12, TFT_BLACK);
    M5.Lcd.setCursor(0, line * 12);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.printf("Step %d: %s", line, msg);
    Serial.println(msg);
}

static const char* resetReasonName(esp_reset_reason_t reason) {
    switch (reason) {
        case ESP_RST_POWERON: return "POWERON";
        case ESP_RST_EXT: return "EXT";
        case ESP_RST_SW: return "SW";
        case ESP_RST_PANIC: return "PANIC";
        case ESP_RST_INT_WDT: return "INT_WDT";
        case ESP_RST_TASK_WDT: return "TASK_WDT";
        case ESP_RST_WDT: return "WDT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_BROWNOUT: return "BROWNOUT";
        case ESP_RST_SDIO: return "SDIO";
        default: return "UNKNOWN";
    }
}

static bool createTaskChecked(TaskFunction_t taskFn, const char* name,
                              uint32_t stackSize, UBaseType_t priority,
                              TaskHandle_t* handle, BaseType_t core) {
    BaseType_t result = xTaskCreatePinnedToCore(taskFn, name, stackSize, nullptr,
                                                priority, handle, core);
    if (result != pdPASS) {
        Serial.printf("Task create failed: %s, stack=%lu, core=%ld\n",
                      name, static_cast<unsigned long>(stackSize),
                      static_cast<long>(core));
        return false;
    }
    Serial.printf("Task created: %s\n", name);
    return true;
}

void setup() {
    Serial.begin(115200);
    unsigned long serialWaitStart = millis();
    while (!Serial && millis() - serialWaitStart < 1000) {
        delay(10);
    }
    Serial.println("\n\n=== CoreS3 AI Pet Boot ===");

    auto cfg = M5.config();
    cfg.output_power = true;
    M5.begin(cfg);
    displayMutex = xSemaphoreCreateMutex();
    M5.Lcd.setBrightness(255);
    M5.Lcd.fillScreen(TFT_BLACK);
    M5.Lcd.setTextColor(TFT_WHITE, TFT_BLACK);
    M5.Lcd.setTextSize(1);
    M5.Lcd.setTextDatum(TL_DATUM);
    M5.Lcd.setCursor(0, 0);
    M5.Lcd.println("Booting CoreS3 AI Pet...");
    esp_reset_reason_t resetReason = esp_reset_reason();
    M5.Lcd.printf("Reset: %s\n", resetReasonName(resetReason));
    Serial.printf("Reset reason: %s (%d)\n", resetReasonName(resetReason), static_cast<int>(resetReason));
    Serial.println("M5.begin() OK");
    Serial.printf("Heap: %u, PSRAM free/total: %u/%u\n",
                  ESP.getFreeHeap(), ESP.getFreePsram(), ESP.getPsramSize());
    delay(200);

    bootStep("AppState", 1);
    AppState::instance().registerCallback(stateChangeHandler);

    bootStep("FaceUI sprite...", 2);
    bool faceUiOk = gFaceUI.begin();
    bootStep(faceUiOk ? "FaceUI OK" : "FaceUI FAIL", 2);

    bootStep("MenuUI...", 3);
    gMenuUI.begin();
    bootStep("MenuUI OK", 3);

    bootStep("CameraDebugUI...", 4);
    gCameraDebugUI.begin();

    bootStep("PomodoroUI...", 5);
    gPomodoroUI.begin();

    bootStep("WifiManager...", 6);
    gWifiManager.begin();
    bootStep(gWifiManager.isConfigured() ? "WiFi configured" : "WiFi not configured", 6);

    bootStep("StorageManager...", 7);
    bool sdOk = gStorageManager.begin();
    bootStep(sdOk ? "SD ready" : "SD not found (will retry)", 7);

    bootStep("IMU Orientation...", 8);
    gImuOrientation.begin();

    bootStep("Camera deferred", 9);
    if (CAMERA_INIT_ON_BOOT) {
        bool camOk = gCameraManager.begin();
        gCameraManager.stopCapture();
        bootStep(camOk ? "Camera OK" : "Camera FAIL (non-fatal)", 9);
    }

    bootStep("FaceDetector...", 10);
    gFaceDetector.begin();
    gFaceUI.setVisionStatus(gFaceDetector.statusText());
    gMenuUI.setVisionStatus(gFaceDetector.statusText());
    bootStep("FaceDetector OK", 10);

    bootStep("XiaoZhiClient...", 11);
    gXiaoZhiClient.begin();
    gXiaoZhiClient.setStateCallback(aiStateHandler);
    bootStep("XiaoZhiClient OK", 11);

    bootStep("PowerManager...", 12);
    gPowerManager.begin();
    gPowerManager.setLowBatteryCallback(lowBatteryHandler);
    bootStep("PowerManager OK", 12);

    bootStep("EventBus...", 13);
    EventBus::instance().subscribe(EventType::FACE_DETECTED, handleEvent);
    EventBus::instance().subscribe(EventType::FACE_LOST, handleEvent);

    bootStep("Creating tasks...", 14);
    createTaskChecked(uiTask, "UI", UI_TASK_STACK_SIZE, UI_TASK_PRIORITY, &uiTaskHandle, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    createTaskChecked(touchTask, "Touch", TOUCH_TASK_STACK_SIZE, TOUCH_TASK_PRIORITY, &touchTaskHandle, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    createTaskChecked(cameraTask, "Camera", CAMERA_TASK_STACK_SIZE, CAMERA_TASK_PRIORITY, &cameraTaskHandle, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    createTaskChecked(visionTask, "Vision", VISION_TASK_STACK_SIZE, VISION_TASK_PRIORITY, &visionTaskHandle, 0);
    vTaskDelay(pdMS_TO_TICKS(50));
    createTaskChecked(aiTask, "AI", AI_TASK_STACK_SIZE, AI_TASK_PRIORITY, &aiTaskHandle, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    createTaskChecked(powerTask, "Power", POWER_TASK_STACK_SIZE, POWER_TASK_PRIORITY, &powerTaskHandle, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    createTaskChecked(networkTask, "Network", NETWORK_TASK_STACK_SIZE, NETWORK_TASK_PRIORITY, &networkTaskHandle, 1);
    vTaskDelay(pdMS_TO_TICKS(50));
    Serial.println("Setup complete!");
}

void loop() {
    static unsigned long lastHeartbeat = 0;
    unsigned long now = millis();
    if (SERIAL_DIAGNOSTIC_HEARTBEAT && millis() - lastHeartbeat >= SERIAL_HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = millis();
        Serial.printf("Alive: heap=%u, psram=%u, state=%d, wifi=%d, camera=%d\n",
                      ESP.getFreeHeap(), ESP.getFreePsram(),
                      static_cast<int>(AppState::instance().getState()),
                      gWifiManager.isConnected() ? 1 : 0,
                      gCameraManager.isRunning() ? 1 : 0);
    }

    AppStateEnum state = AppState::instance().getState();
    if (state == AppStateEnum::POMODORO) {
        gImuOrientation.update();
        PomoOrientation stable = gImuOrientation.getStable();
        gPomodoroUI.setOrientation(stable);
    }

    if (state == AppStateEnum::FACE) {
        gImuOrientation.update();
        if (gImuOrientation.isShaking()) {
            AppState::instance().setEmotion(FaceEmotion::SICK);
            gFaceUI.setTemporaryGaze(0.0f, 0.3f, 2000);
            gSickActive = true;
            gSickUntil = now + SICK_EMOTION_DURATION_MS;
            Serial.println("Shake detected -> SICK");
        }

        if (gSickActive && now >= gSickUntil) {
            gSickActive = false;
            if (AppState::instance().getEmotion() == FaceEmotion::SICK) {
                AppState::instance().setEmotion(FaceEmotion::NORMAL);
            }
        }
    }

    vTaskDelay(pdMS_TO_TICKS(100));
}
