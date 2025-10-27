#ifndef AUDIO_MODULE_H
#define AUDIO_MODULE_H

#include "AudioTools.h"
#include "AudioTools/AudioCodecs/CodecMP3Helix.h"
#include "AudioTools/Disk/AudioSourceURL.h"
#include "AudioTools/Communication/AudioHttp.h"

// I2S Pin Configuration
#define I2S_LRCK_PIN 5
#define I2S_DATA_PIN 3
#define I2S_BCLK_PIN 4

class AudioModule {
public:
    AudioModule(const char* ssid, const char* password);
    
    bool begin();
    void process();
    
    // Simple controls
    void play();
    void pause();
    bool isPlaying();
    void setVolume(float vol); // 0.0 to 1.0
    float getVolume();
    bool setURL(const char* url);
    String getCurrentURL();
    
    // Sleep timer
    void setSleepTimer(unsigned long durationMinutes);
    void cancelSleepTimer();
    bool hasSleepTimer();
    unsigned long getSleepTimeRemaining(); // seconds
    void processSleepTimer(); // call in loop
    
private:
    // WiFi credentials
    const char* wifiSSID;
    const char* wifiPassword;
    
    // Audio objects (same as working example)
    URLStream* urlStream;
    AudioSourceURL* source;
    I2SStream* i2s;
    MP3DecoderHelix* decoder;
    AudioPlayer* player;
    
    bool playing;
    float currentVolume;
    
    // Sleep timer state
    unsigned long sleepEndTime;
    unsigned long sleepFadeStart;
    float sleepStartVolume;
    bool sleepTimerActive;
    
    // Current URL storage
    String currentURL;
    
    // Helper to restart with new URL
    bool restartWithURL(const char* url);
};

#endif