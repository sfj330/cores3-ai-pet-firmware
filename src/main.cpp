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
#include "ai/xiaozhi_client.h"
#include "ai/voice_state.h"
#include "power/power_manager.h"

// ==================== Global Module Instances ====================
FaceUI gFaceUI;
MenuUI gMenuUI;
CameraDebugUI gCameraDebugUI;
PomodoroUI gPomodoroUI;
CameraManager gCameraManager;
FaceDetector gFaceDetector;
FaceTracker gFaceTracker;
XiaoZhiClient gXiaoZhiClient;
PowerManager gPowerManager;

// ==================== Shared State ====================
static float gCurrentFps = 0.0f;
static unsigned long gLastFpsCalc = 0;
static int gFrameCount = 0;

// ==================== FreeRTOS Task Handles ====================
static TaskHandle_t uiTaskHandle = nullptr;
static TaskHandle_t touchTaskHandle = nullptr;
static TaskHandle_t cameraTaskHandle = nullptr;
static TaskHandle_t visionTaskHandle = nullptr;
static TaskHandle_t aiTaskHandle = nullptr;
static TaskHandle_t powerTaskHandle = nullptr;
static SemaphoreHandle_t displayMutex = nullptr;

// ==================== Forward Declarations ====================
static void gestureEventHandler(GestureType gesture);
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

// ==================== UI Task ====================
void uiTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / 20); // 20 FPS

    while (true) {
        unsigned long now = millis();
        AppStateEnum state = AppState::instance().getState();
        FaceEmotion emotion = AppState::instance().getEmotion();

        if (takeDisplayLock()) {
            switch (state) {
                case AppStateEnum::FACE:
                    gFaceUI.setExpression(static_cast<int>(emotion));
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

// ==================== Touch Task ====================
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

// ==================== Camera Task ====================
void cameraTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / CAMERA_FPS);

    while (true) {
        if (gCameraManager.isRunning()) {
            CameraFrame frame = gCameraManager.getDisplayFrame();

            if (frame.valid) {
                if (AppState::instance().getState() == AppStateEnum::CAMERA_DEBUG) {
                    if (takeDisplayLock(pdMS_TO_TICKS(5))) {
                        M5.Lcd.pushImage(0, 0, frame.width, frame.height, (uint16_t*)frame.data);
                        giveDisplayLock();
                    }
                }
            }
        }

        vTaskDelay(delayTicks);
    }
}

// ==================== Vision Task ====================
void visionTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(1000 / DETECTION_FPS);

    while (true) {
        AppStateEnum state = AppState::instance().getState();

        if (state == AppStateEnum::FACE || state == AppStateEnum::CAMERA_DEBUG) {
            if (gCameraManager.isRunning() && gFaceDetector.isEnabled()) {
                CameraFrame frame = gCameraManager.getDetectionFrame();
                if (frame.valid) {
                    FaceResult face = gFaceDetector.detect(frame.data, frame.width, frame.height);
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

// ==================== AI Task ====================
void aiTask(void* pvParameters) {
    const TickType_t delayTicks = pdMS_TO_TICKS(50);

    while (true) {
        if (gXiaoZhiClient.isConnected()) {
            gXiaoZhiClient.process();
        }

        vTaskDelay(delayTicks);
    }
}

// ==================== Power Task ====================
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

// ==================== Gesture Event Handler ====================
static void gestureEventHandler(GestureType gesture) {
    AppState& appState = AppState::instance();
    AppStateEnum currentState = appState.getState();

    Event gestureEvent;
    gestureEvent.type = EventType::GESTURE_EVENT;
    gestureEvent.intParam = static_cast<int>(gesture);
    EventBus::instance().publish(gestureEvent);

    switch (currentState) {
        case AppStateEnum::FACE:
            switch (gesture) {
                case GestureType::RIGHT_SWIPE:
                    appState.setState(AppStateEnum::MENU);
                    break;
                case GestureType::SINGLE_TAP:
                    appState.setEmotion(FaceEmotion::CURIOUS);
                    break;
                case GestureType::DOUBLE_TAP:
                    appState.setEmotion(FaceEmotion::LISTENING);
                    if (gXiaoZhiClient.isConnected()) {
                        gXiaoZhiClient.startListening();
                    }
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

        case AppStateEnum::MENU:
            switch (gesture) {
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::FACE);
                    break;
                case GestureType::RIGHT_SWIPE:
                    gMenuUI.setActiveCard(gMenuUI.getActiveCard() + 1);
                    break;
                case GestureType::SINGLE_TAP:
                    switch (gMenuUI.getActiveCard()) {
                        case 0: break;
                        case 1: appState.setState(AppStateEnum::CAMERA_DEBUG); break;
                        case 2: appState.setState(AppStateEnum::POMODORO); break;
                        case 3: break;
                    }
                    break;
                default:
                    break;
            }
            break;

        case AppStateEnum::CAMERA_DEBUG:
            switch (gesture) {
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::MENU);
                    break;
                case GestureType::SINGLE_TAP:
                    gCameraDebugUI.setDetectionOverlay(!gCameraDebugUI.getDetectionOverlay());
                    break;
                default:
                    break;
            }
            break;

        case AppStateEnum::POMODORO:
            switch (gesture) {
                case GestureType::LEFT_SWIPE:
                    appState.setState(AppStateEnum::MENU);
                    break;
                case GestureType::SINGLE_TAP:
                    gPomodoroUI.togglePause();
                    break;
                default:
                    break;
            }
            break;

        case AppStateEnum::SLEEP:
            switch (gesture) {
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

// ==================== State Change Handler ====================
static void stateChangeHandler(AppStateEnum state) {
    switch (state) {
        case AppStateEnum::FACE:
            gMenuUI.hide();
            gCameraDebugUI.hide();
            gPomodoroUI.hide();
            AppState::instance().setEmotion(FaceEmotion::NORMAL);
            if (!gFaceDetector.isEnabled()) {
                gCameraManager.stopCapture();
                gCameraDebugUI.setCameraReady(false);
            }
            break;

        case AppStateEnum::MENU:
            if (!gFaceDetector.isEnabled()) {
                gCameraManager.stopCapture();
                gCameraDebugUI.setCameraReady(false);
            }
            gMenuUI.show();
            break;

        case AppStateEnum::CAMERA_DEBUG:
            gCameraDebugUI.show();
            if (!gCameraManager.isRunning()) {
                bool cameraReady = gCameraManager.isInitialized() || gCameraManager.begin();
                cameraReady = cameraReady && gCameraManager.startCapture();
                gCameraDebugUI.setCameraReady(cameraReady);
                Serial.println(cameraReady ? "Camera debug started" : "Camera debug start failed");
            }
            break;

        case AppStateEnum::POMODORO:
            if (!gFaceDetector.isEnabled()) {
                gCameraManager.stopCapture();
                gCameraDebugUI.setCameraReady(false);
            }
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

// ==================== AI State Change Handler ====================
static void aiStateHandler(VoiceState voiceState) {
    AppState& appState = AppState::instance();
    switch (voiceState) {
        case VoiceState::IDLE:       appState.setEmotion(FaceEmotion::NORMAL); break;
        case VoiceState::LISTENING:  appState.setEmotion(FaceEmotion::LISTENING); break;
        case VoiceState::THINKING:   appState.setEmotion(FaceEmotion::THINKING); break;
        case VoiceState::SPEAKING:   appState.setEmotion(FaceEmotion::SPEAKING); break;
        case VoiceState::ERROR:      appState.setEmotion(FaceEmotion::SURPRISED); break;
    }
    Event aiEvent;
    aiEvent.type = EventType::AI_STATE_CHANGE;
    aiEvent.intParam = static_cast<int>(voiceState);
    EventBus::instance().publish(aiEvent);
}

// ==================== Low Battery Handler ====================
static void lowBatteryHandler(float voltage) {
    AppState& appState = AppState::instance();
    if (appState.getState() == AppStateEnum::FACE) {
        appState.setEmotion(FaceEmotion::SLEEPY);
    }
}

// ==================== Event Bus Callback ====================
static void handleEvent(const Event& event) {
    // Future: additional logic
}

// Helper: show debug step on screen during boot
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

// ==================== Setup ====================
void setup() {
    Serial.begin(115200);
    unsigned long serialWaitStart = millis();
    while (!Serial && millis() - serialWaitStart < 1000) {
        delay(10);
    }
    Serial.println("\n\n=== CoreS3 AI Pet Boot ===");

    // Step 1: Init display
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

    // Step 2: AppState
    bootStep("AppState", 1);
    AppState::instance().registerCallback(stateChangeHandler);

    // Step 3: FaceUI (sprite creation - 150KB, needs PSRAM)
    bootStep("FaceUI sprite...", 2);
    bool faceUiOk = gFaceUI.begin();
    bootStep(faceUiOk ? "FaceUI OK" : "FaceUI FAIL", 2);

    // Step 4: MenuUI
    bootStep("MenuUI...", 3);
    gMenuUI.begin();
    bootStep("MenuUI OK", 3);

    // Step 5: Camera debug UI
    bootStep("CameraDebugUI...", 4);
    gCameraDebugUI.begin();

    // Step 6: Pomodoro UI
    bootStep("PomodoroUI...", 5);
    gPomodoroUI.begin();

    // Step 7: CameraManager
    bootStep("Camera deferred", 6);
    if (CAMERA_INIT_ON_BOOT) {
        bool camOk = gCameraManager.begin();
        gCameraManager.stopCapture();
        bootStep(camOk ? "Camera OK" : "Camera FAIL (non-fatal)", 6);
    }

    // Step 8: FaceDetector
    bootStep("FaceDetector...", 7);
    gFaceDetector.begin();
    bootStep("FaceDetector OK", 7);

    // Step 9: AI client
    bootStep("XiaoZhiClient...", 8);
    gXiaoZhiClient.begin();
    gXiaoZhiClient.setStateCallback(aiStateHandler);
    bootStep("XiaoZhiClient OK", 8);

    // Step 10: Power
    bootStep("PowerManager...", 9);
    gPowerManager.begin();
    gPowerManager.setLowBatteryCallback(lowBatteryHandler);
    bootStep("PowerManager OK", 9);

    // Step 11: EventBus
    bootStep("EventBus...", 10);
    EventBus::instance().subscribe(EventType::FACE_DETECTED, handleEvent);
    EventBus::instance().subscribe(EventType::FACE_LOST, handleEvent);

    // Step 12: Create tasks (with delay between each for scheduler stability)
    bootStep("Creating tasks...", 11);
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
    Serial.println("Setup complete!");
}

// ==================== Main Loop ====================
void loop() {
    // loop() runs as a low-priority task on core 1
    // All heavy work is done in FreeRTOS tasks
    static unsigned long lastHeartbeat = 0;
    if (SERIAL_DIAGNOSTIC_HEARTBEAT && millis() - lastHeartbeat >= SERIAL_HEARTBEAT_INTERVAL_MS) {
        lastHeartbeat = millis();
        Serial.printf("Alive: heap=%u, psram=%u, state=%d, camera=%d\n",
                      ESP.getFreeHeap(), ESP.getFreePsram(),
                      static_cast<int>(AppState::instance().getState()),
                      gCameraManager.isRunning() ? 1 : 0);
    }
    vTaskDelay(pdMS_TO_TICKS(100));
}
