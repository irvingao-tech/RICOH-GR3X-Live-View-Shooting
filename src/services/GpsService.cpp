#include "GpsService.h"

#include <M5Unified.h>

#include "../config.h"

namespace rvf {

GpsService::GpsService() : _serial(1) {}

bool GpsService::begin() {
  const int8_t rx = GPS_UART_RX_PIN;
  const int8_t tx = GPS_UART_TX_PIN;
  if (rx < 0 || tx < 0) {
    Serial.printf("GPS: UART pins unavailable rx=%d tx=%d\n", rx, tx);
    return false;
  }

  _serial.begin(GPS_UART_BAUD, SERIAL_8N1, rx, tx);
  _started = true;
  Serial.printf("GPS: AT6668 UART ready baud=%lu rx=G%d tx=G%d\n",
                static_cast<unsigned long>(GPS_UART_BAUD), rx, tx);
  return true;
}

void GpsService::poll() {
  if (!_started) {
    return;
  }

  size_t processed = 0;
  while (_serial.available() > 0 && processed < 4096) {
    _gps.encode(static_cast<char>(_serial.read()));
    ++processed;
  }

  if (!_gps.location.isUpdated() || !_gps.location.isValid() ||
      !_gps.date.isValid() || !_gps.time.isValid()) {
    return;
  }

  _fix.latitude = _gps.location.lat();
  _fix.longitude = _gps.location.lng();
  _fix.altitudeMeters = _gps.altitude.isValid() ? _gps.altitude.meters() : 0.0;
  _fix.year = _gps.date.year();
  _fix.month = _gps.date.month();
  _fix.day = _gps.date.day();
  _fix.hour = _gps.time.hour();
  _fix.minute = _gps.time.minute();
  _fix.second = _gps.time.second();
  _lastFixAt = millis();
}

bool GpsService::hasFreshFix(uint32_t maxAgeMs) const {
  return _lastFixAt != 0 && (millis() - _lastFixAt) <= maxAgeMs;
}

const RicohGpsFix& GpsService::fix() const {
  return _fix;
}

uint32_t GpsService::fixAgeMs() const {
  return _lastFixAt == 0 ? UINT32_MAX : millis() - _lastFixAt;
}

uint32_t GpsService::satellites() {
  return _gps.satellites.isValid() ? _gps.satellites.value() : 0;
}

}  // namespace rvf
