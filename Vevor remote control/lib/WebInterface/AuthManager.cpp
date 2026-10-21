/**
 * Authentication Manager Implementation
 */

#include "AuthManager.h"

// Default PIN: 1234
const char* DEFAULT_PIN = "1234";
const unsigned long TOKEN_VALIDITY_MS = 3600000;  // 1 hour

AuthManager::AuthManager()
  : _pinSet(false), _tokenExpiry(0) {
}

void AuthManager::begin() {
  _prefs.begin("auth", false);

  // Load saved PIN hash and salt
  _pinHash = _prefs.getString("pin_hash", "");
  _salt = _prefs.getString("salt", "");

  if (_pinHash.length() == 0) {
    // No PIN set, use default
    Serial.println("\n========== Auth Manager ==========");
    Serial.println("No PIN configured");
    Serial.printf("Default PIN: %s\n", DEFAULT_PIN);
    Serial.println("Use 'pin set XXXX' to change");
    Serial.println("==================================\n");

    // Generate salt and hash default PIN
    _salt = generateSalt();
    _pinHash = hashPIN(DEFAULT_PIN, _salt);
    _prefs.putString("pin_hash", _pinHash);
    _prefs.putString("salt", _salt);
    _pinSet = false;
  } else {
    Serial.println("\n========== Auth Manager ==========");
    Serial.println("PIN configured: YES");
    Serial.println("==================================\n");
    _pinSet = true;
  }
}

bool AuthManager::setPIN(const String& pin) {
  if (!isValidPIN(pin)) {
    Serial.println("ERROR: PIN must be exactly 4 digits");
    return false;
  }

  // Generate new salt for new PIN
  _salt = generateSalt();
  _pinHash = hashPIN(pin, _salt);

  _prefs.putString("pin_hash", _pinHash);
  _prefs.putString("salt", _salt);
  _pinSet = true;

  // Invalidate any existing tokens
  invalidateTokens();

  Serial.println("PIN code changed successfully");
  Serial.println("All tokens invalidated");

  return true;
}

bool AuthManager::verifyPIN(const String& pin) {
  if (!isValidPIN(pin)) {
    return false;
  }

  String hash = hashPIN(pin, _salt);
  bool valid = (hash == _pinHash);

  if (valid) {
    Serial.println("PIN verified successfully");
  } else {
    Serial.println("PIN verification FAILED");
  }

  return valid;
}

String AuthManager::createToken() {
  _currentToken = generateToken();
  _tokenExpiry = millis() + TOKEN_VALIDITY_MS;

  Serial.println("Authentication token created");
  Serial.printf("Token: %s\n", _currentToken.c_str());
  Serial.printf("Valid for: %lu minutes\n", TOKEN_VALIDITY_MS / 60000);

  return _currentToken;
}

bool AuthManager::verifyToken(const String& token) {
  if (token.length() == 0 || _currentToken.length() == 0) {
    return false;
  }

  if (token != _currentToken) {
    return false;
  }

  // Check expiry
  if (millis() > _tokenExpiry) {
    Serial.println("Token expired");
    invalidateTokens();
    return false;
  }

  return true;
}

void AuthManager::invalidateTokens() {
  _currentToken = "";
  _tokenExpiry = 0;
  Serial.println("All tokens invalidated");
}

String AuthManager::hashPIN(const String& pin, const String& salt) {
  // Combine PIN and salt
  String combined = pin + salt;

  // Use SHA256 for hashing
  byte shaResult[32];
  mbedtls_md_context_t ctx;
  mbedtls_md_type_t md_type = MBEDTLS_MD_SHA256;

  mbedtls_md_init(&ctx);
  mbedtls_md_setup(&ctx, mbedtls_md_info_from_type(md_type), 0);
  mbedtls_md_starts(&ctx);
  mbedtls_md_update(&ctx, (const unsigned char*)combined.c_str(), combined.length());
  mbedtls_md_finish(&ctx, shaResult);
  mbedtls_md_free(&ctx);

  // Convert to hex string
  String hash = "";
  for (int i = 0; i < 32; i++) {
    char str[3];
    sprintf(str, "%02x", (int)shaResult[i]);
    hash += str;
  }

  return hash;
}

String AuthManager::generateSalt() {
  // Generate random 16-byte salt
  String salt = "";
  for (int i = 0; i < 16; i++) {
    byte randomByte = random(256);
    char str[3];
    sprintf(str, "%02x", randomByte);
    salt += str;
  }
  return salt;
}

String AuthManager::generateToken() {
  // Generate random 32-byte token
  String token = "";
  for (int i = 0; i < 32; i++) {
    byte randomByte = random(256);
    char str[3];
    sprintf(str, "%02x", randomByte);
    token += str;
  }
  return token;
}

bool AuthManager::isValidPIN(const String& pin) {
  // Must be exactly 4 characters
  if (pin.length() != 4) {
    return false;
  }

  // All characters must be digits
  for (int i = 0; i < 4; i++) {
    if (!isDigit(pin.charAt(i))) {
      return false;
    }
  }

  return true;
}
