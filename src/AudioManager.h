#ifndef AUDIO_MANAGER_H
#define AUDIO_MANAGER_H

#include "Globals.h"

// Core Audio Functions
void playAudio(int folder, int file, AudioPriority prio, bool loop = false);
void stopAudio(AudioPriority prio);

// Music Player Functions
void playSong(int id);
void generateShuffleList();
void nextSong();
void setShuffle(bool enable);

#endif // AUDIO_MANAGER_H
