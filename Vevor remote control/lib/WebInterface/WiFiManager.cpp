/**
 * WiFi Manager Implementation
 */

#include "WiFiManager.h"

WiFiManager::WiFiManager()
  : _apMode(false), _lastReconnectAttempt(0) {
}

void WiFiManager::begin() {
  _prefs.begin("wifi", false);
  loadCredentials();

  Serial.println("\n========== WiFi Manager ==========");
  Serial.printf("SSID: %s\n", _ssid.length() > 0 ? _ssid.c_str() : "(not set)");
  Serial.printf("Saved: %s\n", hasCredentials() ? "YES" : "NO");
  Serial.println("==================================\n");

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
}

void WiFiManager::setSSID(const String& ssid) {
  _ssid = ssid;
  saveCredentials();
  Serial.printf("WiFi SSID set: %s\n", ssid.c_str());
}

void WiFiManager::setPassword(const String& password) {
  _password = password;
  saveCredentials();
  Serial.println("WiFi password set (hidden)");
}

bool WiFiManager::connect(unsigned long timeout_ms) {
  if (!hasCredentials()) {
    Serial.println("ERROR: No WiFi credentials configured");
    return false;
  }

  Serial.printf("\nConnecting to WiFi: %s\n", _ssid.c_str());
  WiFi.begin(_ssid.c_str(), _password.c_str());

  unsigned long start = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - start) < timeout_ms) {
    delay(500);
    Serial.print(".");
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("WiFi connected!");
    Serial.printf("IP address: %s\n", WiFi.localIP().toString().c_str());
    Serial.printf("Signal: %d dBm\n", WiFi.RSSI());
    return true;
  } else {
    Serial.println("WiFi connection FAILED");
    return false;
  }
}

bool WiFiManager::startAP(const String& ssid, const String& password) {
  String apSSID = ssid;
  if (apSSID.length() == 0) {
    // Generate default AP name with chip ID
    uint32_t chipId = 0;
    for(int i=0; i<17; i=i+8) {
      chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
    }
    apSSID = "Heater-" + String(chipId, HEX);
  }

  Serial.printf("\nStarting AP mode: %s\n", apSSID.c_str());

  WiFi.mode(WIFI_AP);
  bool success = WiFi.softAP(apSSID.c_str(), password.c_str());

  if (success) {
    _apMode = true;
    Serial.println("AP started!");
    Serial.printf("IP address: %s\n", WiFi.softAPIP().toString().c_str());
    Serial.printf("Password: %s\n", password.c_str());
  } else {
    Serial.println("AP start FAILED");
  }

  return success;
}

String WiFiManager::getIP() const {
  if (_apMode) {
    return WiFi.softAPIP().toString();
  } else {
    return WiFi.localIP().toString();
  }
}

String WiFiManager::getStatusString() const {
  if (_apMode) {
    return "AP Mode";
  }

  switch (WiFi.status()) {
    case WL_CONNECTED:
      return "Connected";
    case WL_NO_SSID_AVAIL:
      return "SSID not found";
    case WL_CONNECT_FAILED:
      return "Connection failed";
    case WL_CONNECTION_LOST:
      return "Connection lost";
    case WL_DISCONNECTED:
      return "Disconnected";
    default:
      return "Unknown";
  }
}

void WiFiManager::update() {
  // Auto-reconnect if connection lost (only in STA mode)
  if (!_apMode && WiFi.status() != WL_CONNECTED && hasCredentials()) {
    if (millis() - _lastReconnectAttempt > 30000) {  // Try every 30 seconds
      _lastReconnectAttempt = millis();
      Serial.println("WiFi disconnected, attempting reconnect...");
      connect(10000);  // 10 second timeout
    }
  }
}

void WiFiManager::loadCredentials() {
  _ssid = _prefs.getString("ssid", "");
  _password = _prefs.getString("password", "");
}

void WiFiManager::saveCredentials() {
  _prefs.putString("ssid", _ssid);
  _prefs.putString("password", _password);
}
