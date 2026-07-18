#pragma once

#include <Arduino.h>
#include <TinyGPSPlus.h>

#include "../ricoh_ble_client.h"

namespace rvf {

class GpsService {
public:
  GpsService();

  bool begin();
  void poll();
  bool hasFreshFix(uint32_t maxAgeMs) const;
  const RicohGpsFix& fix() const;
  uint32_t fixAgeMs() const;
  uint32_t satellites();

private:
  HardwareSerial _serial;
  TinyGPSPlus _gps;
  RicohGpsFix _fix;
  uint32_t _lastFixAt = 0;
  bool _started = false;
};

}  // namespace rvf
