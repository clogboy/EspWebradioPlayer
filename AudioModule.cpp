#include "AudioModule.h"

// Single URL holder for dynamic changes
static const char* dynamicURL[1] = { "" };

AudioModule::AudioModule(const char* ssid, const char* password) 
    : wifiSSID(ssid), wifiPassword(password), playing(false), currentVolume(0.05),
      sleepTimerActive(false), sleepEndTime(0), sleepFadeStart(0), sleepStartVolume(0) {
    
    urlStream = new URLStream(wifiSSID, wifiPassword);
    source = new AudioSourceURL(*urlStream, dynamicURL, "audio/mp3");
    i2s = new I2SStream();
    decoder = new MP3DecoderHelix();
    player = new AudioPlayer(*source, *i2s, *decoder);
}

bool AudioModule::begin() {
    Serial.println("Setting up audio...");
    
    // I2S setup (exactly as working example)
    auto cfg = i2s->defaultConfig(TX_MODE);
    cfg.pin_bck = I2S_BCLK_PIN;
    cfg.pin_ws = I2S_LRCK_PIN;
    cfg.pin_data = I2S_DATA_PIN;
    cfg.channels = 2;
    
    if (!i2s->begin(cfg)) {
        Serial.println("I2S init failed");
        return false;
    }
    
    // Player setup
    if (!player->begin()) {
        Serial.println("Player init failed");
        return false;
    }
    
    // Start paused - wait for user to select a station
    playing = false;
    player->setVolume(currentVolume);
    
    Serial.println("Audio ready (paused - select station to play)");
    return true;
}

void AudioModule::process() {
    if (playing) {
        player->copy();
    }
    
    // Handle sleep timer
    if (sleepTimerActive) {
        processSleepTimer();
    }
}

void AudioModule::play() {
    playing = true;
    Serial.println("Audio: playing");
}

void AudioModule::pause() {
    playing = false;
    Serial.println("Audio: paused");
}

bool AudioModule::isPlaying() {
    return playing;
}

void AudioModule::setVolume(float vol) {
    if (vol < 0.0) vol = 0.0;
    if (vol > 1.0) vol = 1.0;
    currentVolume = vol;
    player->setVolume(currentVolume);
    Serial.print("Volume: ");
    Serial.println(currentVolume);
}

float AudioModule::getVolume() {
    return currentVolume;
}

bool AudioModule::setURL(const char* url) {
    if (!url || strlen(url) == 0) {
        Serial.println("setURL: Invalid URL");
        return false;
    }
    
    Serial.print("Changing stream to: ");
    Serial.println(url);
    
    currentURL = String(url);
    
    // Stop current playback
    bool wasPlaying = playing;
    playing = false;
    delay(200); // Give audio thread time to stop
    
    // Clean up old objects (in reverse order of creation)
    if (player) { delete player; player = nullptr; }
    if (decoder) { delete decoder; decoder = nullptr; }
    if (source) { delete source; source = nullptr; }
    if (i2s) { delete i2s; i2s = nullptr; }
    if (urlStream) { delete urlStream; urlStream = nullptr; }
    
    // Recreate with new URL
    dynamicURL[0] = currentURL.c_str();
    
    urlStream = new URLStream(wifiSSID, wifiPassword);
    source = new AudioSourceURL(*urlStream, dynamicURL, "audio/mp3");
    i2s = new I2SStream();
    decoder = new MP3DecoderHelix();
    player = new AudioPlayer(*source, *i2s, *decoder);
    
    // Reinitialize
    auto cfg = i2s->defaultConfig(TX_MODE);
    cfg.pin_bck = I2S_BCLK_PIN;
    cfg.pin_ws = I2S_LRCK_PIN;
    cfg.pin_data = I2S_DATA_PIN;
    cfg.channels = 2;
    
    if (!i2s->begin(cfg)) {
        Serial.println("I2S reinit failed");
        return false;
    }
    
    if (!player->begin()) {
        Serial.println("Player reinit failed");
        return false;
    }
    
    player->setVolume(currentVolume);
    
    // Resume if was playing
    if (wasPlaying) {
        playing = true;
    }
    
    Serial.println("Stream changed successfully");
    return true;
}

String AudioModule::getCurrentURL() {
    return currentURL;
}

void AudioModule::setSleepTimer(unsigned long durationMinutes) {
    unsigned long durationMs = durationMinutes * 60 * 1000;
    unsigned long fadeMs = 2 * 60 * 1000; // 2 minute fade
    
    sleepEndTime = millis() + durationMs;
    sleepFadeStart = sleepEndTime - fadeMs;
    sleepStartVolume = currentVolume;
    sleepTimerActive = true;
    
    Serial.print("Sleep timer set for ");
    Serial.print(durationMinutes);
    Serial.println(" minutes");
}

void AudioModule::cancelSleepTimer() {
    sleepTimerActive = false;
    Serial.println("Sleep timer cancelled");
}

bool AudioModule::hasSleepTimer() {
    return sleepTimerActive;
}

unsigned long AudioModule::getSleepTimeRemaining() {
    if (!sleepTimerActive) return 0;
    
    unsigned long now = millis();
    if (now >= sleepEndTime) return 0;
    
    return (sleepEndTime - now) / 1000; // return seconds
}

void AudioModule::processSleepTimer() {
    unsigned long now = millis();
    
    // Time's up - pause
    if (now >= sleepEndTime) {
        pause();
        cancelSleepTimer();
        Serial.println("Sleep timer ended - paused");
        return;
    }
    
    // In fade period - gradually reduce volume
    if (now >= sleepFadeStart) {
        unsigned long fadeElapsed = now - sleepFadeStart;
        unsigned long fadeDuration = sleepEndTime - sleepFadeStart;
        float fadeProgress = (float)fadeElapsed / fadeDuration;
        
        float newVolume = sleepStartVolume * (1.0 - fadeProgress);
        player->setVolume(newVolume);
    }
}