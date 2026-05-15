#include "ServoManager.h"

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
