#include "Globals.h"
#include <esp_task_wdt.h>

// ================= [4. GLOBAL VARIABLES] =================
String currentNote = "Chua co ghi chu"; 
bool isShowingNote = false; 
unsigned long noteDisplayStartTime = 0;

String petMessage = "";           
unsigned long petDisplayStartTime = 0; 
bool isShowingPet = false;        

// Strings
const char STR_ALARM_TITLE[]  = "BAO THUC!";
const char STR_ALARM_MSG[]    = "Day di em yeu oi!";
const char STR_ALARM_OFF[]    = "Da tat bao thuc";
const char STR_POMO_ON[]      = "Pomodoro ON";
const char STR_POMO_OFF[]     = "Pomodoro OFF";
const char STR_POMO_DONE[]    = "Hoan thanh!";
const char STR_TOUCHED[]      = "Da cham!";
const char STR_MUSIC_PAUSE[]  = "Tam dung nhac";

struct tm startLoveDate = {0, 0, 0, 23, 1, 125}; // 23/02/2025

// Audio state
int shuffleList[TOTAL_SONGS];
int shuffleIndex = 0;
bool isShuffle = false;
int currentSongID = 1;

// Safe mode state
RTC_DATA_ATTR int crashCount = 0;
bool isSafeMode = false;

// Audio System state
AudioPriority currentAudioPrio = PRIO_IDLE;
bool audioLocked = false;
volatile bool dfPlayerReady = false; 

// Servo state
ServoMode servoState = S_IDLE;
unsigned long servoStartTime = 0;
unsigned long lastServoStep = 0;
int servoStep = 0;

// UI state
struct tm cachedTime;
bool timeSynced = false;
char screenTitle[32];
char screenContent[128] = "";
unsigned long screenEndTime = 0;
bool showClock = true;
unsigned long lastSwitchScreen = 0;

// Pomodoro state
bool isPomodoro = false;
unsigned long pomoStartTime = 0;
const unsigned long POMO_DURATION = 25 * 60 * 1000;

// Alarm state
int alarmHour = -1;
int alarmMin = -1;
int lastAlarmMinute = -1;
bool isAlarmRinging = false;

// Thread-safe Shared Data
SharedIncoming sharedMsg = { false, {0}, 0 };
SharedOutgoing sharedSend = { false, {0} };
portMUX_TYPE sharedMux = portMUX_INITIALIZER_UNLOCKED;

// ================= [5. EXTERN OBJECTS] =================
HardwareSerial myHardwareSerial(2);
DFRobotDFPlayerMini myDFPlayer;
Servo myservo;
U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, U8X8_PIN_NONE);
WiFiClientSecure client;
UniversalTelegramBot bot(SECRET_BOT_TOKEN, client);

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
