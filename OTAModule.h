#ifndef OTA_MODULE_H
#define OTA_MODULE_H

#include <ArduinoOTA.h>

class OTAModule {
public:
    void begin(const char* hostname = "GridBeacon");
    void handle();
    
private:
    bool enabled;
};

#endif