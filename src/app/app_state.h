#pragma once

#include <functional>

enum class AppStateEnum {
    FACE,
    MENU,
    CAMERA_DEBUG,
    POMODORO,
    SLEEP
};

enum class FaceEmotion {
    NORMAL,
    HAPPY,
    CURIOUS,
    LISTENING,
    THINKING,
    SPEAKING,
    SURPRISED,
    SLEEPY,
    TRACKING
};

class AppState {
public:
    using StateCallback = std::function<void(AppStateEnum)>;

    static AppState& instance();

    void setState(AppStateEnum state);
    AppStateEnum getState() const;

    void setEmotion(FaceEmotion emotion);
    FaceEmotion getEmotion() const;

    void registerCallback(StateCallback cb);

private:
    AppState() = default;

    AppStateEnum currentState_ = AppStateEnum::FACE;
    FaceEmotion currentEmotion_ = FaceEmotion::NORMAL;
    StateCallback callback_ = nullptr;
};
