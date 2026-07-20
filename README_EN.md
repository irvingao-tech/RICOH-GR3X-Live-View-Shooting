# RICOH GR IIIx Live View Shooting

[简体中文](README.md) | **English**

![RICOH GR IIIx Live View Shooting promotional image](assets/ricoh-gr3x-live-view-shooting.png)

### Promotional Video

[![Watch the RICOH GR IIIx Live View Shooting promotional video](https://img.youtube.com/vi/Fc9UBgckpoI/hqdefault.jpg)](https://www.youtube.com/watch?v=Fc9UBgckpoI)

Click the video thumbnail above to watch it on YouTube.

A Bluetooth remote controller and live-view monitor for the **RICOH GR IIIx**, running on the **M5Stack StickS3**. The StickS3 pairs securely with the camera over BLE, reads its Wi-Fi credentials, connects to the camera's access point, and displays Live View on its screen. While Live View is active, briefly press the blue button to autofocus and take a photo.

> Current hardware status: **Initial pairing, automatic reconnection, Wi-Fi Live View, and the BLE shutter have all been verified on a GR IIIx.** Support for reading data from the GPS/BDS Unit v1.1 and sending it to the camera has been added. StickS3 Grove power and UART pins are configured for G9/G10, but writing GPS data into photo EXIF still requires hardware verification.

## Features

- RICOH GR IIIx BLE scanning, secure six-digit passkey pairing, and saved bonding information
- Automatic reconnection to a previously paired camera
- Dynamic GR IIIx Wi-Fi credential retrieval over BLE
- Camera MJPEG Live View displayed on the StickS3 screen
- Balanced preview pipeline: an 8 KB network read buffer, full-frame assembly in internal RAM, and a single LCD submission to reduce visible block-by-block refresh
- Live View HUD for shutter speed, aperture, ISO, and exposure compensation, plus AF/SHOT command feedback during capture
- Autofocus and capture using the blue button
- Short-press Button B to switch between Live View and local-camera operation; hold it for three seconds to clear BLE pairing
- Six-digit pairing passkey entry directly on the StickS3 screen
- Optional Windows utility for faster passkey entry
- Experimental GPS/BDS Unit v1.1 location transfer to the GR IIIx; EXIF writing is not yet fully verified

## Verified Hardware

- RICOH GR IIIx
- M5Stack StickS3 (ESP32-S3)
- USB data cable for flashing, power, and the optional computer-based passkey utility
- Optional: M5Stack GPS/BDS Unit v1.1 (AT6668)

This project targets the **GR IIIx**. The GR III, GR IV, and other models have not been verified on this branch. Do not assume that their protocols are identical.

## Development Environment

The recommended setup on Windows is [Visual Studio Code](https://code.visualstudio.com/) with the PlatformIO extension. The commands below must be run in a terminal where PlatformIO Core CLI is available.

The first build downloads the ESP32 platform and required libraries, so the relevant package sources must be reachable.

## Build and Flash

1. Connect the StickS3 using a USB cable that supports data transfer.
2. Open PowerShell and change to the project directory.
3. Build the firmware:

```powershell
pio run -e m5stack-sticks3
```

4. Find the StickS3 serial port and flash the firmware. This example uses `COM4`:

```powershell
pio run -e m5stack-sticks3 --target upload --upload-port COM4
```

If you see `Write timeout` or the upload remains at `Connecting...`:

1. Close any serial monitor or passkey utility using that port.
2. Keep the USB cable connected.
3. Hold the StickS3 side reset/power button and release it after the green indicator flashes to enter download mode.
4. Run the upload command again.

The upload succeeded when the output contains `[SUCCESS]`, `Hash of data verified`, and `Hard resetting via RTS pin`.

## First-time Bluetooth Pairing

### On the Camera

1. Turn on the GR IIIx.
2. Make sure Bluetooth is enabled.
3. If an earlier pairing attempt failed, remove the StickS3 entry from the camera's paired-device list.
4. Choose the camera option to add a new device or begin pairing.
5. Start or reset the StickS3 and wait for `BLE SEARCHING` / `BLE CONNECTING` to appear.

The camera will display a six-digit passkey. This code is not sent by SMS or email; it must be entered on the StickS3.

### Method A: Enter the Passkey on the StickS3

After the passkey screen appears, no Windows computer is required:

- Short-press Button A (the blue front button): increment the current digit, cycling from 0 to 9
- Short-press Button B (the secondary function button): confirm the current digit and move to the next position
- After all six digits are confirmed: the passkey is submitted automatically

### Method B: Use the Windows Passkey Utility

The first-time pairing window is short, so the computer utility is usually faster. Install Python 3 and `pyserial`, then run:

```powershell
py -m pip install pyserial
py tools\pin_entry_gui.py COM4
```

Replace `COM4` with the actual StickS3 serial port. Once the utility is connected, enter the six digits on the computer keyboard as soon as the camera displays them. The utility submits the passkey automatically after the sixth digit.

> The passkey utility and PlatformIO serial monitor cannot use the same COM port simultaneously. Close the utility before flashing firmware.

After pairing succeeds, a new entry appears in the camera's paired-device list. The StickS3 continues by connecting to the camera's Wi-Fi and enters Live View automatically. Normally, the passkey does not need to be entered again.

## Daily Use

1. Turn on the GR IIIx and leave it in shooting mode.
2. Turn on the StickS3.
3. Wait for BLE and Wi-Fi to connect automatically.
4. When Live View appears on the StickS3, **briefly press the blue front button** to autofocus and take a photo.
5. Photos are saved to the camera's SD card, not to the StickS3.

During normal use, USB is needed only for power or charging and **does not connect to the camera**. The StickS3 communicates with the camera wirelessly over BLE and Wi-Fi.

### Live View and Local-camera Modes

When the GR IIIx enters remote Wi-Fi Live View, its own screen may turn off and parameter controls may move to the remote device. The firmware provides two modes:

- **Short-press Button B** to stop StickS3 Live View and turn off camera Wi-Fi while retaining the BLE shutter. The camera screen, buttons, and dials become available for local operation.
- **Short-press Button B again** to restart camera Wi-Fi and StickS3 Live View.
- In either mode, short-press Button A to autofocus and take a photo.

The ESP32-S3 has no dedicated JPEG hardware decoder. This project uses the optimized JPEGDEC software decoder and writes decoded blocks directly to the LCD. Actual frame rate still depends on the GR IIIx output rate, Wi-Fi/BLE coexistence, JPEG size, and signal strength. Use the screen and serial statistics as the source of truth.

## Clear Existing Pairing

If the device remains at `BLE CONNECTING` even though the camera reports that it is paired, the bonding keys stored on each side are usually inconsistent:

1. Remove the StickS3 entry from the GR IIIx paired-device list.
2. Hold Button B on the StickS3 for about three seconds to clear its local BLE bonding cache.
3. Repeat the first-time pairing procedure.

## GPS/BDS Unit v1.1 (Experimental)

Connect the GPS module to the StickS3 bottom HY2.0-4P port with a Grove cable. This is a custom Grove port: the firmware uses `G9` as GPS RX and `G10` as GPS TX and enables `EXT_5V` at startup to power the module. The GPS/BDS Unit v1.1 defaults to 115200 bps. Once a valid fix is available, the firmware attempts to send the position to the camera every ten seconds.

The following items have not yet been confirmed:

- Stable satellite positioning
- Successful GPS metadata writing to photo EXIF

The GPS module is optional. Leaving it disconnected does not affect BLE, Live View, or shooting.

## Troubleshooting

### The Camera Lists the Device, but the StickS3 Remains at `BLE CONNECTING`

Delete the old bonding information from both the camera and StickS3, then pair them again. Clearing only one side is usually insufficient.

### The Camera Quickly Reports a Pairing Failure After Showing the Passkey

Start the Windows passkey utility first and confirm that it is connected to the COM port. Then begin camera pairing and enter the six digits immediately when they appear.

### The Passkey Utility Cannot Connect to the COM Port

- Confirm the actual port number in Windows Device Manager.
- Close PlatformIO serial monitor and any other application using the serial port.
- Restart the utility with the correct port, for example `py tools\pin_entry_gui.py COM5`.

### The Green Indicator Keeps Flashing

Flashing during pairing or connection normally means the firmware is still working; it does not by itself indicate a failed upload. Check the screen state and serial log for the actual status.

### Live View Works, but the Camera Does Not Take a Photo

Make sure the camera is still in shooting mode, and briefly press rather than hold the blue button. Check the camera's SD card for the captured photo.

## Serial Diagnostics

When troubleshooting, close the passkey utility and start a serial monitor:

```powershell
pio device monitor --baud 115200 --port COM4
```

When opening an issue, include a complete log beginning at StickS3 startup, but redact private information such as the Wi-Fi password.

## Project Structure

```text
src/                         StickS3 firmware source code
src/ricoh_ble_client.*       GR IIIx BLE, pairing, Wi-Fi credentials, shutter, and GPS protocol
src/services/GpsService.*    GPS/BDS Unit v1.1 reader service (experimental)
tools/pin_entry_gui.py       Windows first-time pairing passkey utility
docs/gr3x_quickstart.md      Condensed test procedure
platformio.ini               PlatformIO build configuration
```

## Origin and Acknowledgments

This project is based on [sky18Dragon/RICOH-GR-Live-View-Shooting](https://github.com/sky18Dragon/RICOH-GR-Live-View-Shooting). Thanks to the original author for the core StickS3 live-view architecture and GR camera-control implementation.

This branch adds and adjusts GR IIIx-specific UUID characteristic access, secure pairing, six-digit passkey entry, connection-resource cleanup, Wi-Fi credential retrieval, and experimental GPS support.

## License

This project retains the original [GNU General Public License v3.0](LICENSE). Modified or derivative versions must comply with GPL-3.0 and preserve the original project's copyright and license notices.
