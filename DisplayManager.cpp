#include "DisplayManager.h"
#include "AudioManager.h"
#include "NetworkManager.h"

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
      blynkWrite(0, 0);
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
        u8g2.setFont(u8g2_font_unifont_t_vietnamese2);
        unsigned long elapsed = millis() - noteDisplayStartTime;
        int currentPage = elapsed / 7500; 
        if (currentPage > 1) currentPage = 1; 

        u8g2.drawBox(0, 0, 128, 14);
        u8g2.setDrawColor(0); 
        
        char titleBuf[32];
        sprintf(titleBuf, "TO-DO LIST %d/2", currentPage + 1);
        int wTitle = u8g2.getUTF8Width(titleBuf);
        u8g2.setCursor((128 - wTitle) / 2, 11);
        u8g2.print(titleBuf);
        u8g2.setDrawColor(1); 
        
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
