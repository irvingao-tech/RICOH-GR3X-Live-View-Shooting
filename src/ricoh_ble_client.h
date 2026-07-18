#pragma once

#include <Arduino.h>

struct RicohBleDeviceInfo {
  bool found = false;
  String name;
  String address;
  uint8_t addressType = 0;
  int rssi = 0;
  bool connectable = false;
  bool hasInfoService = false;
  bool hasCameraService = false;
  bool hasShootingService = false;
  bool hasControlService = false;
};

struct RicohBleWifiCredentials {
  bool valid = false;
  bool encryptedPassphrase = false;
  int securityType = -1;
  uint16_t frequencyMhz = 0;
  uint8_t channel = 0;
  String ssid;
  String passphrase;
  String bssid;
};

struct RicohBleConnectOptions {
  uint32_t timeoutMs = 0;
  uint32_t securityWaitMs = 0;
  uint32_t preConnectDelayMs = 0;
  bool exchangeMtu = true;
};

struct RicohGpsFix {
  double latitude = 0.0;
  double longitude = 0.0;
  double altitudeMeters = 0.0;
  uint16_t year = 0;
  uint8_t month = 0;
  uint8_t day = 0;
  uint8_t hour = 0;
  uint8_t minute = 0;
  uint8_t second = 0;
};

enum class RicohCameraPowerState {
  Unknown,
  On,
  OffOrShuttingDown,
};

enum class RicohCameraOperationMode {
  Unknown,
  Capture,
  Playback,
  BleStartup,
  Other,
  PowerOffTransfer,
};

class RicohBleClient {
public:
  using ServiceCallback = bool (*)();

  void begin();
  void setServiceCallback(ServiceCallback callback);
  RicohBleDeviceInfo scanForCamera(const String& preferredAddress, const String& preferredName, uint32_t scanSeconds);
  bool connect(const RicohBleDeviceInfo& info, uint32_t timeoutMs);
  bool connect(const RicohBleDeviceInfo& info, const RicohBleConnectOptions& options);
  bool isBonded(const RicohBleDeviceInfo& info);
  bool isConnected() const;
  bool shutterReady() const;
  bool shoot(bool autofocus = true);
  bool openWifi();
  bool readPowerState(RicohCameraPowerState& state);
  bool readOperationMode(RicohCameraOperationMode& mode);
  bool enablePowerStateNotify();
  bool consumePowerOffNotification();
  bool waitForWifiCredentials(RicohBleWifiCredentials& credentials, uint32_t timeoutMs);
  bool updateGps(const RicohGpsFix& fix);
  void disconnect();
  int consumeDisconnectReason();
  void clearDisconnectReason();
  bool deleteAllBonds();
  bool pairingPasskeyPending() const;
  bool submitPairingPasskey(uint32_t passkey);
  void resetStack(bool clearObjects = true);
  bool lastFailureWasResourceExhausted() const;

  String statusText() const;
  const String& lastError() const;

private:
  bool _begun = false;
  bool _connected = false;
  bool _lastFailureResourceExhausted = false;
  String _lastError;
  void* _client = nullptr;
};
