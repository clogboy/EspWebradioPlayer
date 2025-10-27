#include "WiFiModule.h"

WiFiModule::WiFiModule() : mode(MODE_NONE), networkCount(0) {}

bool WiFiModule::begin() {
    Serial.println("WiFi init...");
    
    loadAllNetworks();
    
    // Try each saved network
    for (int i = 0; i < networkCount; i++) {
        Serial.print("Trying network ");
        Serial.print(i + 1);
        Serial.print("/");
        Serial.print(networkCount);
        Serial.print(": ");
        Serial.println(networks[i].ssid);
        
        if (tryConnect(networks[i].ssid.c_str(), networks[i].password.c_str())) {
            mode = MODE_STATION;
            Serial.print("Connected. IP: ");
            Serial.println(WiFi.localIP());
            return true;
        }
    }
    
    // No saved networks or none worked - start AP
    Serial.println("No networks available, starting AP mode");
    if (startAP()) {
        mode = MODE_AP;
        Serial.print("AP IP: ");
        Serial.println(WiFi.softAPIP());
        return true;
    }
    
    Serial.println("WiFi init failed");
    return false;
}

bool WiFiModule::tryConnect(const char* ssid, const char* password) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 40) {
        delay(500);
        Serial.print(".");
        attempts++;
    }
    Serial.println();
    
    return WiFi.status() == WL_CONNECTED;
}

bool WiFiModule::startAP() {
    WiFi.mode(WIFI_AP);
    return WiFi.softAP(AP_SSID, AP_PASSWORD);
}

WiFiMode WiFiModule::getMode() {
    return mode;
}

bool WiFiModule::isConnected() {
    return (mode == MODE_STATION && WiFi.status() == WL_CONNECTED);
}

bool WiFiModule::addNetwork(const char* ssid, const char* password) {
    if (networkCount >= MAX_NETWORKS) {
        Serial.println("Network list full");
        return false;
    }
    
    // Check if already exists
    for (int i = 0; i < networkCount; i++) {
        if (networks[i].ssid == String(ssid)) {
            // Update password
            networks[i].password = String(password);
            saveAllNetworks();
            Serial.println("Network updated");
            return true;
        }
    }
    
    // Add new
    networks[networkCount].ssid = String(ssid);
    networks[networkCount].password = String(password);
    networkCount++;
    saveAllNetworks();
    
    Serial.print("Network added: ");
    Serial.println(ssid);
    return true;
}

bool WiFiModule::removeNetwork(int index) {
    if (index < 0 || index >= networkCount) return false;
    
    // Shift array
    for (int i = index; i < networkCount - 1; i++) {
        networks[i] = networks[i + 1];
    }
    networkCount--;
    saveAllNetworks();
    
    Serial.print("Network removed at index ");
    Serial.println(index);
    return true;
}

int WiFiModule::getNetworkCount() {
    return networkCount;
}

SavedNetwork* WiFiModule::getNetworks() {
    return networks;
}

void WiFiModule::clearAllNetworks() {
    networkCount = 0;
    saveAllNetworks();
    Serial.println("All networks cleared");
}

void WiFiModule::loadAllNetworks() {
    prefs.begin("wifi", true);
    networkCount = prefs.getInt("count", 0);
    
    if (networkCount > MAX_NETWORKS) networkCount = MAX_NETWORKS;
    
    for (int i = 0; i < networkCount; i++) {
        String ssidKey = "ssid" + String(i);
        String passKey = "pass" + String(i);
        networks[i].ssid = prefs.getString(ssidKey.c_str(), "");
        networks[i].password = prefs.getString(passKey.c_str(), "");
    }
    
    prefs.end();
    Serial.print("Loaded ");
    Serial.print(networkCount);
    Serial.println(" networks");
}

void WiFiModule::saveAllNetworks() {
    prefs.begin("wifi", false);
    prefs.clear();
    prefs.putInt("count", networkCount);
    
    for (int i = 0; i < networkCount; i++) {
        String ssidKey = "ssid" + String(i);
        String passKey = "pass" + String(i);
        prefs.putString(ssidKey.c_str(), networks[i].ssid);
        prefs.putString(passKey.c_str(), networks[i].password);
    }
    
    prefs.end();
}

// Legacy single-network support
bool WiFiModule::saveCredentials(const char* ssid, const char* password) {
    return addNetwork(ssid, password);
}

bool WiFiModule::loadCredentials(char* ssid, char* password, size_t maxLen) {
    if (networkCount == 0) return false;
    
    strncpy(ssid, networks[0].ssid.c_str(), maxLen - 1);
    strncpy(password, networks[0].password.c_str(), maxLen - 1);
    ssid[maxLen - 1] = '\0';
    password[maxLen - 1] = '\0';
    
    return true;
}

void WiFiModule::clearCredentials() {
    clearAllNetworks();
}