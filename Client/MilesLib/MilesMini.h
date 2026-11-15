#ifndef __MILESLIB_MILESMINI_H__
#define __MILESLIB_MILESMINI_H__

struct SMiniVoice;

bool MiniEnsureEngine();
void MiniShutdownEngine();

bool MiniVoiceCreate(SMiniVoice** voice);
void MiniVoiceDestroy(SMiniVoice* voice);
bool MiniVoiceLoad(SMiniVoice* voice, const void* data, unsigned long size, bool spatial);
bool MiniVoiceLoadCopy(SMiniVoice* voice, const void* data, unsigned long size, bool spatial);
void MiniVoicePlay(SMiniVoice* voice, int loopCount);
void MiniVoicePause(SMiniVoice* voice);
void MiniVoiceResume(SMiniVoice* voice);
void MiniVoiceStop(SMiniVoice* voice);
void MiniVoiceClear(SMiniVoice* voice);
bool MiniVoiceHasData(SMiniVoice* voice);
float MiniVoiceGetVolume(SMiniVoice* voice);
void MiniVoiceSetVolume(SMiniVoice* voice, float volume);
bool MiniVoiceIsDone(SMiniVoice* voice);
void MiniVoiceSetPosition(SMiniVoice* voice, float x, float y, float z);
void MiniVoiceSetVelocity(SMiniVoice* voice, float x, float y, float z);

void MiniListenerPosition(float x, float y, float z);
void MiniListenerDirection(float fx, float fy, float fz, float ux, float uy, float uz);
void MiniListenerVelocity(float x, float y, float z);

#endif
