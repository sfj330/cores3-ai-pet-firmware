#include "face_ui.h"
#include "config/app_config.h"
#include <M5CoreS3.h>
#include <cmath>

FaceUI::FaceUI() : canvas_(&M5.Lcd) {}

bool FaceUI::begin() {
    canvas_.setPsram(true);
    canvas_.setColorDepth(16);
    spriteReady_ = canvas_.createSprite(DISPLAY_WIDTH, DISPLAY_HEIGHT) != nullptr;
    if (!spriteReady_) {
        M5.Lcd.setTextColor(TFT_RED, TFT_BLACK);
        M5.Lcd.setCursor(0, 36);
        M5.Lcd.print("FaceUI sprite alloc failed");
        return false;
    }
    canvas_.fillSprite(TFT_BLACK);
    canvas_.pushSprite(0, 0);
    return true;
}

void FaceUI::setGazeOffset(float dx, float dy) {
    targetGazeX_ = dx;
    targetGazeY_ = dy;
}

void FaceUI::setExpression(int emotion) {
    currentEmotion_ = emotion;
}

void FaceUI::setSpeakingMouthOpen(bool open) {
    speakingMouthOpen_ = open;
}

void FaceUI::wake() {
    sleeping_ = false;
    brightness_ = 255;
    M5.Lcd.setBrightness(brightness_);
}

void FaceUI::update() {
    if (sleeping_ || !spriteReady_) return;

    updateBlink();
    updateGaze();
    updateSpeakingAnimation();
    drawFace();

    // Push the complete sprite to the display in one operation (no flicker)
    canvas_.pushSprite(0, 0);
}

void FaceUI::updateBlink() {
    unsigned long now = millis();

    if (isBlinking_) {
        if (now - blinkStartTime_ > BLINK_DURATION) {
            isBlinking_ = false;
            nextBlinkTime_ = now + random(BLINK_INTERVAL_MIN, BLINK_INTERVAL_MAX);
        }
    } else if (now >= nextBlinkTime_) {
        isBlinking_ = true;
        blinkStartTime_ = now;
    }
}

void FaceUI::updateGaze() {
    gazeX_ = gazeX_ * (1.0f - GAZE_SMOOTH_ALPHA) + targetGazeX_ * GAZE_SMOOTH_ALPHA;
    gazeY_ = gazeY_ * (1.0f - GAZE_SMOOTH_ALPHA) + targetGazeY_ * GAZE_SMOOTH_ALPHA;
}

void FaceUI::updateSpeakingAnimation() {
    if (speakingMouthOpen_ || currentEmotion_ == 4) { // SPEAKING = 4
        unsigned long now = millis();
        if (now - lastSpeakAnimTime_ > SPEAK_ANIM_INTERVAL) {
            lastSpeakAnimTime_ = now;
            // Random mouth open amount for speaking animation
            mouthOpenAmount_ = random(MOUTH_OPEN_MIN, MOUTH_OPEN_MAX) / (float)MOUTH_OPEN_MAX;
        }
    } else {
        mouthOpenAmount_ = 0.0f;
    }
}

void FaceUI::drawFace() {
    canvas_.fillSprite(TFT_BLACK);

    int cx = FACE_CENTER_X;
    int cy = FACE_CENTER_Y;

    drawEyes(cx, cy);
    drawEyebrows(cx, cy);
    drawMouth(cx, cy);
    drawCheeks(cx, cy);
}

void FaceUI::drawEyes(int cx, int cy) {
    int eyeY = cy + EYE_Y_OFFSET;
    int leftEyeX = cx - EYE_SPACING / 2;
    int rightEyeX = cx + EYE_SPACING / 2;

    // Gaze offset in pixels
    int pupilDx = (int)(gazeX_ * GAZE_MAX_OFFSET);
    int pupilDy = (int)(gazeY_ * GAZE_MAX_OFFSET);

    // Eye size modifiers per emotion
    int eyeW = EYE_WIDTH;
    int eyeH = EYE_HEIGHT;
    int pupilR = PUPIL_RADIUS;

    switch (currentEmotion_) {
        case 1: // HAPPY — squinted
            eyeH = EYE_HEIGHT * 2 / 3;
            break;
        case 2: // CURIOUS — wide
            eyeW = EYE_WIDTH + 4;
            eyeH = EYE_HEIGHT + 4;
            break;
        case 5: // SPEAKING — normal
            break;
        case 6: // SURPRISED — very wide
            eyeW = EYE_WIDTH + 8;
            eyeH = EYE_HEIGHT + 8;
            break;
        case 7: // SLEEPY — half closed
            eyeH = EYE_HEIGHT / 3;
            pupilR = 0;
            break;
        default: // NORMAL, LISTENING, THINKING, TRACKING
            break;
    }

    // Blink override
    if (isBlinking_) {
        eyeH = 2;
        pupilR = 0;
    }

    // Draw sclera (white of the eye)
    canvas_.fillCircle(leftEyeX, eyeY, eyeW / 2, TFT_WHITE);
    canvas_.fillCircle(rightEyeX, eyeY, eyeW / 2, TFT_WHITE);

    // Draw pupils
    if (pupilR > 0) {
        int lPupilX = leftEyeX + pupilDx;
        int rPupilX = rightEyeX + pupilDx;
        int pupilY = eyeY + pupilDy;

        // Clamp pupils within eye
        int maxPupilMove = eyeW / 2 - pupilR - 2;
        lPupilX = constrain(lPupilX, leftEyeX - maxPupilMove, leftEyeX + maxPupilMove);
        rPupilX = constrain(rPupilX, rightEyeX - maxPupilMove, rightEyeX + maxPupilMove);
        pupilY = constrain(pupilY, eyeY - maxPupilMove, eyeY + maxPupilMove);

        canvas_.fillCircle(lPupilX, pupilY, pupilR, TFT_BLACK);
        canvas_.fillCircle(rPupilX, pupilY, pupilR, TFT_BLACK);

        // Eye highlights
        canvas_.fillCircle(lPupilX - 2, pupilY - 2, 2, TFT_WHITE);
        canvas_.fillCircle(rPupilX - 2, pupilY - 2, 2, TFT_WHITE);

        // Draw eyelid lines on top
        canvas_.drawFastHLine(leftEyeX - eyeW / 2, eyeY - eyeH / 2, eyeW, TFT_DARKGREY);
        canvas_.drawFastHLine(rightEyeX - eyeW / 2, eyeY - eyeH / 2, eyeW, TFT_DARKGREY);
    }

    // Eyelid lines for SLEEPY
    if (currentEmotion_ == 7) {
        canvas_.drawFastHLine(leftEyeX - eyeW / 2, eyeY, eyeW, TFT_DARKGREY);
        canvas_.drawFastHLine(rightEyeX - eyeW / 2, eyeY, eyeW, TFT_DARKGREY);
    }
}

void FaceUI::drawEyebrows(int cx, int cy) {
    int browY = cy + BROW_Y_OFFSET;
    int leftBrowX = cx - EYE_SPACING / 2;
    int rightBrowX = cx + EYE_SPACING / 2;

    int lx1 = leftBrowX - BROW_WIDTH / 2;
    int lx2 = leftBrowX + BROW_WIDTH / 2;
    int rx1 = rightBrowX - BROW_WIDTH / 2;
    int rx2 = rightBrowX + BROW_WIDTH / 2;

    int ly = browY;
    int ry = browY;

    switch (currentEmotion_) {
        case 0: // NORMAL — flat
            break;
        case 1: // HAPPY — raised
            ly -= 4; ry -= 4;
            break;
        case 2: // CURIOUS — one raised
            ly -= 6; // left raised
            break;
        case 3: // LISTENING — neutral
            break;
        case 4: // THINKING — knit (angled down inward)
            lx1 = leftBrowX; lx2 = leftBrowX - BROW_WIDTH / 2;
            rx1 = rightBrowX; rx2 = rightBrowX + BROW_WIDTH / 2;
            ly += 2;
            break;
        case 5: // SPEAKING — moving
            ly += sin(millis() / 200.0f) * 2;
            ry += sin(millis() / 200.0f + 1.0f) * 2;
            break;
        case 6: // SURPRISED — very raised
            ly -= 8; ry -= 8;
            break;
        case 7: // SLEEPY — lowered
            ly += 6; ry += 6;
            break;
        default:
            break;
    }

    canvas_.drawLine(lx1, ly, lx2, ly, TFT_BLACK);
    canvas_.drawLine(rx1, ry, rx2, ry, TFT_BLACK);
}

void FaceUI::drawMouth(int cx, int cy) {
    int mouthY = cy + MOUTH_Y_OFFSET;

    // Mouth shape varies by emotion and speaking state
    switch (currentEmotion_) {
        case 0: // NORMAL — small straight line
            canvas_.drawFastHLine(cx - MOUTH_WIDTH / 2, mouthY, MOUTH_WIDTH, TFT_BLACK);
            break;
        case 1: // HAPPY — big upward arc
            canvas_.fillCircle(cx, mouthY, MOUTH_WIDTH / 2, TFT_BLACK);
            canvas_.fillRect(cx - MOUTH_WIDTH / 2, mouthY, MOUTH_WIDTH, MOUTH_WIDTH / 2, TFT_BLACK);
            break;
        case 2: // CURIOUS — small O shape
            canvas_.drawCircle(cx, mouthY, 5, TFT_BLACK);
            break;
        case 3: // LISTENING — slight smile
            {
                int smW = MOUTH_WIDTH / 2;
                for (int i = -smW; i <= smW; ++i) {
                    int yOff = (i * i) / (smW * 2) + 2;
                    canvas_.drawPixel(cx + i, mouthY + yOff, TFT_BLACK);
                }
            }
            break;
        case 4: // THINKING — pursed lip
            canvas_.drawCircle(cx, mouthY, 4, TFT_BLACK);
            break;
        case 5: // SPEAKING — animated open/close
            {
                int openH = (int)(mouthOpenAmount_ * MOUTH_OPEN_MAX);
                canvas_.fillCircle(cx, mouthY - openH / 2, MOUTH_WIDTH / 4, TFT_BLACK);
                canvas_.fillCircle(cx, mouthY + openH / 2, MOUTH_WIDTH / 4, TFT_BLACK);
                canvas_.fillRect(cx - MOUTH_WIDTH / 4, mouthY - openH / 2, MOUTH_WIDTH / 2, openH, TFT_BLACK);
            }
            break;
        case 6: // SURPRISED — big O shape
            canvas_.fillCircle(cx, mouthY, MOUTH_WIDTH / 3, TFT_BLACK);
            break;
        case 7: // SLEEPY — relaxed line
            canvas_.drawFastHLine(cx - MOUTH_WIDTH / 3, mouthY, MOUTH_WIDTH * 2 / 3, TFT_DARKGREY);
            break;
        default: // TRACKING — neutral
            canvas_.drawFastHLine(cx - MOUTH_WIDTH / 2, mouthY, MOUTH_WIDTH, TFT_BLACK);
            break;
    }
}

void FaceUI::drawCheeks(int cx, int cy) {
    // Draw subtle blush for certain emotions
    if (currentEmotion_ == 1 || currentEmotion_ == 6) { // HAPPY or SURPRISED
        int cheekY = cy + EYE_Y_OFFSET + EYE_HEIGHT / 2;
        int leftCheekX = cx - EYE_SPACING / 2 - EYE_WIDTH / 2 - 8;
        int rightCheekX = cx + EYE_SPACING / 2 + EYE_WIDTH / 2 + 8;

        canvas_.fillCircle(leftCheekX, cheekY, 8, TFT_MAROON);
        canvas_.fillCircle(rightCheekX, cheekY, 8, TFT_MAROON);
    }
}
