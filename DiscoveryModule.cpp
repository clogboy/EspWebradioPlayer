#include "DiscoveryModule.h"

DiscoveryModule::DiscoveryModule() 
    : lastBroadcast(0), isPlaying(false), currentStation("") {
    for (int i = 0; i < MAX_DEVICES; i++) {
        devices[i].active = false;
    }
}

bool DiscoveryModule::begin(const char* deviceName) {
    // Load or save device name
    if (deviceName && strlen(deviceName) > 0) {
        myName = String(deviceName);
        saveDeviceName(deviceName);
    } else {
        myName = loadDeviceName();
        if (myName.length() == 0) {
            myName = "GridBeacon-" + String(random(1000, 9999));
            saveDeviceName(myName.c_str());
        }
    }
    
    myIP = WiFi.localIP();
    
    Serial.print("Discovery: Device name is '");
    Serial.print(myName);
    Serial.println("'");
    
    // Start UDP
    if (!udp.beginMulticast(IPAddress(239, 255, 0, 1), DISCOVERY_PORT)) {
        Serial.println("Discovery: UDP multicast failed");
        return false;
    }
    
    Serial.println("Discovery: Started");
    
    // Send initial query to discover existing devices
    broadcast();
    
    return true;
}

void DiscoveryModule::setStatus(bool playing, const char* stationName) {
    isPlaying = playing;
    currentStation = stationName ? String(stationName) : "";
}

void DiscoveryModule::handle() {
    unsigned long now = millis();
    
    // Broadcast every 30 seconds
    if (now - lastBroadcast > 30000) {
        broadcast();
        lastBroadcast = now;
    }
    
    // Check for incoming messages
    handleIncoming();
    
    // Clean up stale devices
    cleanupStale();
}

void DiscoveryModule::broadcast() {
    String status = isPlaying ? "playing" : "paused";
    String message = "GRIDBEACON|" + myName + "|" + myIP.toString() + "|" + status + "|" + currentStation;
    
    udp.beginMulticastPacket();
    udp.print(message);
    udp.endPacket();
    
    Serial.print("Discovery: Broadcast: ");
    Serial.println(message);
}

void DiscoveryModule::handleIncoming() {
    int packetSize = udp.parsePacket();
    if (packetSize == 0) return;
    
    char buffer[256];
    int len = udp.read(buffer, sizeof(buffer) - 1);
    if (len <= 0) return;
    buffer[len] = '\0';
    
    String message = String(buffer);
    
    // Parse: "GRIDBEACON|name|ip|status|station"
    if (!message.startsWith("GRIDBEACON|")) return;
    
    int idx1 = message.indexOf('|', 11);
    int idx2 = message.indexOf('|', idx1 + 1);
    int idx3 = message.indexOf('|', idx2 + 1);
    
    if (idx1 < 0 || idx2 < 0 || idx3 < 0) return;
    
    String name = message.substring(11, idx1);
    String ipStr = message.substring(idx1 + 1, idx2);
    String status = message.substring(idx2 + 1, idx3);
    String station = message.substring(idx3 + 1);
    
    // Ignore messages from self
    if (name == myName) return;
    
    IPAddress ip;
    if (!ip.fromString(ipStr)) return;
    
    updateDevice(name, ip, status, station);
}

void DiscoveryModule::updateDevice(String name, IPAddress ip, String status, String station) {
    // Find existing device or empty slot
    int emptySlot = -1;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].active && devices[i].name == name) {
            // Update existing
            devices[i].ip = ip;
            devices[i].status = status;
            devices[i].station = station;
            devices[i].lastSeen = millis();
            return;
        }
        if (!devices[i].active && emptySlot < 0) {
            emptySlot = i;
        }
    }
    
    // Add new device
    if (emptySlot >= 0) {
        devices[emptySlot].name = name;
        devices[emptySlot].ip = ip;
        devices[emptySlot].status = status;
        devices[emptySlot].station = station;
        devices[emptySlot].lastSeen = millis();
        devices[emptySlot].active = true;
        
        Serial.print("Discovery: Found device '");
        Serial.print(name);
        Serial.print("' at ");
        Serial.println(ip);
    }
}

void DiscoveryModule::cleanupStale() {
    unsigned long now = millis();
    
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].active && (now - devices[i].lastSeen > DEVICE_TIMEOUT)) {
            Serial.print("Discovery: Device '");
            Serial.print(devices[i].name);
            Serial.println("' timed out");
            devices[i].active = false;
        }
    }
}

String DiscoveryModule::getDeviceName() {
    return myName;
}

int DiscoveryModule::getDeviceCount() {
    int count = 0;
    for (int i = 0; i < MAX_DEVICES; i++) {
        if (devices[i].active) count++;
    }
    return count;
}

GridBeaconDevice* DiscoveryModule::getDevices() {
    return devices;
}

void DiscoveryModule::saveDeviceName(const char* name) {
    prefs.begin("discovery", false);
    prefs.putString("name", name);
    prefs.end();
}

String DiscoveryModule::loadDeviceName() {
    prefs.begin("discovery", true);
    String name = prefs.getString("name", "");
    prefs.end();
    return name;
}