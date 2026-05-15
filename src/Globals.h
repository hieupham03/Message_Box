#ifndef GLOBALS_H
#define GLOBALS_H

#include <Arduino.h>
#include "../secrets.h"

#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <UniversalTelegramBot.h>
#include <U8g2lib.h>
#include <ESP32Servo.h>
#include "DFRobotDFPlayerMini.h"
#include "time.h"

// ================= [2. HARDWARE PINS] =================
#define RXD2 32
#define TXD2 33 
#define SERVO_PIN 15
#define TOUCH_PIN 4

// ================= [3. ENUMS & CONSTANTS] =================
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

#define ALARM_FOLDER  1
#define MUSIC_FOLDER  2
#define TOTAL_SONGS   100

// Safe mode
#define WDT_TIMEOUT 30

// ================= [4. EXTERN GLOBAL VARIABLES] =================
extern String currentNote;
extern bool isShowingNote;
extern unsigned long noteDisplayStartTime;

extern String petMessage;
extern unsigned long petDisplayStartTime;
extern bool isShowingPet;

// Strings
extern const char STR_ALARM_TITLE[];
extern const char STR_ALARM_MSG[];
extern const char STR_ALARM_OFF[];
extern const char STR_POMO_ON[];
extern const char STR_POMO_OFF[];
extern const char STR_POMO_DONE[];
extern const char STR_TOUCHED[];
extern const char STR_MUSIC_PAUSE[];

extern struct tm startLoveDate;

// Audio state
extern int shuffleList[TOTAL_SONGS];
extern int shuffleIndex;
extern bool isShuffle;
extern int currentSongID;

// Safe mode state
extern int crashCount;
extern bool isSafeMode;

// Audio System state
extern AudioPriority currentAudioPrio;
extern bool audioLocked;
extern volatile bool dfPlayerReady;

// Servo state
extern ServoMode servoState;
extern unsigned long servoStartTime;
extern unsigned long lastServoStep;
extern int servoStep;

// UI state
extern struct tm cachedTime;
extern bool timeSynced;
extern char screenTitle[32];
extern char screenContent[128];
extern unsigned long screenEndTime;
extern bool showClock;
extern unsigned long lastSwitchScreen;

// Pomodoro state
extern bool isPomodoro;
extern unsigned long pomoStartTime;
extern const unsigned long POMO_DURATION;

// Alarm state
extern int alarmHour;
extern int alarmMin;
extern int lastAlarmMinute;
extern bool isAlarmRinging;

// Thread-safe Shared Data
#define MSG_BUFFER_SIZE 128
typedef struct {
  volatile bool hasNewMsg;
  char msg[MSG_BUFFER_SIZE];
  volatile uint32_t timestamp;
} SharedIncoming;

typedef struct {
  volatile bool hasPendingSend;
  char msg[64];
} SharedOutgoing;

extern SharedIncoming sharedMsg;
extern SharedOutgoing sharedSend;
extern portMUX_TYPE sharedMux;

// ================= [5. EXTERN OBJECTS] =================
extern HardwareSerial myHardwareSerial;
extern DFRobotDFPlayerMini myDFPlayer;
extern Servo myservo;
extern U8G2_SH1106_128X64_NONAME_F_HW_I2C u8g2;
extern WiFiClientSecure client;
extern UniversalTelegramBot bot;
extern const char songName[][64];

// System functions
void checkSafeMode();
void markStable();

#endif // GLOBALS_H
