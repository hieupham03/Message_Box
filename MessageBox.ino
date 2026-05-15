#include "src/Globals.h"
#include "src/DisplayManager.h"
#include "src/AudioManager.h"
#include "src/ServoManager.h"
#include "src/TouchManager.h"
#include "src/NetworkManager.h"
#include <esp_task_wdt.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  WiFi.setSleep(false);
  u8g2.begin();
  u8g2.enableUTF8Print();
  Serial.begin(115200);
  delay(500);
  
  checkSafeMode();
  if (isSafeMode) {
    Serial.println("SAFE MODE - Restarting...");
    delay(3000);
    ESP.restart();
  }

  // Hardware Init
  myservo.attach(SERVO_PIN);
  myservo.write(90);
  
  // Audio Init
  myHardwareSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(500);
  if (myDFPlayer.begin(myHardwareSerial)) {
    dfPlayerReady = true;
    myDFPlayer.volume(5);
    myDFPlayer.EQ(DFPLAYER_EQ_POP);
    Serial.println("DFPlayer Ready");
  } else {
    Serial.println("DFPlayer Failed!");
  }

  // Network Init
  setupNetworkAndBlynk();

  // Start Core 0 Task
  if (!isSafeMode) {
    xTaskCreatePinnedToCore(
      TaskTelegram,
      "TeleTask",
      16384,  
      NULL,
      1,      
      NULL,
      0
    );
  }
  
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
  Serial.println("Setup Complete!");
}

void loop() {
  esp_task_wdt_reset();
  
  if (isSafeMode) {
    delay(1000);
    return;
  }
  
  // 1. Network & Time & Blynk
  processNetworkTasks();
  yield();
  markStable();

  // 2. Audio Processing Loop
  if (dfPlayerReady && currentAudioPrio != PRIO_IDLE) {
    static unsigned long lastDFCheck = 0;
    
    // Tự động lặp báo thức
    if (isAlarmRinging && currentAudioPrio == PRIO_ALARM) {
      static unsigned long lastAlarmReplay = 0;
      const unsigned long thoiGianBaiBaoThuc = 5000; 
      
      if (millis() - lastAlarmReplay > thoiGianBaiBaoThuc) { 
        myDFPlayer.playFolder(1, 1);
        lastAlarmReplay = millis();
        Serial.println("[AUDIO] Ép lặp lại báo thức!");
      }
    }

    // Tự động chuyển bài cho Nhạc
    if (millis() - lastDFCheck > 500) { 
      if (myDFPlayer.available()) {
        uint8_t type = myDFPlayer.readType();
        if (type == DFPlayerPlayFinished) { 
          if (currentAudioPrio == PRIO_MUSIC) {
            nextSong();
          } 
        }
      }
      lastDFCheck = millis();
    }
  }

  // 3. Process Touch
  handleTouch();
  
  // 4. Process Servo
  updateServo();
  
  // 5. Process Telegram Messages from Core 0
  if (sharedMsg.hasNewMsg) {
    char localMsg[MSG_BUFFER_SIZE];
    portENTER_CRITICAL(&sharedMux);
    strncpy(localMsg, sharedMsg.msg, MSG_BUFFER_SIZE);
    sharedMsg.hasNewMsg = false;
    portEXIT_CRITICAL(&sharedMux);

    if (strcmp(localMsg, "/pomodoro") == 0) {
      isPomodoro = true;
      pomoStartTime = millis();
      blynkWrite(0, 1);
      setScreen("TELEGRAM", "Pomodoro ON", 3000);
    } else {
      setScreen("TIN NHAN", localMsg, 10000);
      setServo(S_NOTIFY);
    }
  }

  // 6. UI Render
  static unsigned long lastRender = 0;
  if (millis() - lastRender > 200) {
    drawScreen();
    lastRender = millis();
  }
  
  // 7. Auto Reset 24h
  static unsigned long bootTime = millis();
  if (millis() - bootTime > 86400000UL) {
    Serial.println("24h reset");
    ESP.restart();
  }
  
  delay(10);
}