#pragma once

#include <Arduino.h>

static const size_t DEFAULT_ESPNOW_ENCRYPTION_KEY_LENGTH = 4;
static const uint8_t DEFAULT_ESPNOW_ENCRYPTION_KEY[DEFAULT_ESPNOW_ENCRYPTION_KEY_LENGTH] = {
  0x4B, 0xA3, 0x3F, 0x9C
};
static const size_t MAX_ESPNOW_ENCRYPTION_KEY_LENGTH = 16;

static inline String defaultEspNowEncryptionKeyString() {
  String text = "[";
  for (size_t i = 0; i < DEFAULT_ESPNOW_ENCRYPTION_KEY_LENGTH; ++i) {
    if (i > 0) text += ",";
    char buffer[7];
    snprintf(buffer, sizeof(buffer), "0x%02X", DEFAULT_ESPNOW_ENCRYPTION_KEY[i]);
    text += "\"";
    text += buffer;
    text += "\"";
  }
  text += "]";
  return text;
}

static inline void xorInPlace(uint8_t *data, size_t len, const uint8_t *key, size_t keyLength) {
  if (data == nullptr || key == nullptr || keyLength == 0) return;
  for (size_t i = 0; i < len; ++i) {
    data[i] ^= key[i % keyLength];
  }
}

static inline bool parseEspNowEncryptionKey(const String &rawValue,
                                            uint8_t *outKey, size_t &outLength) {
  if (outKey == nullptr) return false;

  String normalized = rawValue;
  normalized.trim();
  if (normalized.startsWith("[")) normalized.remove(0, 1);
  if (normalized.endsWith("]")) normalized.remove(normalized.length() - 1);

  size_t index = 0;
  int start = 0;
  while (start < normalized.length()) {
    if (index >= MAX_ESPNOW_ENCRYPTION_KEY_LENGTH) return false;
    int comma = normalized.indexOf(',', start);
    String token = comma == -1 ? normalized.substring(start) : normalized.substring(start, comma);
    token.trim();
    if (token.startsWith("\"") && token.endsWith("\"") && token.length() >= 2) {
      token = token.substring(1, token.length() - 1);
      token.trim();
    }
    if (token.length() == 0) return false;

    char *endPtr = nullptr;
    long value = strtol(token.c_str(), &endPtr, 0);
    if (endPtr == token.c_str() || *endPtr != '\0' || value < 0 || value > 255) return false;

    outKey[index++] = static_cast<uint8_t>(value);
    if (comma == -1) break;
    start = comma + 1;
  }

  if (index == 0) return false;
  outLength = index;
  return true;
}
