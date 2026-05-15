#include "../secrets.h"

// Define Blynk macros FIRST
#define BLYNK_TEMPLATE_ID   SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME SECRET_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN    SECRET_BLYNK_AUTH_TOKEN
#define BLYNK_PRINT Serial
#define BLYNK_MAX_SENDBYTES 4096

#include <BlynkSimpleEsp32.h>

#include "NetworkManager.h"
#include "DisplayManager.h"
#include "AudioManager.h"
#include "ServoManager.h"
#include <WiFiManager.h>
#include <ArduinoOTA.h>

void blynkWrite(int vPin, int value) {
  Blynk.virtualWrite(vPin, value);
}

void checkAlarm() {
  if (!timeSynced || alarmHour < 0) return;

  int nowMin = cachedTime.tm_hour * 60 + cachedTime.tm_min;
  int alarmTime = alarmHour * 60 + alarmMin;

  if (abs(nowMin - alarmTime) <= 1) {
    if (cachedTime.tm_min != lastAlarmMinute) {
      lastAlarmMinute = cachedTime.tm_min;
      isAlarmRinging = true;
      playAudio(1, 1, PRIO_ALARM, true);
      setServo(S_ALARM);
    }
  }
}

void TaskTelegram(void *pvParameters) {
  Serial.print("TeleTask on Core ");
  Serial.println(xPortGetCoreID());
  
  for(;;) {
    if (WiFi.status() != WL_CONNECTED) {
      vTaskDelay(2000 / portTICK_PERIOD_MS);
      continue;
    }
    
    if (timeSynced) {
      int numNew = bot.getUpdates(bot.last_message_received + 1);
      if (sharedSend.hasPendingSend) {
        char msgOut[64];
        portENTER_CRITICAL(&sharedMux);
        strncpy(msgOut, sharedSend.msg, 64);
        sharedSend.hasPendingSend = false;
        portEXIT_CRITICAL(&sharedMux);
        
        bot.sendMessage(SECRET_CHAT_ID, msgOut, "");
        Serial.print("Sent: "); Serial.println(msgOut);
      }
      if (numNew > 0) {
        for (int i = 0; i < numNew; i++) {
          String chat_id = bot.messages[i].chat_id;
          String text = bot.messages[i].text;
          if (chat_id.equals(SECRET_CHAT_ID)) {
            if (text.equals("/status")) {
              bot.sendMessage(chat_id, "LoveBox V10.5 Online!", "");
            }
            else if (text.equals("/pomodoro")) {
              bot.sendMessage(chat_id, "OK! Dang bat Pomodoro...", "");
            }
            else {
              char reply[64];
              snprintf(reply, sizeof(reply), "Da nhan: %s", text.c_str());
              bot.sendMessage(chat_id, reply, "");
            }
            
            portENTER_CRITICAL(&sharedMux);
            strncpy(sharedMsg.msg, text.c_str(), MSG_BUFFER_SIZE - 1);
            sharedMsg.msg[MSG_BUFFER_SIZE - 1] = '\0';
            sharedMsg.timestamp = millis();
            sharedMsg.hasNewMsg = true;
            portEXIT_CRITICAL(&sharedMux);
          }
        }
      }
    }
    vTaskDelay(2000 / portTICK_PERIOD_MS);  
  }
}

// ================= [BLYNK CALLBACKS] =================
BLYNK_CONNECTED() {
  Blynk.syncAll();
}

BLYNK_WRITE(V0) {
  isPomodoro = param.asInt();
  if (isPomodoro) pomoStartTime = millis();
}

BLYNK_WRITE(V1) {
  TimeInputParam t(param);
  if (t.hasStartTime()) {
    alarmHour = t.getStartHour();
    alarmMin = t.getStartMinute();
    lastAlarmMinute = -1;
    char buf[32];
    sprintf(buf, "Luc %02d:%02d", alarmHour, alarmMin);
    setScreen("DAT BAO THUC", buf, 3000);
  } else {
    alarmHour = -1;
    alarmMin = -1;
    setScreen("BAO THUC", "Da tat!", 3000);
  }
}

BLYNK_WRITE(V2) {
  if (dfPlayerReady) {
    int vol = param.asInt();
    myDFPlayer.volume(vol);
    char buf[20];
    sprintf(buf, "Am luong: %d", vol);
    setScreen("VOLUME", buf, 2000);
  }
}

BLYNK_WRITE(V3) {
  if (param.asInt()) nextSong();
}

BLYNK_WRITE(V4) {
  if (param.asInt()) playAudio(2, currentSongID, PRIO_MUSIC, false);
  else stopAudio(PRIO_MUSIC);
}

BLYNK_WRITE(V5) {
  int id = param.asInt();
  if (id < 1 || id > TOTAL_SONGS) return;

  currentSongID = id;
  char nameBuf[64];
  char buf[80];

  strcpy_P(nameBuf, songName[id - 1]);
  snprintf(buf, sizeof(buf), "%03d - %s", id, nameBuf);

  setScreen("NOW PLAYING", buf, 4000);
  playAudio(MUSIC_FOLDER, id, PRIO_MUSIC, false);
  shuffleIndex = 0;
  if (isShuffle) generateShuffleList();
}

BLYNK_WRITE(V7) {
  currentNote = param.asString();
  setScreen("TODO LIST", "Da luu ghi chu!", 2000); 
}

BLYNK_WRITE(V8) {
  setShuffle(param.asInt());
}

BLYNK_WRITE(V9) {
  petMessage = param.asStr();      
  isShowingPet = true;               
  petDisplayStartTime = millis();  
  isShowingNote = false;            
  
  setServo(S_NOTIFY);                
  Serial.print("Pet noi: ");
  Serial.println(petMessage);
}

void setupNetworkAndBlynk() {
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
  u8g2.setCursor(10, 35);
  u8g2.print("Dang tim WiFi...");
  u8g2.sendBuffer();

  WiFi.mode(WIFI_STA);
  String savedSSID = WiFi.SSID(); 
  String savedPass = WiFi.psk();
  WiFi.persistent(false); 

  Serial.println("Thử mạng nhà Bạn Gái...");
  WiFi.begin(SECRET_WIFI_GF_SSID, SECRET_WIFI_GF_PASS);
  int timeout = 0;
  while (WiFi.status() != WL_CONNECTED && timeout < 10) { 
    delay(500); Serial.print("."); timeout++;
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nThử mạng nhà Bạn...");
    WiFi.begin(SECRET_WIFI_MY_SSID, SECRET_WIFI_MY_PASS);
    timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 10) { 
      delay(500); Serial.print("."); timeout++;
    }
  }

  if (WiFi.status() != WL_CONNECTED && savedSSID.length() > 0 
      && savedSSID != SECRET_WIFI_GF_SSID && savedSSID != SECRET_WIFI_MY_SSID) {
    Serial.println("\nThử mạng bên ngoài đã lưu: " + savedSSID);
    WiFi.begin(savedSSID.c_str(), savedPass.c_str());
    timeout = 0;
    while (WiFi.status() != WL_CONNECTED && timeout < 10) { 
      delay(500); Serial.print("."); timeout++;
    }
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("\nMở trạm phát Setup...");
    WiFi.persistent(true); 
    WiFiManager wm;
    
    wm.setAPCallback([](WiFiManager *myWiFiManager) {
      u8g2.clearBuffer();
      u8g2.setDrawColor(1);
      u8g2.drawBox(0, 0, 128, 14);
      u8g2.setDrawColor(0);
      u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
      u8g2.setCursor(20, 12);
      u8g2.print("Cai Dat WiFi");

      u8g2.setDrawColor(1);
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.setCursor(5, 30);
      u8g2.print("1. Mo dien thoai");
      u8g2.setCursor(5, 45);
      u8g2.print("2. Ket noi vao:");
      u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
      u8g2.setCursor(5, 60);
      u8g2.print("> LoveBox Setup");
      u8g2.sendBuffer();
    });

    wm.setConfigPortalTimeout(180); 
    if (!wm.startConfigPortal("LoveBox Setup")) {
      isSafeMode = true;
    }
  }

  if (!isSafeMode) {
    Serial.println("\nWiFi Connected!");
    WiFi.persistent(true); 
    
    u8g2.clearBuffer();
    u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
    u8g2.setCursor(15, 35);
    u8g2.print("Da co Internet!");
    u8g2.sendBuffer();
    delay(1000);

    client.setInsecure();
    client.setTimeout(3000);  
    Blynk.config(BLYNK_AUTH_TOKEN);
    configTime(7*3600, 0, "time.google.com");
    ArduinoOTA.setHostname("LoveBox-OTA");
    ArduinoOTA.begin();
  }
}

void processNetworkTasks() {
  Blynk.run();
  ArduinoOTA.handle();
  
  static unsigned long lastTimeCheck = 0;
  if (millis() - lastTimeCheck > 2000) {  
    time_t now = time(nullptr);
    if (now > 0) {
      localtime_r(&now, &cachedTime);
      if (cachedTime.tm_year > 100) timeSynced = true;
      checkAlarm();
    }
    lastTimeCheck = millis();
  }
}
