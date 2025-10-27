#ifndef DISCOVERY_MODULE_H
#define DISCOVERY_MODULE_H

#include <WiFi.h>
#include <WiFiUdp.h>
#include <Preferences.h>

#define DISCOVERY_PORT 5353
#define DISCOVERY_MULTICAST "239.255.0.1"
#define MAX_DEVICES 10
#define DEVICE_TIMEOUT 120000  // 2 minutes

struct GridBeaconDevice {
    String name;
    IPAddress ip;
    String status;      // "playing" or "paused"
    String station;     // current station name
    unsigned long lastSeen;
    bool active;
};

class DiscoveryModule {
public:
    DiscoveryModule();
    
    bool begin(const char* deviceName);
    void setStatus(bool playing, const char* stationName);
    void handle();  // Call in loop
    
    String getDeviceName();
    int getDeviceCount();
    GridBeaconDevice* getDevices();
    
private:
    WiFiUDP udp;
    Preferences prefs;
    String myName;
    IPAddress myIP;
    bool isPlaying;
    String currentStation;
    
    GridBeaconDevice devices[MAX_DEVICES];
    unsigned long lastBroadcast;
    
    void broadcast();
    void handleIncoming();
    void cleanupStale();
    void updateDevice(String name, IPAddress ip, String status, String station);
    
    // Device name storage
    void saveDeviceName(const char* name);
    String loadDeviceName();
};

#endif