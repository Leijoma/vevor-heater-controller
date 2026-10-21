/**
 * Web Server Implementation
 */

#include "WebServer.h"
#include "WebUI.h"
#include <VevorHeater.h>

WebServer::WebServer(AuthManager& authManager, uint16_t port)
  : _server(port), _auth(authManager),
    _heater(nullptr), _currentTemp(nullptr), _smoothedTemp(nullptr),
    _tempTrend(nullptr), _targetTemp(nullptr), _autoControlEnabled(nullptr),
    _startCallback(nullptr), _stopCallback(nullptr),
    _targetCallback(nullptr), _powerCallback(nullptr) {
}

void WebServer::begin() {
  Serial.println("\n========== Web Server ==========");

  // Enable CORS for all endpoints
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
  DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type, Authorization");

  // GET / - Serve web interface
  _server.on("/", HTTP_GET, [](AsyncWebServerRequest* request) {
    request->send_P(200, "text/html", WEBUI_HTML);
  });

  // GET /api/status - Full heater status (public)
  _server.on("/api/status", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleStatus(request);
  });

  // GET /api/temperature - Temperature data only (public)
  _server.on("/api/temperature", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleTemperature(request);
  });

  // POST /api/authenticate - Verify PIN, get token
  _server.on("/api/authenticate", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        handleAuthenticate(request, data, len);
      }
    }
  );

  // POST /api/start - Start heater (requires auth)
  _server.on("/api/start", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleStart(request);
  });

  // POST /api/stop - Stop heater (requires auth)
  _server.on("/api/stop", HTTP_POST, [this](AsyncWebServerRequest* request) {
    handleStop(request);
  });

  // POST /api/target - Set target temperature (requires auth)
  _server.on("/api/target", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        handleTarget(request, data, len);
      }
    }
  );

  // POST /api/power - Set power level (requires auth)
  _server.on("/api/power", HTTP_POST,
    [](AsyncWebServerRequest* request) {},
    nullptr,
    [this](AsyncWebServerRequest* request, uint8_t* data, size_t len, size_t index, size_t total) {
      if (index == 0) {
        handlePower(request, data, len);
      }
    }
  );

  // GET /api/signalk/values - SignalK format
  _server.on("/api/signalk/values", HTTP_GET, [this](AsyncWebServerRequest* request) {
    handleSignalK(request);
  });

  // 404 handler
  _server.onNotFound([](AsyncWebServerRequest* request) {
    request->send(404, "application/json", "{\"error\":\"Not found\"}");
  });

  _server.begin();
  Serial.println("Web server started on port 80");
  Serial.println("================================\n");
}

void WebServer::handleStatus(AsyncWebServerRequest* request) {
  if (!_heater || !_currentTemp || !_targetTemp) {
    sendError(request, 500, "Server not initialized");
    return;
  }

  JsonDocument doc;

  // Temperature data
  doc["temperature"]["current"] = *_currentTemp;
  doc["temperature"]["smoothed"] = *_smoothedTemp;
  doc["temperature"]["trend"] = *_tempTrend;
  doc["temperature"]["target"] = *_targetTemp;

  // Heater state
  HeaterState state = _heater->getState();
  doc["heater"]["state"] = VevorHeater::getStateString(state);
  doc["heater"]["power"] = (state == STATE_OFF || state == STATE_COOLDOWN) ? 0 : _heater->getPower();
  doc["heater"]["voltage"] = _heater->getVoltage();
  doc["heater"]["fanSpeed"] = _heater->getFanSpeed();
  doc["heater"]["pumpFrequency"] = _heater->getPumpFrequency();
  doc["heater"]["temperature"] = _heater->getTemperature();  // Heat exchanger temperature
  doc["heater"]["running"] = (state != STATE_OFF);

  // Error information
  HeaterError error = _heater->getError();
  doc["heater"]["error"]["code"] = static_cast<uint8_t>(error);
  doc["heater"]["error"]["message"] = VevorHeater::getErrorString(error);
  doc["heater"]["hasError"] = _heater->hasError();

  // Control mode
  doc["control"]["mode"] = (_autoControlEnabled && *_autoControlEnabled) ? "auto" : "manual";

  sendJSON(request, doc);
}

void WebServer::handleTemperature(AsyncWebServerRequest* request) {
  if (!_currentTemp) {
    sendError(request, 500, "Temperature sensor not initialized");
    return;
  }

  JsonDocument doc;
  doc["current"] = *_currentTemp;
  doc["smoothed"] = *_smoothedTemp;
  doc["trend"] = *_tempTrend;
  doc["target"] = *_targetTemp;

  sendJSON(request, doc);
}

void WebServer::handleAuthenticate(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    sendError(request, 400, "Invalid JSON");
    return;
  }

  const char* pin = doc["pin"];
  if (!pin) {
    sendError(request, 400, "PIN required");
    return;
  }

  if (_auth.verifyPIN(String(pin))) {
    String token = _auth.createToken();
    JsonDocument response;
    response["success"] = true;
    response["token"] = token;
    response["message"] = "Authentication successful";
    sendJSON(request, response);
    Serial.printf("API: PIN verified, token issued\n");
  } else {
    sendError(request, 401, "Invalid PIN");
    Serial.println("API: Invalid PIN attempt");
  }
}

void WebServer::handleStart(AsyncWebServerRequest* request) {
  if (!verifyAuth(request)) {
    sendError(request, 401, "Authentication required");
    return;
  }

  if (_startCallback) {
    _startCallback();
    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Heater start command sent";
    sendJSON(request, doc);
    Serial.println("API: Start command executed");
  } else {
    sendError(request, 500, "Start callback not set");
  }
}

void WebServer::handleStop(AsyncWebServerRequest* request) {
  if (!verifyAuth(request)) {
    sendError(request, 401, "Authentication required");
    return;
  }

  if (_stopCallback) {
    _stopCallback();
    JsonDocument doc;
    doc["success"] = true;
    doc["message"] = "Heater stop command sent";
    sendJSON(request, doc);
    Serial.println("API: Stop command executed");
  } else {
    sendError(request, 500, "Stop callback not set");
  }
}

void WebServer::handleTarget(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  if (!verifyAuth(request)) {
    sendError(request, 401, "Authentication required");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    sendError(request, 400, "Invalid JSON");
    return;
  }

  int target = doc["target"];
  if (target < 5 || target > 35) {
    sendError(request, 400, "Target must be between 5 and 35°C");
    return;
  }

  if (_targetCallback) {
    _targetCallback(target);
    JsonDocument response;
    response["success"] = true;
    response["target"] = target;
    response["message"] = "Target temperature set";
    sendJSON(request, response);
    Serial.printf("API: Target set to %d°C\n", target);
  } else {
    sendError(request, 500, "Target callback not set");
  }
}

void WebServer::handlePower(AsyncWebServerRequest* request, uint8_t* data, size_t len) {
  if (!verifyAuth(request)) {
    sendError(request, 401, "Authentication required");
    return;
  }

  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, data, len);

  if (error) {
    sendError(request, 400, "Invalid JSON");
    return;
  }

  int power = doc["power"];
  if (power < 1 || power > 10) {
    sendError(request, 400, "Power must be between 1 and 10");
    return;
  }

  if (_powerCallback) {
    _powerCallback(power);
    JsonDocument response;
    response["success"] = true;
    response["power"] = power;
    response["message"] = "Power level set";
    sendJSON(request, response);
    Serial.printf("API: Power set to %d\n", power);
  } else {
    sendError(request, 500, "Power callback not set");
  }
}

void WebServer::handleSignalK(AsyncWebServerRequest* request) {
  if (!_heater || !_currentTemp || !_targetTemp) {
    sendError(request, 500, "Server not initialized");
    return;
  }

  JsonDocument doc;

  // SignalK format for environment.inside.temperature
  doc["environment"]["inside"]["cabin"]["temperature"]["value"] = *_currentTemp;
  doc["environment"]["inside"]["cabin"]["temperature"]["$source"] = "vevor-heater";
  doc["environment"]["inside"]["cabin"]["temperature"]["timestamp"] = millis();

  // Heater status
  HeaterState state = _heater->getState();
  doc["environment"]["inside"]["cabin"]["heater"]["state"]["value"] = VevorHeater::getStateString(state);
  doc["environment"]["inside"]["cabin"]["heater"]["power"]["value"] = (state == STATE_OFF || state == STATE_COOLDOWN) ? 0 : _heater->getPower();
  doc["environment"]["inside"]["cabin"]["heater"]["targetTemperature"]["value"] = *_targetTemp;
  doc["environment"]["inside"]["cabin"]["heater"]["$source"] = "vevor-heater";

  sendJSON(request, doc);
}

bool WebServer::verifyAuth(AsyncWebServerRequest* request) {
  // Check for Authorization header with Bearer token
  if (request->hasHeader("Authorization")) {
    String authHeader = request->header("Authorization");
    if (authHeader.startsWith("Bearer ")) {
      String token = authHeader.substring(7);
      return _auth.verifyToken(token);
    }
  }
  return false;
}

void WebServer::sendError(AsyncWebServerRequest* request, int code, const char* message) {
  JsonDocument doc;
  doc["error"] = message;
  doc["code"] = code;

  String response;
  serializeJson(doc, response);
  request->send(code, "application/json", response);
}

void WebServer::sendJSON(AsyncWebServerRequest* request, JsonDocument& doc) {
  String response;
  serializeJson(doc, response);
  request->send(200, "application/json", response);
}
