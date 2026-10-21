/**
 * Authentication Manager - Handle PIN code authentication
 *
 * Features:
 * - 4-digit PIN code
 * - Secure storage (hashed)
 * - Session management
 * - Default PIN: 1234
 */

#ifndef AUTH_MANAGER_H
#define AUTH_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include <mbedtls/md.h>

class AuthManager {
public:
  AuthManager();

  /**
   * Initialize authentication manager
   */
  void begin();

  /**
   * Set new PIN code
   * @param pin 4-digit PIN code
   * @return true if valid and saved
   */
  bool setPIN(const String& pin);

  /**
   * Verify PIN code
   * @param pin 4-digit PIN to verify
   * @return true if correct
   */
  bool verifyPIN(const String& pin);

  /**
   * Check if PIN is set (not default)
   */
  bool hasPIN() const { return _pinSet; }

  /**
   * Create authentication token
   * @return Session token (valid for 1 hour)
   */
  String createToken();

  /**
   * Verify authentication token
   * @param token Session token to verify
   * @return true if valid and not expired
   */
  bool verifyToken(const String& token);

  /**
   * Invalidate all tokens (logout)
   */
  void invalidateTokens();

private:
  Preferences _prefs;
  bool _pinSet;
  String _pinHash;
  String _salt;
  String _currentToken;
  unsigned long _tokenExpiry;

  String hashPIN(const String& pin, const String& salt);
  String generateSalt();
  String generateToken();
  bool isValidPIN(const String& pin);
};

#endif
