#include "AudioManager.h"
#include "DisplayManager.h"
#include "NetworkManager.h"

void playAudio(int folder, int file, AudioPriority prio, bool loop) {
  if (isSafeMode || !dfPlayerReady) return;

  if (audioLocked && prio < PRIO_ALARM) return;
  if (!audioLocked && prio < currentAudioPrio) return;

  if (prio == PRIO_ALARM) {
    audioLocked = true;
  }
  currentAudioPrio = prio;

  myDFPlayer.playFolder(folder, file);
  
  delay(300); 

  if (prio != PRIO_ALARM) {
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

void playSong(int id) {
  playAudio(MUSIC_FOLDER, id, PRIO_MUSIC, false);
  blynkWrite(5, id);
  blynkWrite(4, 1);
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
