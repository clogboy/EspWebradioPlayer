#ifndef WEBSERVER_MODULE_H
#define WEBSERVER_MODULE_H

#include <WiFi.h>
#include <WebServer.h>
#include <DNSServer.h>
#include "WiFiModule.h"
#include "AudioModule.h"
#include "LibraryModule.h"
#include "DiscoveryModule.h"

#define DNS_PORT 53

class WebServerModule {
public:
    WebServerModule(WiFiModule* wifi, AudioModule* audio, LibraryModule* library, DiscoveryModule* discovery);
    
    void begin();
    void handle();
    
private:
    WiFiModule* wifiMgr;
    AudioModule* audioMgr;
    LibraryModule* libraryMgr;
    DiscoveryModule* discoveryMgr;
    WebServer* server;
    DNSServer* dnsServer;
    
    // Route handlers
    void handleRoot();
    void handlePlayer();
    void handleSettings();
    void handleSave();
    void handlePlayPause();
    void handleVolume();
    void handleAddLibrary();
    void handleGetLibrary();
    void handleRemoveLibrary();
    void handleRemoveNetwork();
    void handleSleepTimer();
    void handleReset();
    void handleNotFound();
};

#endif