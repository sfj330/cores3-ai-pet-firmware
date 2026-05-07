#include "app_state.h"

AppState& AppState::instance() {
    static AppState inst;
    return inst;
}

void AppState::setState(AppStateEnum state) {
    if (currentState_ != state) {
        currentState_ = state;
        if (callback_) {
            callback_(state);
        }
    }
}

AppStateEnum AppState::getState() const {
    return currentState_;
}

void AppState::setEmotion(FaceEmotion emotion) {
    currentEmotion_ = emotion;
}

FaceEmotion AppState::getEmotion() const {
    return currentEmotion_;
}

void AppState::registerCallback(StateCallback cb) {
    callback_ = std::move(cb);
}
