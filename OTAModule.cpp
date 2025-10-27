#include "ArduinoOTA.h"
#include "OTAModule.h"

void OTAModule::begin(const char* hostname) {
    ArduinoOTA.setHostname(hostname);
    ArduinoOTA.setPassword("gridbeacon");
    
    ArduinoOTA.onStart([]() {
        Serial.println("\nOTA: Starting update");
    });
    
    ArduinoOTA.onEnd([]() {
        Serial.println("\nOTA: Complete");
    });
    
    ArduinoOTA.onError([](ota_error_t error) {
        Serial.print("OTA Error: ");
        Serial.println(error);
    });
    
    ArduinoOTA.begin();
    enabled = true;
    Serial.println("OTA enabled");
}

void OTAModule::handle() {
    if (enabled) {
        ArduinoOTA.handle();
    }
}