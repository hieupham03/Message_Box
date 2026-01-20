// ================= 1. BAO MAT (SECRETS) =================
// Gọi file chứa mật khẩu wifi/token (Phải nằm đầu tiên)
#include "secrets.h"
// ================= 2. CAU HINH BLYNK (MAPPING) =================
// Quan trọng: Phải định nghĩa các dòng này TRƯỚC KHI gọi thư viện Blynk
#define BLYNK_TEMPLATE_ID   SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME SECRET_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN    SECRET_BLYNK_AUTH_TOKEN
#define BLYNK_PRINT Serial

// ================= 2. THU VIEN =================
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>
#include <U8g2lib.h>
#include <Wire.h>
#include <ESP32Servo.h>
#include "time.h"

// Thu vien Dual Core
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Fix Brownout
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// ================= 3. SHARED DATA =================
volatile bool hasNewTelegramMessage = false; 
String sharedMsgText = "";                   
String sharedChatId = "";                    
portMUX_TYPE sharedDataMux = portMUX_INITIALIZER_UNLOCKED; 

// ================= 4. CAU HINH CO BAN =================
const char* ntpServer = "time.google.com";
const long gmtOffset_sec = 7 * 3600;
const int daylightOffset_sec = 0;

enum SystemMode { MODE_NORMAL, MODE_POMODORO };
enum ScreenState { SCREEN_BOOT, SCREEN_IDLE, SCREEN_MESSAGE, SCREEN_NOTIFICATION, SCREEN_SOS };

SystemMode currentMode = MODE_NORMAL;
ScreenState currentScreen = SCREEN_BOOT;

unsigned long screenStartTime = 0;
unsigned long screenDuration = 0;
unsigned long pomodoroStartTime = 0;
const unsigned long POMODORO_DURATION = 25 * 60 * 1000;

String globalTitleBuffer = "";
String globalContentBuffer = "";

// ================= 5. PHAN CUNG =================
Servo myservo;

struct tm startLoveDate = {0, 0, 0, 23, 1, 125}; // 14/02/2023

U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiClientSecure client;
UniversalTelegramBot bot(SECRET_BOT_TOKEN, client);

bool showClock = true;
unsigned long lastSwitchScreen = 0;

bool timeSynced = false;
struct tm cachedTime;
unsigned long lastTimeUpdate = 0;
unsigned long ntpStartMillis = 0;

// Touch Config
#define TOUCH_PIN 4
#define THRESHOLD 30
#define DEBOUNCE_DELAY 50

int currentTouchState = 0;
int lastTouchState = 0;
unsigned long lastDebounceTime = 0;
unsigned long touchStartTime = 0;
int clickCount = 0;
unsigned long lastClickTime = 0;
bool longPressTriggered = false;

// Servo State
enum ServoState { SERVO_IDLE, SERVO_NOTIFY, SERVO_KICK };
ServoState currentServoState = SERVO_IDLE;
unsigned long lastServoTime = 0;
int servoStep = 0;

// ================= 6. CORE 0 TASK: NETWORK WORKER =================
// Core 0 chiu trach nhiem HOAN TOAN viec gui/nhan tin nhan
void TaskTelegram(void *pvParameters) {
  Serial.print("TeleTask running on Core "); Serial.println(xPortGetCoreID());

  while (WiFi.status() != WL_CONNECTED) {
    vTaskDelay(500 / portTICK_PERIOD_MS);
  }

  for(;;) { 
    if (WiFi.status() == WL_CONNECTED && timeSynced) {
      
      int numNewMessages = bot.getUpdates(bot.last_message_received + 1);

      while (numNewMessages) {
        for (int i = 0; i < numNewMessages; i++) {
          String chat_id = String(bot.messages[i].chat_id);
          String text = bot.messages[i].text;
          
          if (chat_id == SECRET_CHAT_ID) {
            
            // 1. XU LY PHAN HOI NGAY TAI CORE 0 (De khong bi xung dot)
            if (text == "/status") {
               bot.sendMessage(chat_id, "Box V7.1 Online (Stable)!", "");
            } else if (text == "/pomodoro") {
               bot.sendMessage(chat_id, "OK! Dang chuyen che do...", "");
            } else {
               bot.sendMessage(chat_id, "Da nhan: " + text, "");
            }

            // 2. GUI DU LIEU SANG CORE 1 DE HIEN THI
            portENTER_CRITICAL(&sharedDataMux); 
            sharedMsgText = text;
            sharedChatId = chat_id;
            hasNewTelegramMessage = true; 
            portEXIT_CRITICAL(&sharedDataMux);
          }
        }
        numNewMessages = bot.getUpdates(bot.last_message_received + 1);
      }
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS); // Nghi 1s
  }
}

// ================= 7. HELPERS (CORE 1) =================
void trySyncTime() {
  static unsigned long lastTry = 0;
  if (timeSynced) return;
  if (millis() - lastTry < 2000) return; 
  lastTry = millis();

  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  time_t now = time(nullptr);
  if (now > 1600000000) {
    localtime_r(&now, &cachedTime);
    timeSynced = true;
    Serial.println(">>> TIME SYNCED");
  } else if (millis() - ntpStartMillis > 40000) {
     // Fallback
     cachedTime.tm_year = 125; cachedTime.tm_mon = 1; cachedTime.tm_mday = 14;
     cachedTime.tm_hour = 0; cachedTime.tm_min = 0;
     timeSynced = true;
  }
}

void updateCachedTime() {
  if (!timeSynced) return;
  if (millis() - lastTimeUpdate >= 1000) {
    time_t now = time(nullptr);
    localtime_r(&now, &cachedTime);
    lastTimeUpdate = millis();
  }
}

void triggerServoNotify() {
  currentServoState = SERVO_NOTIFY; servoStep = 0; lastServoTime = millis();
}
void triggerServoKick() {
  if (currentServoState == SERVO_IDLE) { currentServoState = SERVO_KICK; servoStep = 0; lastServoTime = millis(); }
}

void handleServo() {
  unsigned long now = millis();
  switch (currentServoState) {
    case SERVO_IDLE:
      static unsigned long lastKeepAlive = 0;
      if (now - lastKeepAlive > 15000) { triggerServoKick(); lastKeepAlive = now; }
      break;
    case SERVO_NOTIFY:
      if (now - lastServoTime > 100) {
        lastServoTime = now;
        switch (servoStep) {
          case 0: myservo.write(60); break;
          case 1: myservo.write(120); break;
          case 2: myservo.write(60); break;
          case 3: myservo.write(120); break;
          case 4: myservo.write(90); currentServoState = SERVO_IDLE; break;
        }
        servoStep++;
      }
      break;
    case SERVO_KICK:
      if (now - lastServoTime > 150) {
        lastServoTime = now;
        switch (servoStep) {
          case 0: myservo.write(100); break;
          case 1: myservo.write(90); currentServoState = SERVO_IDLE; break;
        }
        servoStep++;
      }
      break;
  }
}

void setScreen(ScreenState state, unsigned long duration, String title = "", String content = "") {
  currentScreen = state; screenStartTime = millis(); screenDuration = duration;
  if (title != "") globalTitleBuffer = title;
  if (content != "") globalContentBuffer = content;
}

void drawGenericScreen(String title, String content) {
  u8g2.clearBuffer(); u8g2.drawRFrame(0, 0, 128, 64, 4); 
  u8g2.setDrawColor(1); u8g2.drawBox(3, 3, 122, 14);
  u8g2.setDrawColor(0); u8g2.setFont(u8g2_font_unifont_t_vietnamese2); 
  int w = u8g2.getStrWidth(title.c_str()); u8g2.setCursor((128 - w) / 2, 13); u8g2.print(title);
  u8g2.setDrawColor(1); u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
  int y = 30; int x = 6; int maxChar = 16;
  for (int i = 0; i < content.length(); i += maxChar) {
     String sub = content.substring(i, min((int)content.length(), i + maxChar));
     u8g2.setCursor(x, y); u8g2.print(sub); y += 14;
  }
  u8g2.sendBuffer();
}

void drawHeart(int x, int y) {
  if ((millis() / 500) % 2 == 0) {
    u8g2.drawDisc(x + 3, y + 3, 3); u8g2.drawDisc(x + 8, y + 3, 3);
    u8g2.drawTriangle(x, y + 5, x + 11, y + 5, x + 5, y + 11);
  }
}

int getDaysTogether() {
  time_t now = mktime(&cachedTime);
  time_t start = mktime(&startLoveDate);
  return max(0, (int)(difftime(now, start) / 86400));
}

void drawIdleScreen() {
  u8g2.clearBuffer(); u8g2.drawRFrame(0, 0, 128, 64, 4);
  if (!timeSynced) {
    u8g2.setFont(u8g2_font_profont12_tf); u8g2.setCursor(15, 32); u8g2.print("Cho Core 0 Sync...");
    u8g2.sendBuffer(); return;
  }
  if (currentMode == MODE_POMODORO) {
    unsigned long elapsedTime = millis() - pomodoroStartTime;
    long remainingTime = POMODORO_DURATION - elapsedTime;
    if (remainingTime <= 0) { remainingTime = 0; currentMode = MODE_NORMAL; Blynk.virtualWrite(V0, 0); }
    int minutes = (remainingTime / 1000) / 60; int seconds = (remainingTime / 1000) % 60;
    char buf[6]; sprintf(buf, "%02d:%02d", minutes, seconds);
    u8g2.setDrawColor(1); u8g2.drawBox(3, 3, 122, 14);
    u8g2.setDrawColor(0); u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
    u8g2.setCursor(30, 13); u8g2.print("TAP TRUNG!");
    u8g2.setDrawColor(1); u8g2.drawFrame(14, 25, 100, 6);
    int progress = map(elapsedTime, 0, POMODORO_DURATION, 0, 96); u8g2.drawBox(16, 27, progress, 2);
    u8g2.setFont(u8g2_font_logisoso24_tn); int w = u8g2.getStrWidth(buf);
    u8g2.setCursor((128 - w) / 2, 60); u8g2.print(buf);
  } else {
    if (millis() - lastSwitchScreen > 5000) { showClock = !showClock; lastSwitchScreen = millis(); }
    if (showClock) {
      char buf[6]; sprintf(buf, "%02d:%02d", cachedTime.tm_hour, cachedTime.tm_min);
      u8g2.setFont(u8g2_font_logisoso24_tn); int w = u8g2.getStrWidth(buf);
      u8g2.setCursor((128 - w) / 2, 42); u8g2.print(buf);
      u8g2.setFont(u8g2_font_profont12_tf); char dateStr[20];
      sprintf(dateStr, "%02d/%02d/%d", cachedTime.tm_mday, cachedTime.tm_mon + 1, cachedTime.tm_year + 1900);
      w = u8g2.getStrWidth(dateStr); u8g2.setCursor((128 - w) / 2, 58); u8g2.print(dateStr);
    } else {
      u8g2.setFont(u8g2_font_unifont_t_vietnamese2); u8g2.setCursor(25, 20); u8g2.print("Ben nhau");
      String d = String(getDaysTogether()) + " Days";
      u8g2.setFont(u8g2_font_logisoso16_tf); int w = u8g2.getStrWidth(d.c_str());
      u8g2.setCursor((128 - w) / 2, 50); u8g2.print(d);
      drawHeart(10, 35); drawHeart(110, 35);
    }
  }
  u8g2.sendBuffer();
}

void renderScreen() {
  if (currentScreen != SCREEN_IDLE && currentScreen != SCREEN_BOOT) {
    if (millis() - screenStartTime > screenDuration) currentScreen = SCREEN_IDLE; 
    else { drawGenericScreen(globalTitleBuffer, globalContentBuffer); return; }
  }
  if (currentScreen == SCREEN_IDLE) drawIdleScreen();
}

// ================= 8. LOGIC & CALLBACKS (CORE 1) =================
// Core 1 chi nhan du lieu hien thi, KHONG GUI TIN NHAN DI
void processSharedMessages() {
  if (hasNewTelegramMessage) {
    String msg, id;
    
    portENTER_CRITICAL(&sharedDataMux);
    msg = sharedMsgText;
    id = sharedChatId;
    hasNewTelegramMessage = false; 
    portEXIT_CRITICAL(&sharedDataMux);

    // Xu ly LOGIC UI tai day
    if (msg == "/pomodoro") {
      currentMode = MODE_POMODORO; pomodoroStartTime = millis(); Blynk.virtualWrite(V0, 1);
      setScreen(SCREEN_NOTIFICATION, 3000, "Lenh Tu Xa", "Pomodoro ON");
    } 
    else if (msg == "/status") {
      // Khong lam gi ca, Core 0 da reply roi, chi can hien thong bao nhe
      setScreen(SCREEN_NOTIFICATION, 3000, "System", "Status Checked");
    }
    else {
      setScreen(SCREEN_MESSAGE, 15000, "From: Hieu", msg);
      triggerServoNotify();
    }
  }
}

void onSingleClick() {
  triggerServoKick(); 
  if (currentMode == MODE_POMODORO) {
     currentMode = MODE_NORMAL; Blynk.virtualWrite(V0, 0);
     setScreen(SCREEN_NOTIFICATION, 2000, "Che do", "Dung Pomodoro");
  } else {
     setScreen(SCREEN_NOTIFICATION, 2000, "LoveBox", "Da cham!");
  }
}
void onDoubleClick() {
  if (currentMode == MODE_NORMAL) {
    currentMode = MODE_POMODORO; pomodoroStartTime = millis(); Blynk.virtualWrite(V0, 1);
    setScreen(SCREEN_NOTIFICATION, 2000, "Che do", "Pomodoro ON");
  } else {
    currentMode = MODE_NORMAL; Blynk.virtualWrite(V0, 0);
    setScreen(SCREEN_NOTIFICATION, 2000, "Che do", "Pomodoro OFF");
  }
}
void onLongPress() {
  setScreen(SCREEN_SOS, 3000, "SOS", "Dang gui SOS...");
  // LUU Y: SOS O DAY CHUA GUI DUOC VI CORE 1 KHONG DUOC DUNG MANG
  // De don gian, ta bo qua gui SOS tu Core 1 trong phien ban V7.1 nay
  // Hoac chi hien thi man hinh thoi.
  triggerServoNotify();
}

void handleTouch() {
  int raw = touchRead(TOUCH_PIN); int state = (raw < THRESHOLD);
  if (state != lastTouchState) lastDebounceTime = millis();
  lastTouchState = state;
  if (millis() - lastDebounceTime > DEBOUNCE_DELAY) {
    if (state != currentTouchState) {
      currentTouchState = state;
      if (state) { touchStartTime = millis(); longPressTriggered = false; }
      else if (!longPressTriggered) { clickCount++; lastClickTime = millis(); }
    }
  }
  if (currentTouchState && !longPressTriggered && millis() - touchStartTime > 3000) { onLongPress(); longPressTriggered = true; clickCount = 0; }
  if (clickCount && millis() - lastClickTime > 400 && !currentTouchState) { if (clickCount == 1) onSingleClick(); else onDoubleClick(); clickCount = 0; }
}

BLYNK_WRITE(V0) { 
  int val = param.asInt();
  if (val) { currentMode = MODE_POMODORO; pomodoroStartTime = millis(); setScreen(SCREEN_NOTIFICATION, 2000, "Che do", "Pomodoro ON"); } 
  else { currentMode = MODE_NORMAL; setScreen(SCREEN_NOTIFICATION, 2000, "Che do", "Pomodoro OFF"); }
}
BLYNK_WRITE(V2) { int vol = param.asInt(); setScreen(SCREEN_NOTIFICATION, 2000, "Am luong", "Muc: " + String(vol)); }

// ================= 9. SETUP & LOOP =================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  Serial.begin(115200);

  myservo.attach(SERVO_PIN); myservo.write(90);
  u8g2.begin(); u8g2.enableUTF8Print();

  u8g2.clearBuffer(); u8g2.setFont(u8g2_font_profont12_tf);
  u8g2.setCursor(10, 30); u8g2.print("Connecting WiFi..."); u8g2.sendBuffer();
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) delay(300);

  Blynk.config(BLYNK_AUTH_TOKEN);
  
  // FIX LOI SSL (QUAN TRONG)
  client.setInsecure(); 
  
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  ntpStartMillis = millis();

  // KHOI TAO CORE 0
  xTaskCreatePinnedToCore(TaskTelegram, "TelegramTask", 20000, NULL, 1, NULL, 0);
  
  currentScreen = SCREEN_IDLE;
}

void loop() {
  Blynk.run();
  handleTouch();
  trySyncTime();
  updateCachedTime();
  handleServo();

  processSharedMessages(); // Chi nhan lenh hien thi tu Core 0

  static unsigned long lastRender = 0;
  if (millis() - lastRender > 100) { renderScreen(); lastRender = millis(); }
  // Tu dong Reset sau moi 24 gio de giai phong RAM (Tranh tran bo nho)
if (millis() > 86400000) { // 24 * 60 * 60 * 1000 ms
  Serial.println("Daily Reset...");
  ESP.restart();
}
}
