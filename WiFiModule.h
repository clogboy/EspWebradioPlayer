#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include <WiFi.h>
#include <Preferences.h>

#define AP_SSID "GridBeacon-Setup"
#define AP_PASSWORD "gridbeacon"
#define MAX_NETWORKS 10

struct SavedNetwork {
    String ssid;
    String password;
};

enum WiFiMode {
    MODE_STATION,
    MODE_AP,
    MODE_NONE
};

class WiFiModule {
public:
    WiFiModule();
    
    bool begin();
    WiFiMode getMode();
    bool isConnected();
    
    // Multi-network credentials
    bool addNetwork(const char* ssid, const char* password);
    bool removeNetwork(int index);
    int getNetworkCount();
    SavedNetwork* getNetworks();
    void clearAllNetworks();
    
    // Legacy single-network support (for compatibility)
    bool saveCredentials(const char* ssid, const char* password);
    bool loadCredentials(char* ssid, char* password, size_t maxLen);
    void clearCredentials();
    
private:
    Preferences prefs;
    WiFiMode mode;
    SavedNetwork networks[MAX_NETWORKS];
    int networkCount;
    
    bool tryConnect(const char* ssid, const char* password);
    bool startAP();
    void loadAllNetworks();
    void saveAllNetworks();
};

#endif