# 🎵 ESP32 High-Fidelity FLAC Player

[![PlatformIO](https://img.shields.io/badge/PlatformIO-Compatible-orange?logo=platformio)](https://platformio.org/)
[![Framework](https://img.shields.io/badge/Framework-Arduino-blue?logo=arduino)](https://www.arduino.cc/)
[![Hardware](https://img.shields.io/badge/Hardware-ESP32-lightgrey?logo=espressif)](https://www.espressif.com/en/products/socs/esp32)

A powerful, button-controlled FLAC audio player built on the ESP32 platform. This project delivers high-quality audio playback from an SD card using an external I2S DAC for the best possible sound experience.

---

## ✨ Features

- **High-Quality Audio:** Native decoding of lossless FLAC files.
- **I2S Support:** Optimized for external DACs like the **PCM5102A**.
- **Playlist Management:** Simple `.txt` based playlists.
- **Smart Controls:**
  - ⏯️ Play / Pause
  - 🔊 Volume Up / Down
  - ⏭️ Next Track
  - 🔀 Shuffle Mode
  - 📁 Next Playlist (with optional voice announcements)
- **Robust Performance:** Multi-buffered audio output to prevent stuttering.

---

## 🛠️ Hardware Requirements

| Component | Description |
| :--- | :--- |
| **Microcontroller** | ESP32 (e.g., ESP32 DevKit V1) |
| **DAC** | I2S Digital-to-Analog Converter (e.g., **PCM5102A**) |
| **Storage** | Micro SD Card Reader (SPI) |
| **Input** | 6x Momentary Push Buttons |
| **Power** | 3.3V or 5V (depending on your modules) |

### 📍 Pinout Configuration

| Peripheral | ESP32 Pin | Connection |
| :--- | :--- | :--- |
| **SD Card** | GPIO 5 | CS |
| | GPIO 23 | MOSI |
| | GPIO 19 | MISO |
| | GPIO 18 | SCK |
| **I2S DAC** | GPIO 25 | BCLK |
| | GPIO 26 | LRC / WS |
| | GPIO 22 | DOUT |
| **Buttons** | GPIO 32 | Play / Pause |
| | GPIO 33 | Volume Up |
| | GPIO 13 | Volume Down |
| | GPIO 12 | Next Track |
| | GPIO 14 | Shuffle Toggle |
| | GPIO 27 | Next Playlist |

*Note: All buttons should be connected between the GPIO pin and **GND**. The internal pull-up resistors are enabled in code.*

---

## 📂 SD Card Structure

Prepare your SD card (formatted as FAT32) with the following structure:

1. Place all your **.flac** files in the root directory.
2. Create **.txt** files for your playlists (e.g., `Rock.txt`, `Jazz.txt`).
3. In each `.txt` file, list the exact filenames of the songs, one per line:
   ```text
   Song1.flac
   AnotherSong.flac
   ```
4. *(Optional)* Add voice announcements for your playlists by naming a FLAC file `PlaylistName_name.flac` (e.g., `Rock_name.flac`). These will play when you switch playlists.

---

## 🚀 Getting Started

### 1. Prerequisites
- [Visual Studio Code](https://code.visualstudio.com/) with the [PlatformIO](https://platformio.org/install/ide?install=vscode) extension installed.

### 2. Installation
1. Clone this repository or download the source code.
2. Open the project folder in VS Code / PlatformIO.
3. The required library `earlephilhower/ESP8266Audio` will be automatically installed by PlatformIO.

### 3. Uploading
- Connect your ESP32 via USB.
- Click the **PlatformIO: Build** icon to compile.
- Click the **PlatformIO: Upload** icon to flash your device.
- Open the **Serial Monitor** (115200 baud) to see the player initialization and debug logs.

---

## 📜 Libraries Used
- [ESP8266Audio](https://github.com/earlephilhower/ESP8266Audio) - Powerful audio decoding library for ESP8266 and ESP32.

---

## 🤝 Contributing
Feel free to fork this project, submit issues, or create pull requests to improve the code or add new features!

## 📄 License
This project is open-source. Check the source files for specific license details.

---
*Developed with ❤️ for high-quality audio.*
