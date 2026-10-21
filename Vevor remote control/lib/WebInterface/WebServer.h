/**
 * Web Server - REST API for heater control and SignalK integration
 *
 * Features:
 * - REST API endpoints for status, control, and configuration
 * - PIN authentication required for control operations
 * - CORS support for web integration
 * - JSON responses using ArduinoJson
 *
 * API Endpoints:
 * - GET  /api/status         - Full heater status (public)
 * - GET  /api/temperature    - Temperature data (public)
 * - POST /api/authenticate   - Verify PIN, get session token
 * - POST /api/start          - Start heater (requires auth)
 * - POST /api/stop           - Stop heater (requires auth)
 * - POST /api/target         - Set target temperature (requires auth)
 * - POST /api/power          - Set power level (requires auth)
 * - GET  /api/signalk/values - SignalK compatible format
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <Arduino.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <AuthManager.h>

// Forward declarations for heater data access
class VevorHeater;

class WebServer {
public:
  WebServer(AuthManager& authManager, uint16_t port = 80);

  /**
   * Initialize web server and register endpoints
   */
  void begin();

  /**
   * Set heater reference for data access
   */
  void setHeater(VevorHeater* heater) { _heater = heater; }

  /**
   * Set temperature data references
   */
  void setTemperatureData(float* current, float* smoothed, float* trend, int* target) {
    _currentTemp = current;
    _smoothedTemp = smoothed;
    _tempTrend = trend;
    _targetTemp = target;
  }

  /**
   * Set control mode reference
   */
  void setAutoControlEnabled(bool* enabled) { _autoControlEnabled = enabled; }

  /**
   * Set control callbacks (called by API endpoints)
   */
  void setStartCallback(void (*callback)(void)) { _startCallback = callback; }
  void setStopCallback(void (*callback)(void)) { _stopCallback = callback; }
  void setTargetCallback(void (*callback)(int)) { _targetCallback = callback; }
  void setPowerCallback(void (*callback)(int)) { _powerCallback = callback; }

  /**
   * Get underlying AsyncWebServer for adding additional handlers (e.g. OTA)
   */
  AsyncWebServer& getServer() { return _server; }

private:
  AsyncWebServer _server;
  AuthManager& _auth;

  // Data references
  VevorHeater* _heater;
  float* _currentTemp;
  float* _smoothedTemp;
  float* _tempTrend;
  int* _targetTemp;
  bool* _autoControlEnabled;

  // Control callbacks
  void (*_startCallback)(void);
  void (*_stopCallback)(void);
  void (*_targetCallback)(int);
  void (*_powerCallback)(int);

  // Endpoint handlers
  void handleStatus(AsyncWebServerRequest* request);
  void handleTemperature(AsyncWebServerRequest* request);
  void handleAuthenticate(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleStart(AsyncWebServerRequest* request);
  void handleStop(AsyncWebServerRequest* request);
  void handleTarget(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handlePower(AsyncWebServerRequest* request, uint8_t* data, size_t len);
  void handleSignalK(AsyncWebServerRequest* request);

  // Helper methods
  bool verifyAuth(AsyncWebServerRequest* request);
  void sendError(AsyncWebServerRequest* request, int code, const char* message);
  void sendJSON(AsyncWebServerRequest* request, JsonDocument& doc);
};

#endif
