#ifndef LIBRARY_MODULE_H
#define LIBRARY_MODULE_H

#include <Preferences.h>
#include <vector>

#define MAX_LIBRARY_ENTRIES 10

struct Station {
    String name;
    String url;
};

class LibraryModule {
public:
    LibraryModule();
    
    bool addStation(const char* name, const char* url);
    bool removeStation(int index);
    std::vector<Station> getStations();
    int getCount();
    
private:
    Preferences prefs;
    void save(const std::vector<Station>& stations);
    std::vector<Station> load();
};

#endif