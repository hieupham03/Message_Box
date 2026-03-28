

enum AudioPriority {
  PRIO_IDLE = 0,
  PRIO_MUSIC = 1,
  PRIO_NOTIFICATION = 2,
  PRIO_ALARM = 3
};

enum ServoMode { 
  S_IDLE, 
  S_NOTIFY, 
  S_ALARM, 
  S_KICK 
};

// ================= [1. SECRETS & CONFIG] =================

#include "secrets.h"
#define BLYNK_TEMPLATE_ID   SECRET_BLYNK_TEMPLATE_ID
#define BLYNK_TEMPLATE_NAME SECRET_BLYNK_TEMPLATE_NAME
#define BLYNK_AUTH_TOKEN    SECRET_BLYNK_AUTH_TOKEN
#define BLYNK_PRINT Serial
#define BLYNK_MAX_SENDBYTES 4096
#include <WiFi.h>
#include <ArduinoOTA.h>
#include <WiFiClientSecure.h>
#include <BlynkSimpleEsp32.h>
#include <UniversalTelegramBot.h>
#include <U8g2lib.h>
  #include <ESP32Servo.h>
#include "time.h"
#include "DFRobotDFPlayerMini.h"
#include <esp_task_wdt.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"
// --- BIẾN CHO GHI CHÚ (TODO LIST) ---
String currentNote = "Chua co ghi chu"; 
bool isShowingNote = false; 
// --- BIẾN CHO PET (AI) ---
String petMessage = "";           // Lưu tin nhắn từ AI
unsigned long petDisplayStartTime = 0; // Thời điểm nhận tin
bool isShowingPet = false;        // Cờ kiểm soát hiển thị Pet
unsigned long noteDisplayStartTime = 0;

// --- USER CONFIG ---
const char STR_ALARM_TITLE[]  = "BAO THUC!";
const char STR_ALARM_MSG[]    = "Day di em yeu oi!";
const char STR_ALARM_OFF[]    = "Da tat bao thuc";
const char STR_POMO_ON[]      = "Pomodoro ON";
const char STR_POMO_OFF[]     = "Pomodoro OFF";
const char STR_POMO_DONE[]    = "Hoan thanh!";
const char STR_TOUCHED[]      = "Da cham!";
const char STR_MUSIC_PAUSE[]  = "Tam dung nhac";

struct tm startLoveDate = {0, 0, 0, 23, 1, 125}; // 23/02/2025

#define ALARM_FOLDER  1
#define MUSIC_FOLDER  2
#define TOTAL_SONGS   100
int shuffleList[TOTAL_SONGS];
int shuffleIndex = 0;
bool isShuffle = false;
int currentSongID = 1;

const char songName[][64] PROGMEM = {
  "1. Don’t Say You Love Me",
  "2. Magnetic",
  "3. we can't be friends (wait for your love)",
  "4. Lies",
  "5. Blue",
  "6. Young And Beautiful",
  "7. Love Yourself",
  "8. LET'S NOT FALL IN LOVE",
  "9. One Last Time",
  "10. deja vu",
  "11. SOBER",
  "12. BAE BAE",
  "13. Cheating on You",
  "14. PHÓNG ZÌN ZÌN",
  "15. IF YOU",
  "16. Gieo Quẻ (feat. Đen)",
  "17. Bad Boy",
  "18. Love Story (Taylor’s Version)",
  "19. Lover",
  "20. shhhhhhh..",
  "21. Espresso",
  "22. Nonsense",
  "23. I Like Me Better",
  "24. Fantastic Baby",
  "25. Enchanted (Taylor's Version)",
  "26. Summertime Sadness",
  "27. Haru Haru",
  "28. drivers license",
  "29. FXXK IT",
  "30. Payphone",
  "31. Cruel Summer",
  "32. happier",
  "33. Still Life",
  "34. Please Please Please",
  "35. Super Shy",
  "36. boyfriend (with Social House)",
  "37. Cherish (My Love)",
  "38. Lucky Girl Syndrome",
  "39. Taste",
  "40. Die With A Smile",
  "41. Tonight",
  "42. Theo em về nhà",
  "43. Em Xinh",
  "44. Tiến Hay Lùi (Nụ Hôn Bạc Tỷ Original Soundtrack)",
  "45. Spring Day",
  "46. Đừng Làm Trái Tim Anh Đau",
  "47. Baby",
  "48. thank u, next",
  "49. YES or YES",
  "50. Sunset Glow",
  "51. The One That Got Away",
  "52. Sorry",
  "53. What Do You Mean?",
  "54. Sugar",
  "55. I Need U",
  "56. Best Of Me",
  "57. Call Me Maybe",
  "58. Beauty And A Beat",
  "59. Treat You Better",
  "60. Hype Boy",
  "61. Maps",
  "62. Ditto",
  "63. 34+35",
  "64. Stuck with U (with Justin Bieber)",
  "65. I.F.L.Y.",
  "66. Ghost",
  "67. On The Ground",
  "68. double take",
  "69. Try Again",
  "70. STAY (with Justin Bieber)",
  "71. Left and Right (Feat. Jung Kook of BTS)",
  "72. Touch",
  "73. intro (end of the world)",
  "74. Make It Right",
  "75. L’Amour, Les Baguettes, Paris",
  "76. Pure Imagination",
  "77. Honey, There’s the Door",
  "78. Car's Outside",
  "79. You Were Beautiful",
  "80. Yellow",
  "81. Vọng Nguyệt",
  "82. Tát Nước Đầu Đình",
  "83. Photograph",
  "84. HOME",
  "85. Never Be the Same",
  "86. BANG BANG BANG",
  "87. LOSER",
  "88. Say Yes",
  "89. Rewrite The Stars (with James Arthur & Anne-Marie)",
  "90. Xa (Chờ Đến Mùa Gió)",
  "91. TT",
  "92. Say Yes To Heaven",
  "93. Butterfly",
  "94. Spring Snow",
  "95. Airplane pt.2",
  "96. 눈,코,입 (Eyes, Nose, Lips)",
  "97. Dangerously",
  "98. WHISTLE",
  "99. As If It's Your Last",
  "100. Mystery of Love"
};

// --- SAFE MODE SYSTEM ---
#define WDT_TIMEOUT 30
RTC_DATA_ATTR int crashCount = 0;
bool isSafeMode = false;

void checkSafeMode() {
  esp_reset_reason_t reason = esp_reset_reason();

  if (reason == ESP_RST_PANIC ||
      reason == ESP_RST_TASK_WDT ||
      reason == ESP_RST_INT_WDT) {
    crashCount++;
  } else {
    crashCount = 0;
  }

  if (crashCount >= 3) {
    isSafeMode = true;
    Serial.println("!!! SAFE MODE ACTIVATED !!!");
  }
}

void markStable() {
  if (millis() > 60000 && crashCount > 0) {
    crashCount = 0;
    Serial.println("System stable - crash counter reset");
  }
}

// ================= [3. SHARED DATA (THREAD-SAFE)] =================
#define MSG_BUFFER_SIZE 128

// 1. Dinh nghia kieu du lieu (Structs)
typedef struct {
  volatile bool hasNewMsg;
  char msg[MSG_BUFFER_SIZE];
  volatile uint32_t timestamp;
} SharedIncoming;

typedef struct {
  volatile bool hasPendingSend;
  char msg[64];
} SharedOutgoing;

// 2. Khoi tao bien chia se
SharedIncoming sharedMsg = { false, {0}, 0 };
SharedOutgoing sharedSend = { false, {0} };

// 3. Khoi tao Mutex de chong xung dot
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;

// ================= [4. HARDWARE OBJECTS] =================
#define RXD2 32
#define TXD2 33 
#define SERVO_PIN 15
#define TOUCH_PIN 4


HardwareSerial myHardwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;
Servo myservo;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiClientSecure client;
UniversalTelegramBot bot(SECRET_BOT_TOKEN, client);

// ================= [5. AUDIO SYSTEM] =================
AudioPriority currentAudioPrio = PRIO_IDLE;
bool audioLocked = false;
volatile bool dfPlayerReady = false;  // FIX: volatile for thread-safe

void playAudio(int folder, int file, AudioPriority prio, bool loop = false) {

  if (isSafeMode || !dfPlayerReady) return;
  if (audioLocked && prio < PRIO_ALARM) return;
  if (!audioLocked && prio < currentAudioPrio) return;

  if (prio == PRIO_ALARM) {
    audioLocked = true;
  }
  currentAudioPrio = prio;

  myDFPlayer.playFolder(folder, file);

  // 4. Chờ 0.1s để con chip MP3 kịp nạp bài hát vào bộ nhớ
  delay(100); 
  if (prio == PRIO_ALARM) {
    myDFPlayer.enableLoop();
  } else {
    if (loop) myDFPlayer.enableLoop();
    else myDFPlayer.disableLoop();
  }
  
  Serial.printf("[AUDIO] Play F:%d File:%d Prio:%d\n", folder, file, prio);
}

void stopAudio(AudioPriority prio) {
  if (isSafeMode || !dfPlayerReady) return;
  if (prio >= currentAudioPrio) {
    myDFPlayer.pause();
    myDFPlayer.disableLoop();
    audioLocked = false;
    currentAudioPrio = PRIO_IDLE;
    Serial.println("[AUDIO] Stopped");
  }
}

// ================= [6. MUSIC PLAYER] =================
void playSong(int id) {
  playAudio(2, id, PRIO_MUSIC, false);
  Blynk.virtualWrite(V5, id);
  Blynk.virtualWrite(V4, 1);
}

void generateShuffleList() {
  for (int i = 0; i < TOTAL_SONGS; i++) {
    shuffleList[i] = i + 1;
  }

  for (int i = TOTAL_SONGS - 1; i > 0; i--) {
    int j = random(0, i + 1);
    int tmp = shuffleList[i];
    shuffleList[i] = shuffleList[j];
    shuffleList[j] = tmp;
  }

  if (shuffleList[0] == currentSongID && TOTAL_SONGS > 1) {
    int swapIdx = random(1, TOTAL_SONGS);
    int t = shuffleList[0];
    shuffleList[0] = shuffleList[swapIdx];
    shuffleList[swapIdx] = t;
  }

  shuffleIndex = 0;
}

void nextSong() {
  if (isShuffle) {
    if (shuffleIndex >= TOTAL_SONGS) {
      generateShuffleList();
    }
    currentSongID = shuffleList[shuffleIndex++];
  } else {
    currentSongID++;
    if (currentSongID > TOTAL_SONGS) currentSongID = 1;
  }

  char buf[64];
  char nameBuf[64];
  strcpy_P(nameBuf, songName[currentSongID - 1]);
  snprintf(buf, sizeof(buf), "%03d - %s", currentSongID, nameBuf);

  setScreen("NOW PLAYING", buf, 3000);
  playAudio(MUSIC_FOLDER, currentSongID, PRIO_MUSIC, false);
}

void setShuffle(bool enable) {
  isShuffle = enable;
  if (isShuffle) {
    generateShuffleList();
  }
  setScreen("MODE", isShuffle ? "Shuffle ON" : "Shuffle OFF", 2000);
}

// ================= [7. SERVO MANAGER] =================
ServoMode servoState = S_IDLE;
unsigned long servoStartTime = 0;
unsigned long lastServoStep = 0;
int servoStep = 0;

void setServo(ServoMode mode) {
  if (isSafeMode) return;
  if (servoState == S_ALARM && mode != S_IDLE && mode != S_ALARM) return;

  servoState = mode;
  servoStep = 0;
  servoStartTime = millis();
  lastServoStep = millis();
}

void updateServo() {
  if (isSafeMode) return;
  unsigned long now = millis();
  
  if (servoState == S_ALARM && now - servoStartTime > 30000) {
    servoState = S_IDLE;
    myservo.write(88);
    return;
  }

  switch (servoState) {
    case S_IDLE:
      if (now - lastServoStep > 15000) setServo(S_KICK);
      break;
      
    case S_NOTIFY:
      if (now - lastServoStep > 600) {
        lastServoStep = now;
        if (servoStep == 0) myservo.write(60);
        else if (servoStep == 1) myservo.write(115);
        else if (servoStep == 2) myservo.write(60);
        else if (servoStep == 3) myservo.write(115);
        else { myservo.write(88); servoState = S_IDLE; }
        servoStep++;
      }
      break;
      
    case S_ALARM:
      if (now - lastServoStep > 500) {
        lastServoStep = now;
        if (servoStep % 2 == 0) myservo.write(60);
        else myservo.write(115);
        servoStep++;
      }
      break;
      
    case S_KICK:
      if (now - lastServoStep > 200) {
        lastServoStep = now;
        if (servoStep == 0) myservo.write(95);
        else { myservo.write(88); servoState = S_IDLE; }
        servoStep++;
      }
      break;
  }
}

// ================= [8. UI SYSTEM] =================
struct tm cachedTime;
bool timeSynced = false;
char screenTitle[32];
char screenContent[128] = "";
unsigned long screenEndTime = 0;
bool showClock = true;
unsigned long lastSwitchScreen = 0;

bool isPomodoro = false;
unsigned long pomoStartTime = 0;
const unsigned long POMO_DURATION = 25 * 60 * 1000;

int alarmHour = -1, alarmMin = -1;
static int lastAlarmMinute = -1;
bool isAlarmRinging = false;

int getDaysTogether() {
  if (!timeSynced) return 0;
  time_t now = mktime(&cachedTime);
  time_t start = mktime(&startLoveDate);
  return max(0, (int)(difftime(now, start) / 86400));
}

void drawHeart(int x, int y) {
  if ((millis() / 500) % 2 == 0) {
    u8g2.drawDisc(x + 3, y + 3, 3);
    u8g2.drawDisc(x + 8, y + 3, 3);
    u8g2.drawTriangle(x, y + 5, x + 11, y + 5, x + 5, y + 11);
  }
}

void setScreen(const char* title, const char* content, int duration) {
  strncpy(screenTitle, title, sizeof(screenTitle) - 1);
  screenTitle[sizeof(screenTitle) - 1] = '\0';

  strncpy(screenContent, content, sizeof(screenContent) - 1);
  screenContent[sizeof(screenContent) - 1] = '\0';

  screenEndTime = millis() + duration;
}
\

void drawScreen() {
  u8g2.clearBuffer();
  u8g2.drawRFrame(0, 0, 128, 64, 4);

  if (isAlarmRinging) {
    u8g2.setFont(u8g2_font_logisoso24_tn);
    u8g2.setCursor(15, 45);
    u8g2.print("ALARM!");
    u8g2.setFont(u8g2_font_profont12_tf);
    u8g2.setCursor(10, 60);
    u8g2.print(STR_ALARM_MSG);
  }
  else if (millis() < screenEndTime) {
    u8g2.setDrawColor(1);
    u8g2.drawBox(3, 3, 122, 14);
    u8g2.setDrawColor(0);
    u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
    int w = u8g2.getUTF8Width(screenTitle);
    u8g2.setCursor((128 - w) / 2, 13);
    u8g2.print(screenTitle);
    
    u8g2.setDrawColor(1);
    u8g2.setFont(u8g2_font_unifont_t_vietnamese1);
    int len = strlen(screenContent);
    if (len > 16) {
      char line1[17], line2[17];
      strncpy(line1, screenContent, 16);
      line1[16] = '\0';
      strncpy(line2, screenContent + 16, 16);
      line2[16] = '\0';

      u8g2.setCursor(5, 30);
      u8g2.print(line1);
      u8g2.setCursor(5, 45);
      u8g2.print(line2);
    } else {
      u8g2.setCursor(5, 35);
      u8g2.print(screenContent);
    }
    
  }
  else if (isPomodoro) {
    unsigned long elapsed = millis() - pomoStartTime;
    long remain = POMO_DURATION - elapsed;
    
    if (remain <= 0) {
      isPomodoro = false;
      Blynk.virtualWrite(V0, 0);
      setScreen("POMODORO", STR_POMO_DONE, 5000);
      playAudio(2, 3, PRIO_NOTIFICATION, false);
    } else {
      int min = (remain / 1000) / 60;
      int sec = (remain / 1000) % 60;
      char buf[6];
      sprintf(buf, "%02d:%02d", min, sec);

      u8g2.setDrawColor(1);
      u8g2.drawBox(3, 3, 122, 14);
      u8g2.setDrawColor(0);
      u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
      u8g2.setCursor(30, 13);
      u8g2.print("TAP TRUNG!");

      u8g2.setDrawColor(1);
      u8g2.drawFrame(14, 25, 100, 6);
      int prog = map(elapsed, 0, POMO_DURATION, 0, 96);
      u8g2.drawBox(16, 27, prog, 2);

      u8g2.setFont(u8g2_font_logisoso24_tn);
      int w = u8g2.getUTF8Width(buf);
      u8g2.setCursor((128 - w) / 2, 60);
      u8g2.print(buf);
    }
  }
  else {
    if (!timeSynced) {
      u8g2.setFont(u8g2_font_profont12_tf);
      u8g2.setCursor(15, 35);
      u8g2.print("Syncing Time...");
    } 
    else {
      
      if (isShowingPet && (millis() - petDisplayStartTime > 15000)) {
      isShowingPet = false;
    }

      if (isShowingNote && (millis() - noteDisplayStartTime > 15000)) {
        isShowingNote = false; 
      }
      if (isShowingPet) {
        u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
        
        unsigned long elapsed = millis() - petDisplayStartTime;
        int currentPage = elapsed / 5000;

        if (elapsed > 15000) {
          isShowingPet = false; 
        } else {
          u8g2.drawBox(0, 0, 128, 14);
          u8g2.setDrawColor(0); 
          char titleBuf[32];
          sprintf(titleBuf, "BÉ PET %d/3", currentPage + 1);
          int wTitle = u8g2.getUTF8Width(titleBuf);
          u8g2.setCursor((128 - wTitle) / 2, 11); 
          u8g2.print(titleBuf);
          u8g2.setDrawColor(1); 

          int y = 28; 
          int lineCount = 0;
          int linesPerPage = 3; 
          int startLine = currentPage * linesPerPage;
          int endLine = startLine + linesPerPage;

          String tempPet = petMessage;
          if (tempPet.length() == 0) tempPet = "Chưa có tin nhắn";

          while (tempPet.length() > 0 && lineCount < endLine) {
            int splitIndex = 22;
            if (tempPet.length() <= 22) {
              splitIndex = tempPet.length();
            } else {
              int spaceIdx = tempPet.lastIndexOf(' ', 22);
              if (spaceIdx > 0) splitIndex = spaceIdx; 
            }

            String line = tempPet.substring(0, splitIndex);
            tempPet = tempPet.substring(splitIndex);
            tempPet.trim();

            if (lineCount >= startLine) {
              // Thuật toán căn giữa màn hình cho từng dòng chữ
              int w = u8g2.getUTF8Width(line.c_str());
              u8g2.setCursor((128 - w) / 2, y); 
              u8g2.print(line);
              y += 14; 
            }
            lineCount++;
          }
        }
      }
      else if (isShowingNote) {
        // --- VẼ MÀN HÌNH TO-DO LIST (PHÂN 2 TRANG) ---
        u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
        
        unsigned long elapsed = millis() - noteDisplayStartTime;
        // 15 giây chia 2 trang -> mỗi trang 7.5 giây (7500ms)
        int currentPage = elapsed / 7500; 
        if (currentPage > 1) currentPage = 1; 

        // Vẽ dải băng đen chữ trắng cho Tiêu đề
        u8g2.drawBox(0, 0, 128, 14);
        u8g2.setDrawColor(0); 
        
        // Tạo dòng tiêu đề kèm số trang (VD: TO-DO LIST 1/2)
        char titleBuf[32];
        sprintf(titleBuf, "TO-DO LIST %d/2", currentPage + 1);
        int wTitle = u8g2.getUTF8Width(titleBuf);
        u8g2.setCursor((128 - wTitle) / 2, 11);
        u8g2.print(titleBuf);
        u8g2.setDrawColor(1); 
        // Thuật toán cắt dòng tự động & lật trang
        int y = 28; 
        int lineCount = 0;
        int linesPerPage = 3; 
        int startLine = currentPage * linesPerPage;
        int endLine = startLine + linesPerPage;

        String tempNote = currentNote;
        if (tempNote.length() == 0) tempNote = "Chua co ghi chu";

        while (tempNote.length() > 0 && lineCount < endLine) {
          int splitIndex = 22; 
          if (tempNote.length() <= 22) {
            splitIndex = tempNote.length();
          } else {
            int spaceIdx = tempNote.lastIndexOf(' ', 22);
            if (spaceIdx > 0) splitIndex = spaceIdx; 
          }
          
          String line = tempNote.substring(0, splitIndex);
          tempNote = tempNote.substring(splitIndex);
          tempNote.trim();

          if (lineCount >= startLine) {
            int w = u8g2.getUTF8Width(line.c_str());
            u8g2.setCursor((128 - w) / 2, y); 
            u8g2.print(line);
            y += 14; 
          }
          lineCount++; 
        }
      }
      else {
        if (millis() - lastSwitchScreen > 5000) {
          showClock = !showClock;
          lastSwitchScreen = millis();
        }
      
      if (showClock) {
        char buf[10];
        sprintf(buf, "%02d:%02d", cachedTime.tm_hour, cachedTime.tm_min);
        u8g2.setFont(u8g2_font_logisoso24_tn);
        int w = u8g2.getUTF8Width(buf);
        u8g2.setCursor((128 - w) / 2, 42);
        u8g2.print(buf);
        
        u8g2.setFont(u8g2_font_profont12_tf);
        char dateStr[20];
        sprintf(dateStr, "%02d/%02d/%d", cachedTime.tm_mday, cachedTime.tm_mon + 1, cachedTime.tm_year + 1900);
        w = u8g2.getUTF8Width(dateStr);
        u8g2.setCursor((128 - w) / 2, 58);
        u8g2.print(dateStr);
      } else {
        u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
        u8g2.setCursor(25, 20);
        u8g2.print("Ben nhau");
        
        char d[16];
        snprintf(d, sizeof(d), "%d Days", getDaysTogether());
        u8g2.setFont(u8g2_font_logisoso16_tf);
        int w = u8g2.getUTF8Width(d);
        u8g2.setCursor((128 - w) / 2, 50);
        u8g2.print(d);
        
        drawHeart(10, 35);
        drawHeart(110, 35);
      }
      }
      if (alarmHour >= 0) { 
        u8g2.setFont(u8g2_font_profont10_tf); 
        char aBuf[16];
        sprintf(aBuf, "%02d:%02d", alarmHour, alarmMin);
        
        int w = u8g2.getUTF8Width(aBuf);
        u8g2.setCursor(126 - w, 9);
        u8g2.print(aBuf);
                int bellX = 126 - w - 10;
        u8g2.drawBox(bellX + 2, 3, 4, 3);
        u8g2.drawTriangle(bellX + 2, 3, bellX + 6, 3, bellX + 4, 1);
        u8g2.drawBox(bellX, 6, 8, 2);
        u8g2.drawDisc(bellX + 4, 8, 1);
      }
    }
  }
  u8g2.sendBuffer();
}

// ================= [9. TOUCH GESTURES] =================
unsigned long touchStartTime = 0;
int clickCount = 0;
unsigned long lastClickTime = 0;
bool longPressTriggered = false;
int lastTouchState = 0;
int currentTouchState = 0;
unsigned long lastDebounceTime = 0;

void onSingleClick() {
  if (isAlarmRinging) {
    isAlarmRinging = false;
    stopAudio(PRIO_ALARM);
    setServo(S_IDLE);
    setScreen("ALARM", STR_ALARM_OFF, 3000);
  } else if (currentAudioPrio == PRIO_MUSIC) {
    stopAudio(PRIO_MUSIC);
    Blynk.virtualWrite(V4, 0);
    setScreen("MUSIC", STR_MUSIC_PAUSE, 2000);
  } 
  // 3. TÍNH NĂNG 1 CHẠM MỚI: Bật / Tắt To-do list 15 giây
  else {
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
  // 1. Bỏ qua nếu báo thức đang kêu
  if (isAlarmRinging) return;
  // 2. KIỂM TRA TIN NHẮN CỦA PET
  if (petMessage.length() > 0) {
    isShowingPet = true;
    petDisplayStartTime = millis();
    isShowingNote = false; 
    
    setServo(S_NOTIFY); 
    Serial.println("Xem lai tin nhan Pet qua 2 cham");
  } 
  else {
    setScreen("BÉ PET", "Chưa có tin mới", 2000);
    setServo(S_KICK);  
  }
}

void onLongPress() {
  // 1. Bỏ qua nếu báo thức đang kêu
  if (isAlarmRinging) return;

  // 2. Bật / Tắt Pomodoro
  isPomodoro = !isPomodoro; 

  if (isPomodoro) {
    // KHI BẬT: Bắt đầu tính giờ
    pomoStartTime = millis(); 
    setScreen("POMODORO", "Bat dau 25p!", 3000);
    setServo(S_NOTIFY); 
    
    // Tự động tắt nhạc để tập trung
    if (currentAudioPrio == PRIO_MUSIC) {
      stopAudio(PRIO_MUSIC);
      Blynk.virtualWrite(V4, 0); 
    }
  } else {
    // KHI TẮT SỚM
    setScreen("POMODORO", "Da huy!", 2000);
    setServo(S_NOTIFY); 
  }
}

#define TOUCH_THRESHOLD 18   
#define DEBOUNCE_DELAY 35    

void handleTouch() {
  int raw = touchRead(TOUCH_PIN);
  static int touchScore = 0; 

  // --- THUẬT TOÁN TÍCH LŨY TỐC ĐỘ CAO ---
  if (raw < TOUCH_THRESHOLD) {
    touchScore += 4;
  } else {
    touchScore -= 2;
  }
  
  if (touchScore > 12) touchScore = 12;
  if (touchScore < 0) touchScore = 0;
  int state = (touchScore >= 6); 

  // --- LOGIC DEBOUNCE & XỬ LÝ TRẠNG THÁI ---
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
// ================= [10. ALARM LOGIC] =================
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

// ================= [11. CORE 0: TELEGRAM TASK] =================
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
        
        // Lấy tin nhắn từ hàng đợi ra
        portENTER_CRITICAL(&sharedMux);
        strncpy(msgOut, sharedSend.msg, 64);
        sharedSend.hasPendingSend = false;
        portEXIT_CRITICAL(&sharedMux);
        
        // Thực hiện gửi qua Wifi
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
            // Chuyển sang Core 1
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

// ================= [12. BLYNK CALLBACKS] =================
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
// NHẬN GHI CHÚ TỪ APP BLYNK (CHÂN V7)
BLYNK_WRITE(V7) {
  currentNote = param.asString();
  setScreen("TODO LIST", "Da luu ghi chu!", 2000); 
}
BLYNK_WRITE(V8) {
  setShuffle(param.asInt());
}

// ================= [NHẬN TIN NHẮN TỪ BÉ PET QUA V9] =================
BLYNK_WRITE(V9) {
  petMessage = param.asStr();      
  isShowingPet = true;               
  petDisplayStartTime = millis();  
  isShowingNote = false;            
  
  setServo(S_NOTIFY);                
  Serial.print("Pet noi: ");
  Serial.println(petMessage);
}
// ================= [13. SETUP & LOOP] =================
void setup() {
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);
  WiFi.setSleep(false);
  esp_task_wdt_init(WDT_TIMEOUT, true);
  esp_task_wdt_add(NULL);
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
  
  u8g2.begin();
  u8g2.enableUTF8Print();
  
  // Audio Init
  myHardwareSerial.begin(9600, SERIAL_8N1, RXD2, TXD2);
  delay(500);
  if (myDFPlayer.begin(myHardwareSerial)) {
    dfPlayerReady = true;
    myDFPlayer.volume(20);
    myDFPlayer.EQ(DFPLAYER_EQ_POP);
    Serial.println("DFPlayer Ready");
  } else {
    Serial.println("DFPlayer Failed!");
  }

  // Network Init
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_profont12_tf);
  u8g2.setCursor(10, 30);
  u8g2.print("Connecting WiFi...");
  u8g2.sendBuffer();
  
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  int wifiTimeout = 0;
  while (WiFi.status() != WL_CONNECTED && wifiTimeout < 20) {
    delay(500);
    Serial.print(".");
    wifiTimeout++;
  }
  
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi Connected");
    client.setInsecure();
    client.setTimeout(3000);  
    Blynk.config(BLYNK_AUTH_TOKEN);
    configTime(7*3600, 0, "time.google.com");
    ArduinoOTA.setHostname("LoveBox-OTA");
    ArduinoOTA.begin();
  } else {
    Serial.println("\nWiFi Failed - Safe Mode");
    isSafeMode = true;
  }
  
  // Start Core 0 Task
  xTaskCreatePinnedToCore(
    TaskTelegram,
    "TeleTask",
    16384,  
    NULL,
    1,      
    NULL,
    0
  );

  Serial.println("Setup Complete!");
}

void loop() {
  esp_task_wdt_reset();
  
  if (isSafeMode) {
    delay(1000);
    return;
  }
  
  Blynk.run();
  ArduinoOTA.handle();
  yield();
  markStable();
  
  // Time Sync
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


  // XỬ LÝ ÂM THANH: Tự động chuyển bài HOẶC Ép lặp báo thức
  if (dfPlayerReady && currentAudioPrio != PRIO_IDLE) {
    static unsigned long lastDFCheck = 0;
    if (millis() - lastDFCheck > 500) { 
      if (myDFPlayer.available()) {
        uint8_t type = myDFPlayer.readType();
        if (type == DFPlayerPlayFinished) { 
          
          if (currentAudioPrio == PRIO_MUSIC) {
            nextSong(); 
          } 
          else if (currentAudioPrio == PRIO_ALARM) {
            myDFPlayer.playFolder(1, 1); 
          }
          
        }
      }
      lastDFCheck = millis();
    }
  }
  // =======================================================

  // Process Touch
  handleTouch();
  
  // Process Servo
  updateServo();
  
  // Process Telegram Messages
  if (sharedMsg.hasNewMsg) {
    char localMsg[MSG_BUFFER_SIZE];
    portENTER_CRITICAL(&sharedMux);
    strncpy(localMsg, sharedMsg.msg, MSG_BUFFER_SIZE);
    sharedMsg.hasNewMsg = false;
    portEXIT_CRITICAL(&sharedMux);

    if (strcmp(localMsg, "/pomodoro") == 0) {
      isPomodoro = true;
      pomoStartTime = millis();
      Blynk.virtualWrite(V0, 1);
      setScreen("TELEGRAM", "Pomodoro ON", 3000);
    } else {
      setScreen("TIN NHAN", localMsg, 10000);
      setServo(S_NOTIFY);
    }
  }
  static unsigned long lastRender = 0;
  if (millis() - lastRender > 200) {
    drawScreen();
    lastRender = millis();
  }
  
  static unsigned long bootTime = millis();
  if (millis() - bootTime > 86400000UL) {
    Serial.println("24h reset");
    ESP.restart();
  }
  
  delay(10);
}