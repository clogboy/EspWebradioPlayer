#include "LibraryModule.h"

LibraryModule::LibraryModule() {}

bool LibraryModule::addStation(const char* name, const char* url) {
    std::vector<Station> stations = load();
    
    if (stations.size() >= MAX_LIBRARY_ENTRIES) {
        Serial.println("Library full");
        return false;
    }
    
    Station newStation;
    newStation.name = String(name);
    newStation.url = String(url);
    
    stations.push_back(newStation);
    save(stations);
    
    Serial.print("Added to library: ");
    Serial.println(name);
    return true;
}

bool LibraryModule::removeStation(int index) {
    std::vector<Station> stations = load();
    
    if (index < 0 || index >= stations.size()) {
        return false;
    }
    
    stations.erase(stations.begin() + index);
    save(stations);
    
    Serial.print("Removed station at index: ");
    Serial.println(index);
    return true;
}

std::vector<Station> LibraryModule::getStations() {
    return load();
}

int LibraryModule::getCount() {
    return load().size();
}

void LibraryModule::save(const std::vector<Station>& stations) {
    prefs.begin("library", false);
    prefs.clear(); // Clear old data
    
    prefs.putInt("count", stations.size());
    
    for (int i = 0; i < stations.size(); i++) {
        String nameKey = "name" + String(i);
        String urlKey = "url" + String(i);
        prefs.putString(nameKey.c_str(), stations[i].name);
        prefs.putString(urlKey.c_str(), stations[i].url);
    }
    
    prefs.end();
}

std::vector<Station> LibraryModule::load() {
    std::vector<Station> stations;
    
    prefs.begin("library", true); // Read-only
    int count = prefs.getInt("count", 0);
    
    for (int i = 0; i < count; i++) {
        String nameKey = "name" + String(i);
        String urlKey = "url" + String(i);
        
        Station station;
        station.name = prefs.getString(nameKey.c_str(), "");
        station.url = prefs.getString(urlKey.c_str(), "");
        
        if (station.url.length() > 0) {
            stations.push_back(station);
        }
    }
    
    prefs.end();
    return stations;
}