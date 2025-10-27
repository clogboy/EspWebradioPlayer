/**
 * GridBeacon - ESP32-C3 Web Radio Player
 * Clean, debuggable, optimized
 */

#include "WiFiModule.h"
#include "AudioModule.h"
#include "LibraryModule.h"
#include "WebServerModule.h"
#include "OTAModule.h"
#include "DiscoveryModule.h"

// Global instances
WiFiModule wifi;
LibraryModule library;
OTAModule ota;
DiscoveryModule discovery;
AudioModule* audio = nullptr;
WebServerModule* webServer = nullptr;

void setup() {
  Serial.begin(115200);
  delay(2000);  // Longer delay to ensure serial is ready
  
  Serial.println("\n\n=================================");
  Serial.println("    GridBeacon Starting Up");
  Serial.println("=================================\n");
  
  // Enable audio library logging
  AudioToolsLogger.begin(Serial, AudioToolsLogLevel::Info);
  
  // 1. Initialize WiFi (tries stored credentials or starts AP)
  Serial.println("[1/4] Initializing WiFi...");
  if (!wifi.begin()) {
    Serial.println("ERROR: WiFi init failed - halting");
    while(1) { 
      delay(1000);
      Serial.print(".");
    }
  }
  Serial.println("WiFi: OK");
  Serial.print("Mode: ");
  Serial.println(wifi.getMode() == MODE_STATION ? "STATION" : "AP");
  
  // 2. If connected to WiFi, initialize audio, discovery, and OTA
  if (wifi.getMode() == MODE_STATION) {
    Serial.println("\n[2/4] Station Mode - Loading credentials...");
    char ssid[64], password[64];
    wifi.loadCredentials(ssid, password, 64);
    Serial.print("SSID: ");
    Serial.println(ssid);
    
    // Initialize discovery (loads saved room name)
    Serial.println("\n[3/4] Initializing discovery...");
    discovery.begin("");  // Empty = load from Preferences
    Serial.println("Discovery: OK");
    
    // Initialize audio (starts paused)
    Serial.println("\n[4/4] Initializing audio...");
    Serial.println("Creating AudioModule instance...");
    audio = new AudioModule(ssid, password);
    
    Serial.println("Calling audio->begin()...");
    if (audio->begin()) {
      Serial.println("Audio: initialized (paused)");
    } else {
      Serial.println("ERROR: Audio init failed - continuing without audio");
      delete audio;
      audio = nullptr;
    }
    
    // Enable OTA updates
    Serial.println("\nEnabling OTA updates...");
    ota.begin("GridBeacon");
    Serial.println("OTA: OK");
    
  } else {
    Serial.println("\n[2/4] AP Mode - Skipping audio/discovery/OTA");
    Serial.print("Connect to AP: ");
    Serial.println(AP_SSID);
    Serial.print("Password: ");
    Serial.println(AP_PASSWORD);
  }
  
  // 3. Start web server (works in both AP and station mode)
  Serial.println("\nStarting web server...");
  webServer = new WebServerModule(&wifi, audio, &library, &discovery);
  webServer->begin();
  Serial.println("Web server: OK");
  
  Serial.println("\n=================================");
  Serial.println("       SYSTEM READY");
  Serial.println("=================================");
  
  if (wifi.getMode() == MODE_AP) {
    Serial.println("\nAP Mode:");
    Serial.print("  SSID: ");
    Serial.println(AP_SSID);
    Serial.print("  IP: ");
    Serial.println(WiFi.softAPIP());
    Serial.println("  Go to: http://192.168.4.1");
  } else {
    Serial.println("\nStation Mode:");
    Serial.print("  IP: ");
    Serial.println(WiFi.localIP());
    Serial.print("  Go to: http://");
    Serial.println(WiFi.localIP());
  }
  Serial.println("\n");
}

void loop() {
  // Handle OTA (only active in station mode)
  ota.handle();
  
  // Handle device discovery
  if (wifi.getMode() == MODE_STATION) {
    discovery.handle();
    
    // Update discovery status when audio state changes
    if (audio != nullptr) {
      discovery.setStatus(audio->isPlaying(), "Current Station");
    }
  }
  
  // Handle web requests
  if (webServer != nullptr) {
    webServer->handle();
  }
  
  // Process audio stream
  if (audio != nullptr) {
    audio->process();
  }
  
  // Small yield to prevent watchdog
  //yield();
}