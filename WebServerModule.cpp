#include "WebServerModule.h"

WebServerModule::WebServerModule(WiFiModule* wifi, AudioModule* audio, LibraryModule* library, DiscoveryModule* discovery)
  : wifiMgr(wifi), audioMgr(audio), libraryMgr(library), discoveryMgr(discovery) {

  server = new WebServer(80);
  dnsServer = new DNSServer();
}

void WebServerModule::begin() {
  Serial.println("Starting web server...");

  // Start captive portal DNS in AP mode
  if (wifiMgr->getMode() == MODE_AP) {
    dnsServer->start(DNS_PORT, "*", WiFi.softAPIP());
    Serial.println("Captive portal DNS started");
  }

  // Route handlers
  server->on("/", [this]() {
    handleRoot();
  });
  server->on("/player", [this]() {
    handlePlayer();
  });
  server->on("/settings", [this]() {
    handleSettings();
  });
  server->on("/save", HTTP_POST, [this]() {
    handleSave();
  });
  server->on("/play", HTTP_POST, [this]() {
    handlePlayPause();
  });
  server->on("/volume", HTTP_POST, [this]() {
    handleVolume();
  });
  server->on("/library/add", HTTP_POST, [this]() {
    handleAddLibrary();
  });
  server->on("/library/get", [this]() {
    handleGetLibrary();
  });
  server->on("/library/remove", HTTP_POST, [this]() {
    handleRemoveLibrary();
  });
  server->on("/network/remove", HTTP_POST, [this]() {
    handleRemoveNetwork();
  });
  server->on("/sleep", HTTP_POST, [this]() {
    handleSleepTimer();
  });
  server->on("/reset", HTTP_POST, [this]() {
    handleReset();
  });
  server->onNotFound([this]() {
    handleNotFound();
  });

  server->begin();
  Serial.println("Web server started");
}

void WebServerModule::handle() {
  if (wifiMgr->getMode() == MODE_AP && dnsServer != nullptr) {
    dnsServer->processNextRequest();
  }
  if (server != nullptr) {
    server->handleClient();
  }
}

void WebServerModule::handleRoot() {
  // If in station mode, show player. If in AP mode, show WiFi setup
  if (wifiMgr->getMode() == MODE_STATION) {
    handlePlayer();
  } else {
    // WiFi setup page (captive portal)
    String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                  "<title>GridBeacon Setup</title><style>*{margin:0;padding:0;box-sizing:border-box}"
                  "body{font-family:Impact,Arial Black,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);"
                  "min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}"
                  ".container{max-width:420px;width:100%}.card{background:#fff;border-radius:16px;padding:40px 30px;"
                  "box-shadow:0 20px 60px rgba(0,0,0,0.3);position:relative;overflow:hidden}"
                  ".card::before{content:'';position:absolute;top:0;left:0;right:0;height:8px;"
                  "background:linear-gradient(90deg,#FF6B6B,#FFE66D,#4ECDC4,#FF6B6B);background-size:200% 100%;"
                  "animation:slideGradient 3s linear infinite}@keyframes slideGradient{0%{background-position:0% 50%}"
                  "100%{background-position:200% 50%}}h1{font-size:42px;color:#2d3748;text-transform:uppercase;"
                  "letter-spacing:2px;margin-bottom:8px;text-shadow:3px 3px 0px #FFE66D}.tagline{font-family:Arial,sans-serif;"
                  "font-size:14px;color:#718096;margin-bottom:30px;font-weight:normal}label{display:block;font-size:14px;"
                  "color:#4a5568;margin-bottom:8px;margin-top:20px;text-transform:uppercase;letter-spacing:1px;font-weight:bold}"
                  "input{width:100%;padding:14px 16px;border:3px solid #e2e8f0;border-radius:8px;font-size:16px;"
                  "font-family:Arial,sans-serif;transition:all 0.3s;background:#f7fafc}input:focus{outline:none;"
                  "border-color:#4ECDC4;background:#fff;transform:translateY(-2px);box-shadow:0 4px 12px rgba(78,205,196,0.3)}"
                  "button{width:100%;padding:16px;margin-top:30px;background:linear-gradient(135deg,#FF6B6B 0%,#FF8E53 100%);"
                  "color:white;border:none;border-radius:8px;font-size:18px;font-weight:bold;text-transform:uppercase;"
                  "letter-spacing:2px;cursor:pointer;transition:all 0.3s;box-shadow:0 4px 15px rgba(255,107,107,0.4)}"
                  "button:hover{transform:translateY(-3px);box-shadow:0 6px 20px rgba(255,107,107,0.6)}"
                  ".status-badge{display:inline-block;background:#4ECDC4;color:white;padding:6px 12px;border-radius:20px;"
                  "font-size:12px;font-family:Arial,sans-serif;font-weight:bold;margin-top:20px}.footer{text-align:center;"
                  "margin-top:20px;color:white;font-size:12px;font-family:Arial,sans-serif;text-shadow:1px 1px 2px rgba(0,0,0,0.3)}"
                  "</style></head><body><div class='container'><div class='card'><h1>GRIDBEACON</h1>"
                  "<p class='tagline'>Break free. Stream anywhere.</p><form action='/save' method='POST'>"
                  "<label for='ssid'>WiFi Network</label><input type='text' id='ssid' name='ssid' placeholder='Enter network name' required>"
                  "<label for='password'>Password</label><input type='password' id='password' name='password' placeholder='Enter password' required>"
                  "<label for='room'>Room/Location Name</label><input type='text' id='room' name='room' placeholder='e.g. Bedroom, Kitchen' required maxlength='20'>"
                  "<button type='submit'>CONNECT</button></form><div class='status-badge'>● AP MODE</div></div>"
                  "<div class='footer'>Built by rebels, for rebels</div></div></body></html>";

    server->send(200, "text/html", html);
  }
}

void WebServerModule::handlePlayer() {
  // Player interface
  float vol = audioMgr ? audioMgr->getVolume() * 100 : 25;
  bool playing = audioMgr ? audioMgr->isPlaying() : false;

  // Get memory info
  uint32_t freeHeap = ESP.getFreeHeap();
  uint32_t heapSize = ESP.getHeapSize();
  uint32_t usedHeap = heapSize - freeHeap;
  int heapPercent = (usedHeap * 100) / heapSize;

  // Get library stations
  String libraryHTML = "";
  if (libraryMgr) {
    auto stations = libraryMgr->getStations();
    if (stations.size() > 0) {
      libraryHTML = "<div class='library-section'><label>Your Stations</label><div class='station-list'>";
      for (int i = 0; i < stations.size(); i++) {
        libraryHTML += "<div class='station-item' onclick='playStation(\"" + stations[i].url + "\")'>"
                                                                                               "<div class='station-name'>"
                       + stations[i].name + "</div>"
                                            "<button class='btn-remove' onclick='event.stopPropagation();removeStation("
                       + String(i) + ")'>✕</button></div>";
      }
      libraryHTML += "</div></div>";
    } else {
      libraryHTML = "<div class='library-section'><label>Your Stations</label>"
                    "<p style='color:#718096;font-family:Arial;font-size:14px;text-align:center;padding:20px'>No stations saved yet. Add one below!</p></div>";
    }
  }

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'>"
                "<title>GridBeacon Player</title><style>*{margin:0;padding:0;box-sizing:border-box}"
                "body{font-family:Impact,Arial Black,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);"
                "min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}"
                ".container{max-width:480px;width:100%}.card{background:#fff;border-radius:16px;padding:30px;"
                "box-shadow:0 20px 60px rgba(0,0,0,0.3);position:relative;overflow:hidden}"
                ".card::before{content:'';position:absolute;top:0;left:0;right:0;height:8px;"
                "background:linear-gradient(90deg,#FF6B6B,#FFE66D,#4ECDC4,#FF6B6B);background-size:200% 100%;"
                "animation:slideGradient 3s linear infinite}@keyframes slideGradient{0%{background-position:0% 50%}"
                "100%{background-position:200% 50%}}h1{font-size:38px;color:#2d3748;text-transform:uppercase;"
                "letter-spacing:2px;margin-bottom:20px;text-shadow:3px 3px 0px #FFE66D}"
                ".status-bar{display:flex;justify-content:space-between;align-items:center;margin-bottom:25px;"
                "padding:12px 16px;background:#f7fafc;border-radius:8px;font-family:Arial,sans-serif}"
                ".status-indicator{display:flex;align-items:center;gap:8px;font-size:14px;color:#4a5568}"
                ".status-dot{width:10px;height:10px;border-radius:50%;background:#4ECDC4;"
                "animation:pulse 2s ease-in-out infinite}@keyframes pulse{0%,100%{opacity:1}50%{opacity:0.5}}"
                ".volume-display{font-weight:bold;color:#FF6B6B}"
                ".memory-bar{margin-top:8px;display:flex;align-items:center;gap:8px;font-size:12px;color:#718096}"
                ".memory-fill{height:6px;background:#e2e8f0;border-radius:3px;flex:1;overflow:hidden}"
                ".memory-used{height:100%;background:linear-gradient(90deg,#4ECDC4,#FFE66D,#FF6B6B);transition:width 0.3s}"
                ".section{margin-bottom:25px}"
                "label{display:block;font-size:13px;color:#4a5568;margin-bottom:8px;text-transform:uppercase;"
                "letter-spacing:1px;font-weight:bold}input[type=url]{width:100%;padding:12px 14px;border:3px solid #e2e8f0;"
                "border-radius:8px;font-size:15px;font-family:Arial,sans-serif;transition:all 0.3s;background:#f7fafc}"
                "input[type=url]:focus{outline:none;border-color:#4ECDC4;background:#fff;"
                "box-shadow:0 4px 12px rgba(78,205,196,0.3)}"
                ".library-section{margin-bottom:25px}.station-list{display:flex;flex-direction:column;gap:10px}"
                ".station-item{background:#f7fafc;border:2px solid #e2e8f0;border-radius:8px;padding:12px 14px;"
                "display:flex;justify-content:space-between;align-items:center;cursor:pointer;transition:all 0.3s}"
                ".station-item:hover{border-color:#4ECDC4;transform:translateY(-2px);box-shadow:0 4px 12px rgba(78,205,196,0.3)}"
                ".station-name{font-family:Arial,sans-serif;font-size:15px;color:#2d3748;font-weight:bold}"
                ".btn-remove{background:#ff6b6b;color:white;border:none;border-radius:50%;width:24px;height:24px;"
                "font-size:14px;cursor:pointer;transition:all 0.2s;display:flex;align-items:center;justify-content:center}"
                ".btn-remove:hover{background:#ff5252;transform:scale(1.1)}"
                ".volume-section{margin-bottom:25px}"
                ".volume-label{display:flex;justify-content:space-between;align-items:center;margin-bottom:12px}"
                ".volume-value{font-family:Arial,sans-serif;font-weight:bold;color:#FF6B6B;font-size:18px}"
                ".slider-container{position:relative;height:50px;background:#f7fafc;border-radius:25px;"
                "border:3px solid #e2e8f0;overflow:hidden}.slider-fill{position:absolute;left:0;top:0;height:100%;"
                "width:"
                + String((int)vol) + "%;background:linear-gradient(90deg,#4ECDC4 0%,#44A08D 100%);transition:width 0.2s}"
                                     "input[type=range]{position:relative;width:100%;height:50px;-webkit-appearance:none;appearance:none;"
                                     "background:transparent;cursor:pointer;z-index:10}input[type=range]::-webkit-slider-thumb{"
                                     "-webkit-appearance:none;appearance:none;width:30px;height:30px;background:white;border:3px solid #4ECDC4;"
                                     "border-radius:50%;cursor:pointer;box-shadow:0 2px 8px rgba(0,0,0,0.2)}"
                                     "input[type=range]::-moz-range-thumb{width:30px;height:30px;background:white;border:3px solid #4ECDC4;"
                                     "border-radius:50%;cursor:pointer;box-shadow:0 2px 8px rgba(0,0,0,0.2)}.controls{margin-bottom:25px}"
                                     ".control-btn{width:100%;padding:18px;background:#fff;border:3px solid #e2e8f0;border-radius:8px;"
                                     "font-size:18px;font-weight:bold;font-family:Impact,Arial Black,sans-serif;letter-spacing:2px;"
                                     "cursor:pointer;transition:all 0.3s;display:flex;align-items:center;justify-content:center}"
                                     ".control-btn:hover{border-color:#4ECDC4;transform:translateY(-2px);box-shadow:0 4px 12px rgba(78,205,196,0.3)}"
                                     ".control-btn.play-pause{background:linear-gradient(135deg,#4ECDC4 0%,#44A08D 100%);color:white;border:none;font-size:20px}"
                                     ".control-btn.play-pause:hover{transform:translateY(-3px);box-shadow:0 6px 20px rgba(78,205,196,0.6)}"
                                     ".control-btn.playing{background:linear-gradient(135deg,#FF6B6B 0%,#FF8E53 100%)}"
                                     ".btn-library{padding:14px;background:linear-gradient(135deg,#FFE66D 0%,#FFC857 100%);color:#2d3748;"
                                     "border:none;border-radius:8px;font-size:16px;font-weight:bold;text-transform:uppercase;letter-spacing:2px;"
                                     "cursor:pointer;transition:all 0.3s;box-shadow:0 4px 15px rgba(255,230,109,0.4);width:100%;margin-bottom:15px}"
                                     ".btn-library:hover{transform:translateY(-3px);box-shadow:0 6px 20px rgba(255,230,109,0.6)}"
                                     ".btn-reset{width:100%;padding:12px;background:transparent;color:#718096;border:2px solid #e2e8f0;"
                                     "border-radius:8px;font-size:13px;font-family:Arial,sans-serif;text-transform:uppercase;letter-spacing:1px;"
                                     "cursor:pointer;transition:all 0.3s}.btn-reset:hover{border-color:#FF6B6B;color:#FF6B6B;background:#fff5f5}"
                                     ".footer{text-align:center;margin-top:20px;color:white;font-size:12px;font-family:Arial,sans-serif;"
                                     "text-shadow:1px 1px 2px rgba(0,0,0,0.3)}</style></head><body><div class='container'><div class='card'>"
                                     "<h1>GRIDBEACON</h1><div class='status-bar'><div><div class='status-indicator'><div class='status-dot'></div>"
                                     "<span>"
                + String(playing ? "Streaming" : "Ready") + "</span></div>"
                                                            "<div class='memory-bar'><span>RAM</span><div class='memory-fill'><div class='memory-used' style='width:"
                + String(heapPercent) + "%'></div></div>"
                                        "<span>"
                + String(freeHeap / 1024) + "KB</span></div></div><div class='volume-display'>VOL: <span id='volDisp'>" + String((int)vol) + "</span>%</div></div>"
                + libraryHTML + "<div class='controls'><button class='control-btn play-pause " + String(playing ? "playing" : "") + "' onclick='togglePlay()'>"
                + String(playing ? "⏸ PAUSE" : "▶ PLAY") + "</button></div>"
                                                             "<div class='volume-section'><div class='volume-label'><label style='margin:0'>Volume</label>"
                                                             "<span class='volume-value'><span id='volVal'>"
                + String((int)vol) + "</span>%</span></div>"
                                     "<div class='slider-container'><div class='slider-fill' id='sliderFill'></div>"
                                     "<input type='range' id='volumeSlider' min='0' max='100' value='"
                + String((int)vol) + "' oninput='updateVolume(this.value)'></div></div>"
                                     "<div class='section'><label>Or enter custom URL</label>"
                                     "<input type='url' id='streamUrl' placeholder='https://example.com/stream.mp3'></div>"
                                     "<button class='btn-library' onclick='addLibrary()'>+ ADD TO LIBRARY</button>"
                                     "<button class='btn-reset' onclick='factoryReset()'>Factory Reset</button></div>"
                                     "<div class='footer'>Built by rebels, for rebels</div></div>"
                                     "<script>function updateVolume(v){document.getElementById('volVal').textContent=v;"
                                     "document.getElementById('volDisp').textContent=v;document.getElementById('sliderFill').style.width=v+'%';"
                                     "fetch('/volume',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
                                     "body:'value='+v})}"
                                     "let currentURL='';"
                                     "function playStation(url){currentURL=url;document.getElementById('streamUrl').value=url;"
                                     "fetch('/play',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
                                     "body:'url='+encodeURIComponent(url)}).then(()=>location.reload())}"
                                     "function togglePlay(){const url=document.getElementById('streamUrl').value.trim();"
                                     "if(url){currentURL=url;fetch('/play',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
                                     "body:'url='+encodeURIComponent(url)}).then(()=>location.reload())}"
                                     "else if(currentURL){fetch('/play',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
                                     "body:'url='+encodeURIComponent(currentURL)}).then(()=>location.reload())}"
                                     "else{fetch('/play',{method:'POST'}).then(()=>location.reload())}}"
                                     "function removeStation(idx){if(confirm('Remove this station?'))fetch('/library/remove',{method:'POST',"
                                     "headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'index='+idx}).then(()=>location.reload())}"
                                     "function addLibrary(){let url=document.getElementById('streamUrl').value||currentURL;"
                                     "if(!url)return alert('No station playing');const name=prompt('Station name:');"
                                     "if(!name)return;fetch('/library/add',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},"
                                     "body:'name='+encodeURIComponent(name)+'&url='+encodeURIComponent(url)}).then(r=>r.text())"
                                     ".then(msg=>{alert(msg);location.reload()})}"
                                     "function factoryReset(){if(confirm('Reset all settings?'))fetch('/reset',{method:'POST'})"
                                     ".then(()=>alert('Resetting...'))}</script></body></html>";

  server->send(200, "text/html", html);
}

void WebServerModule::handleSave() {
  if (!server->hasArg("ssid") || !server->hasArg("password") || !server->hasArg("room")) {
    server->send(400, "text/plain", "Missing credentials or room name");
    return;
  }

  String ssid = server->arg("ssid");
  String password = server->arg("password");
  String room = server->arg("room");

  Serial.print("Saving credentials for: ");
  Serial.println(ssid);
  Serial.print("Room name: ");
  Serial.println(room);

  wifiMgr->saveCredentials(ssid.c_str(), password.c_str());

  // Save room name for discovery
  if (discoveryMgr) {
    discoveryMgr->begin(room.c_str());  // This saves the name
  }

  String html = "<!DOCTYPE html><html><head><meta http-equiv='refresh' content='5;url=/'>"
                "<title>Saved</title></head><body>"
                "<h1>Credentials Saved</h1>"
                "<p>GridBeacon ("
                + room + ") is rebooting...</p>"
                         "</body></html>";

  server->send(200, "text/html", html);

  delay(2000);
  ESP.restart();
}

void WebServerModule::handlePlayPause() {
  if (!audioMgr) {
    server->send(503, "text/plain", "Audio not available");
    return;
  }

  // Check if user provided a new URL
  if (server->hasArg("url")) {
    String url = server->arg("url");
    if (url.length() > 0) {
      // Switch to new URL and start playing
      if (audioMgr->setURL(url.c_str())) {
        audioMgr->play();
        server->send(200, "text/plain", "Playing new stream");
      } else {
        server->send(500, "text/plain", "Failed to switch stream");
      }
      return;
    }
  }

  // No URL provided - just toggle play/pause
  if (audioMgr->isPlaying()) {
    audioMgr->pause();
  } else {
    audioMgr->play();
  }

  server->send(200, "text/plain", "OK");
}

void WebServerModule::handleVolume() {
  if (!audioMgr || !server->hasArg("value")) {
    server->send(400, "text/plain", "Missing value");
    return;
  }

  int vol = server->arg("value").toInt();
  audioMgr->setVolume(vol / 100.0);
  server->send(200, "text/plain", "OK");
}

void WebServerModule::handleAddLibrary() {
  if (!libraryMgr || !server->hasArg("name") || !server->hasArg("url")) {
    server->send(400, "text/plain", "Missing parameters");
    return;
  }

  String name = server->arg("name");
  String url = server->arg("url");

  if (libraryMgr->addStation(name.c_str(), url.c_str())) {
    server->send(200, "text/plain", "Added to library");
  } else {
    server->send(507, "text/plain", "Library full");
  }
}

void WebServerModule::handleGetLibrary() {
  if (!libraryMgr) {
    server->send(503, "text/plain", "Library not available");
    return;
  }

  auto stations = libraryMgr->getStations();
  String json = "[";

  for (int i = 0; i < stations.size(); i++) {
    if (i > 0) json += ",";
    json += "{\"name\":\"" + stations[i].name + "\",\"url\":\"" + stations[i].url + "\"}";
  }

  json += "]";
  server->send(200, "application/json", json);
}

void WebServerModule::handleRemoveLibrary() {
  if (!libraryMgr || !server->hasArg("index")) {
    server->send(400, "text/plain", "Missing index");
    return;
  }

  int index = server->arg("index").toInt();

  if (libraryMgr->removeStation(index)) {
    server->send(200, "text/plain", "Removed");
  } else {
    server->send(404, "text/plain", "Not found");
  }
}

void WebServerModule::handleSleepTimer() {
  if (!audioMgr) {
    server->send(503, "text/plain", "Audio not available");
    return;
  }

  if (server->hasArg("cancel")) {
    audioMgr->cancelSleepTimer();
    server->send(200, "text/plain", "Timer cancelled");
    return;
  }

  if (!server->hasArg("minutes")) {
    server->send(400, "text/plain", "Missing minutes");
    return;
  }

  int minutes = server->arg("minutes").toInt();
  if (minutes < 1 || minutes > 180) {
    server->send(400, "text/plain", "Invalid duration (1-180 min)");
    return;
  }

  audioMgr->setSleepTimer(minutes);
  server->send(200, "text/plain", "Sleep timer set");
}

void WebServerModule::handleSettings() {
  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><meta name='viewport' content='width=device-width,initial-scale=1'><title>GridBeacon Settings</title>";
  html += "<style>*{margin:0;padding:0;box-sizing:border-box}";
  html += "body{font-family:Impact,Arial Black,sans-serif;background:linear-gradient(135deg,#667eea 0%,#764ba2 100%);min-height:100vh;display:flex;align-items:center;justify-content:center;padding:20px}";
  html += ".container{max-width:480px;width:100%}.card{background:#fff;border-radius:16px;padding:30px;box-shadow:0 20px 60px rgba(0,0,0,0.3);position:relative;overflow:hidden}";
  html += ".card::before{content:'';position:absolute;top:0;left:0;right:0;height:8px;background:linear-gradient(90deg,#FF6B6B,#FFE66D,#4ECDC4,#FF6B6B);background-size:200% 100%;animation:slideGradient 3s linear infinite}";
  html += "@keyframes slideGradient{0%{background-position:0% 50%}100%{background-position:200% 50%}}";
  html += "h1{font-size:38px;color:#2d3748;text-transform:uppercase;letter-spacing:2px;margin-bottom:20px;text-shadow:3px 3px 0px #FFE66D}";
  html += ".section{margin-bottom:25px}label{display:block;font-size:13px;color:#4a5568;margin-bottom:8px;text-transform:uppercase;letter-spacing:1px;font-weight:bold}";
  html += ".network-list{display:flex;flex-direction:column;gap:10px}";
  html += ".network-item{background:#f7fafc;border:2px solid #e2e8f0;border-radius:8px;padding:12px 14px;display:flex;justify-content:space-between;align-items:center}";
  html += ".network-name{font-family:Arial,sans-serif;font-size:15px;color:#2d3748;font-weight:bold}";
  html += ".btn-remove{background:#ff6b6b;color:white;border:none;border-radius:50%;width:24px;height:24px;font-size:14px;cursor:pointer;transition:all 0.2s}";
  html += ".btn-remove:hover{background:#ff5252;transform:scale(1.1)}";
  html += ".btn-back{width:100%;padding:14px;background:linear-gradient(135deg,#4ECDC4 0%,#44A08D 100%);color:white;border:none;border-radius:8px;font-size:16px;font-weight:bold;text-transform:uppercase;letter-spacing:2px;cursor:pointer;transition:all 0.3s;margin-bottom:15px}";
  html += ".btn-back:hover{transform:translateY(-3px);box-shadow:0 6px 20px rgba(78,205,196,0.6)}";
  html += ".btn-reset{width:100%;padding:12px;background:transparent;color:#718096;border:2px solid #e2e8f0;border-radius:8px;font-size:13px;font-family:Arial,sans-serif;text-transform:uppercase;letter-spacing:1px;cursor:pointer;transition:all 0.3s}";
  html += ".btn-reset:hover{border-color:#FF6B6B;color:#FF6B6B;background:#fff5f5}";
  html += ".footer{text-align:center;margin-top:20px;color:white;font-size:12px;font-family:Arial,sans-serif;text-shadow:1px 1px 2px rgba(0,0,0,0.3)}";
  html += "</style></head><body><div class='container'><div class='card'>";
  html += "<h1>SETTINGS</h1>";

  // Saved WiFi Networks
  html += "<div class='section'><label>Saved WiFi Networks</label>";
  if (wifiMgr->getNetworkCount() > 0) {
    html += "<div class='network-list'>";
    SavedNetwork* networks = wifiMgr->getNetworks();
    for (int i = 0; i < wifiMgr->getNetworkCount(); i++) {
      html += "<div class='network-item'><div class='network-name'>" + networks[i].ssid + "</div>";
      html += "<button class='btn-remove' onclick='removeNetwork(" + String(i) + ")'>✕</button></div>";
    }
    html += "</div>";
  } else {
    html += "<p style='color:#718096;font-family:Arial;font-size:14px;text-align:center;padding:20px'>No saved networks</p>";
  }
  html += "</div>";

  html += "<button class='btn-back' onclick='window.location=\"/\"'>← BACK TO PLAYER</button>";
  html += "<button class='btn-reset' onclick='factoryReset()'>Factory Reset</button>";
  html += "</div><div class='footer'>Built by rebels, for rebels</div></div>";

  html += "<script>";
  html += "function removeNetwork(idx){if(confirm('Forget this network?'))fetch('/network/remove',{method:'POST',headers:{'Content-Type':'application/x-www-form-urlencoded'},body:'index='+idx}).then(()=>location.reload())}";
  html += "function factoryReset(){if(confirm('Reset all settings?'))fetch('/reset',{method:'POST'}).then(()=>alert('Resetting...'))}";
  html += "</script></body></html>";

  server->send(200, "text/html", html);
}

void WebServerModule::handleRemoveNetwork() {
  if (!wifiMgr || !server->hasArg("index")) {
    server->send(400, "text/plain", "Missing index");
    return;
  }

  int index = server->arg("index").toInt();

  if (wifiMgr->removeNetwork(index)) {
    server->send(200, "text/plain", "Network removed");
  } else {
    server->send(404, "text/plain", "Network not found");
  }
}

void WebServerModule::handleReset() {
  wifiMgr->clearCredentials();
  if (libraryMgr) {
    // Clear library too
    for (int i = 0; i < 10; i++) {
      libraryMgr->removeStation(0);
    }
  }

  server->send(200, "text/plain", "Resetting...");
  delay(1000);
  ESP.restart();
}

void WebServerModule::handleNotFound() {
  // Redirect all 404s to root in AP mode, otherwise show player
  if (wifiMgr->getMode() == MODE_AP) {
    handleRoot();
  } else {
    handlePlayer();
  }
}