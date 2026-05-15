#include "TouchManager.h"
#include "DisplayManager.h"
#include "AudioManager.h"
#include "ServoManager.h"
#include "NetworkManager.h"

// Touch global variables for logic
unsigned long touchStartTime = 0;
int clickCount = 0;
unsigned long lastClickTime = 0;
bool longPressTriggered = false;
int lastTouchState = 0;
int currentTouchState = 0;
unsigned long lastDebounceTime = 0;

#define TOUCH_THRESHOLD 30
#define DEBOUNCE_DELAY 35    
#define TOUCH_OFFSET 15

void onSingleClick() {
  if (isAlarmRinging) {
    isAlarmRinging = false;
    stopAudio(PRIO_ALARM);
    setServo(S_IDLE);
    setScreen("ALARM", STR_ALARM_OFF, 3000);
  } else if (currentAudioPrio == PRIO_MUSIC) {
    stopAudio(PRIO_MUSIC);
    blynkWrite(4, 0);
    setScreen("MUSIC", STR_MUSIC_PAUSE, 2000);
  } else {
    if (isShowingNote) {
      isShowingNote = false;
    } else {
      isShowingNote = true;
      noteDisplayStartTime = millis();
      setServo(S_NOTIFY); 
    }
  }
}

void onDoubleClick() {
  if (isAlarmRinging) return;
  
  if (petMessage.length() > 0) {
    isShowingPet = true;
    petDisplayStartTime = millis();
    isShowingNote = false; 
    setServo(S_NOTIFY); 
    Serial.println("Xem lai tin nhan Pet qua 2 cham");
  } else {
    setScreen("BÉ PET", "Chưa có tin mới", 2000);
    setServo(S_KICK);  
  }
}

void onLongPress() {
  if (isAlarmRinging) return;

  isPomodoro = !isPomodoro; 

  if (isPomodoro) {
    pomoStartTime = millis(); 
    setScreen("POMODORO", "Bat dau 25p!", 3000);
    setServo(S_NOTIFY); 
    
    if (currentAudioPrio == PRIO_MUSIC) {
      stopAudio(PRIO_MUSIC);
      blynkWrite(4, 0); 
    }
  } else {
    setScreen("POMODORO", "Da huy!", 2000);
    setServo(S_NOTIFY); 
  }
}

void handleTouch() {
  int samples[5];
  for (int i = 0; i < 5; i++) {
    samples[i] = touchRead(TOUCH_PIN);
  }
  
  for (int i = 0; i < 4; i++) {
    for (int j = i + 1; j < 5; j++) {
      if (samples[i] > samples[j]) {
        int temp = samples[i];
        samples[i] = samples[j];
        samples[j] = temp;
      }
    }
  }
  int raw = samples[2]; 

  static float baseline = 0;
  static bool firstRun = true;
  if (firstRun || baseline == 0) {
    baseline = raw; 
    firstRun = false;
  }

  static unsigned long lastPrintTime = 0;
  if (millis() - lastPrintTime > 300) { 
    lastPrintTime = millis();
  }

  static int touchScore = 0; 

  if (raw < (baseline - TOUCH_OFFSET)) {
    touchScore += 2; 
  } else {
    touchScore -= 2;
    baseline = (baseline * 0.95) + (raw * 0.05); 
  }
  
  if (touchScore > 12) touchScore = 12;
  if (touchScore < 0) touchScore = 0;
  
  int state = (touchScore >= 8); 

  if (state != lastTouchState) {
    lastDebounceTime = millis();
  }
  lastTouchState = state;
  
  if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY) {
    if (state != currentTouchState) {
      currentTouchState = state;
      
      if (currentTouchState) {
        touchStartTime = millis();
        longPressTriggered = false;
      } else {
        if (!longPressTriggered) {
          clickCount++;
          lastClickTime = millis();
        }
      }
    }
  }
  
  if (currentTouchState && !longPressTriggered && (millis() - touchStartTime > 3000)) {
    onLongPress();
    longPressTriggered = true;
    clickCount = 0; 
  }
  
  if (clickCount > 0 && (millis() - lastClickTime > 550) && !currentTouchState) {
    if (clickCount == 1) {
      onSingleClick();
    } else {
      onDoubleClick(); 
    }
    clickCount = 0;
  }
}
