/**
 * WiFi Manager - Handle WiFi credentials and connection
 *
 * Features:
 * - Store/load WiFi SSID and password in Preferences
 * - Connect to WiFi network
 * - AP mode fallback if connection fails
 * - Status reporting
 */

#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <Preferences.h>

class WiFiManager {
public:
  WiFiManager();

  /**
   * Initialize WiFi manager and load saved credentials
   */
  void begin();

  /**
   * Set WiFi SSID
   */
  void setSSID(const String& ssid);

  /**
   * Set WiFi password
   */
  void setPassword(const String& password);

  /**
   * Get current SSID
   */
  String getSSID() const { return _ssid; }

  /**
   * Check if credentials are configured
   */
  bool hasCredentials() const { return _ssid.length() > 0; }

  /**
   * Connect to WiFi using saved credentials
   * @param timeout_ms Timeout in milliseconds
   * @return true if connected successfully
   */
  bool connect(unsigned long timeout_ms = 30000);

  /**
   * Start AP mode for configuration
   * @param ssid AP name (default: "Heater-XXXXXX")
   * @param password AP password (default: "heater123")
   */
  bool startAP(const String& ssid = "", const String& password = "heater123");

  /**
   * Check if currently connected
   */
  bool isConnected() const { return WiFi.status() == WL_CONNECTED; }

  /**
   * Get IP address
   */
  String getIP() const;

  /**
   * Get connection status string
   */
  String getStatusString() const;

  /**
   * Update - call in loop()
   */
  void update();

private:
  Preferences _prefs;
  String _ssid;
  String _password;
  bool _apMode;
  unsigned long _lastReconnectAttempt;

  void loadCredentials();
  void saveCredentials();
};

#endif
