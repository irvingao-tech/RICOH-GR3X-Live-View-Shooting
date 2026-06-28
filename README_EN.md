# RICOH GR StickS3 Remote Viewfinder

Firmware for running RICOH GR wireless live-view and remote shutter on the M5Stack StickS3.

It identifies and connects to the camera using **BLE as the only online entry point**, temporarily enables the camera's Wi-Fi over BLE, reads the dynamic Wi-Fi credentials returned by the camera, and finally displays the live preview via HTTP LiveView.

[中文 README](README.md)

---

## Current Stable Features

- **BLE-First Connection Flow**: Scans for `GR_` / RICOH devices upon powering up, prioritizing the camera's BLE address and name stored in NVS.
- **Dynamic Wi-Fi Credentials**: No longer depends on a fixed SSID/passphrase in `platformio.ini`. After the camera's Wi-Fi is woken up, the SSID, password, and BSSID are read via BLE.
- **LiveView Real-Time Preview**: Connects to the camera's Wi-Fi, accesses the RICOH HTTP API, opens `/v1/liveview`, and displays the MJPEG video stream on the StickS3 screen.
- **G11 External Shutter**: Triggers the BLE shutter sequence immediately when the GPIO11 button is pressed to GND.
- **Camera Power-Off Guard**: Enters a guard state when a proactive BLE disconnection from the camera is detected, preventing the StickS3 from waking the camera again during its shutdown process.
- **Manual Wake & Reconnection**: After the cooldown period of the guard state ends, pressing G11 or Button B will re-wake and reconnect to the camera.
- **Autoconnect on StickS3 Reboot**: The guard state is only kept in RAM. If the StickS3 itself reboots, it will start over from the BLE scan and autoconnect to the camera.

---

## Workflow

```text
StickS3 Power On / Reboot
  ↓
Initialize screen, buttons, NVS, BLE, Wi-Fi
  ↓
Read Camera Profile (BLE address, camera name, camera IP)
  ↓
Scan for GR / RICOH BLE devices
  ↓
Connect to camera BLE and complete secure pairing / encryption
  ↓
Write WLAN ON command over BLE (handle 0x0135, value 0x01)
  ↓
Read Wi-Fi parameters returned from camera over BLE
  ↓
Connect to camera Wi-Fi AP
  ↓
HTTP Probe camera status
  ↓
Open LiveView, read MJPEG stream, decode and display
```

### Camera Power-Off Guard Flow

```text
LiveView Running
  ↓
Camera powers off / Proactive BLE disconnection
  ↓
Receive BLE disconnect reason 531 / 533
  ↓
Enter CAMERA_SLEEP_GUARD
  ↓
BLE scanning, BLE reconnection, and Wi-Fi activation are forbidden during 15-second cooldown
  ↓
Do not automatically wake the camera after cooldown ends
  ↓
User presses G11 or Button B
  ↓
Rebuild NimBLE stack, scan again, and reconnect to the camera
```

This prevents the StickS3 from sending a Wi-Fi wake command while the camera is shutting down or advertising in low-power standby.

---

## Controls

| Control | Function |
| --- | --- |
| G11 External Button | Triggers BLE shutter during LiveView; acts as manual wake / reconnect button in camera guard state |
| Button B | Pauses / resumes LiveView; acts as manual wake / reconnect button in camera guard state |
| Button A | Reserved |

G11 Hardware Wiring: Connect a button between **GPIO11 (G11)** and **GND**. The firmware uses `INPUT_PULLUP` and a falling-edge interrupt.

---

## RICOH BLE GATT Handles

| Feature | Handle | Operation | Description |
| --- | ---: | --- | --- |
| Wi-Fi Enable | `0x0135` | Write | Write `0x01` to wake camera Wi-Fi |
| Wi-Fi SSID | `0x0138` | Read | Read camera AP SSID |
| Wi-Fi Passphrase | `0x013A` | Read | Read camera AP password |
| Wi-Fi BSSID | `0x0140` | Read | Read camera AP MAC address |
| Shutter Control | `0x0099` | Write | `0x01` Focus, `0x02` Shoot, `0x00` Release |

---

## Hardware Requirements

- M5Stack StickS3
- RICOH GR III / GR IIIx / GR IV or any camera model compatible with the RICOH BLE control protocol
- Optional: External G11 shutter button

---

## Build and Flash

```bash
# Compile
platformio run

# Upload
platformio run -t upload

# Monitor serial logs
platformio device monitor
```

Serial baud rate: `115200`

---

## Key Configuration

Main configuration files:

- `src/config.h`
- `platformio.ini`

Since the current firmware fetches Wi-Fi credentials via BLE, there is no need to configure fixed `GR_WIFI_SSID` / `GR_WIFI_PASSWORD` flags in `platformio.ini`.

Important Parameters:

| Parameter | Default Value | Description |
| --- | ---: | --- |
| `BLE_SCAN_SECONDS` | `2` | Duration of a single BLE scanning round |
| `BLE_CONNECT_ATTEMPTS` | `12` | Number of BLE reconnection attempts when a paired camera profile is stored |
| `FIRST_BOOT_BLE_PAIRING_ATTEMPTS` | `12` | Number of pairing scan rounds on first boot / when NVS profile is empty |
| `CAMERA_POWER_OFF_COOLDOWN_MS` | `15000` | Cooldown period after the camera is powered off / disconnected |
| `BLE_MANUAL_WAKE_REINIT_SETTLE_MS` | `3000` | Settling time after rebuilding the BLE stack during manual wake |
| `LIVEVIEW_STALL_TIMEOUT_MS` | `5000` | Stall detection threshold for LiveView to attempt recovery when no frames are received |

---

## Typical Logs

Normal connection:

```text
BLE: connected secure
Flow: BLE_SCAN -> BLE_READY (BLE connected)
BLE: Wi-Fi open requested
BLE: Wi-Fi parameters received ssid='GR_H264456' bssid=''
WiFi: connected ip=192.168.0.4
HTTP: camera ready model='RICOH GR IV HDF'
LiveView: connected
```

Camera power-off guard:

```text
BLE: disconnected reason=531
Flow: BLE_READY -> CAMERA_SLEEP_GUARD (...)
BLE guard: remote disconnect reason=531; auto wake paused for 15s, then manual wake required
```

Manual wake:

```text
BLE guard: manual wake requested (G11 manual wake), previous disconnect reason=531
BLE guard: manual wake BLE stack rebuild (G11 manual wake)
BLE: resetting stack (clear objects)
BLE: scanning for GR camera
```

---

## Troubleshooting

### Camera should not be woken up after powering off

This is the default behavior of the firmware. When a `531 / 533` disconnect reason is received, the system enters `CAMERA_SLEEP_GUARD` and will not automatically send a Wi-Fi ON command. Once the cooldown ends, you must press the G11 or Button B to wake the camera manually.

### Will the StickS3 reconnect automatically after rebooting?

Yes. The guard state is not written to NVS, so rebooting the StickS3 will start fresh from the BLE scanning and connection process.

### The G11 shutter logs show "BLE shutter failed" but the camera still takes a photo

RICOH cameras may disconnect or reject the subsequent release command at the exact moment of capturing, causing the logs to show a write failure. As long as the camera successfully takes the picture and the LiveView recovers, this does not affect normal usage.

---

## Project Structure

```text
src/
  main.cpp                 Main state machine, connection flow, guard state, button logic
  ricoh_ble_client.*       RICOH BLE scanning, connection, Wi-Fi parameters reading, shutter writing
  gr_wifi.*                ESP32 Wi-Fi STA connection
  gr_api.*                 RICOH HTTP API & LiveView
  mjpeg_stream.*           MJPEG stream parser
  jpeg_decoder.*           JPEG decoding and display rendering
  display.*                StickS3 screen UI
  camera_profile_store.*   NVS camera profile storage
  g11_button.*             G11 external button interrupt
```

## License

This project is licensed under the GNU General Public License v3.0.

You may use, modify, and distribute this project under the terms of the GPL-3.0 license.  
If you distribute modified versions or derivative works, you must also release the corresponding source code under the same license.

See the [LICENSE](LICENSE) file for details.
