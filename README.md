# ESP32 Automated Satellite Tracker Ground Station

An automated, dual-axis (Azimuth/Elevation) satellite tracking ground station driven by an ESP32 microcontroller. The system supports real-time onboard SGP4 orbital propagation as well as remote network instrumentation via the EasyComm II protocol over TCP.

This repository contains two distinct hardware implementation variations maintained across independent development branches.

---

## Repository Architecture

Select the appropriate branch depending on your physical display hardware configuration:

### 1. Main Branch (`main`)
* **Display Configuration**: 0.96-inch or 1.3-inch I2C OLED driven by SSD1306/SH1106 controllers.
* **Characteristics**: Core tracking engine featuring typographic telemetry display profiles. Optimized for minimal processing overhead and straightforward visual validation.

### 2. TFT Update Branch (`tft-st7789-update`)
* **Display Configuration**: 240x320 SPI TFT Color Display driven by the ST7789 chip.
* **Characteristics**: High-density Tactical HUD layout profile. Features a real-time swept radar canvas, dynamic axis tracking error delta computations (`dAZ`/`dEL`), a captive portal network provisioning gateway, and an autonomous geostationary orbit validator to prevent loop execution constraints.

To clone a specific implementation branch from the terminal interface:

```bash
# Clone the default OLED branch
git clone -b main [https://github.com/uzzambutt/ESP32-Sattelite-Tracker.git](https://github.com/uzzambutt/ESP32-Sattelite-Tracker.git)

# Clone the high-density TFT branch
git clone -b tft-st7789-update [https://github.com/uzzambutt/ESP32-Sattelite-Tracker.git](https://github.com/uzzambutt/ESP32-Sattelite-Tracker.git)
```

---

## Hardware Requirements

### Common Core Components

* **Microcontroller**: ESP32 DevKitC v4 (WROOM-32D / WROOM-32U architecture).
* **Breakout Interconnect**: Arduino CNC Shield V3.00.
* **Motor Drivers**: 2 x A4988 or DRV8825 constant-current chopper modules configured to 1/16 microstepping. All three hardware jumpers must be installed beneath the driver sockets.
* **Actuators**: 2 x NEMA 17 hybrid bipolar stepper motors with a 1.8 degree step angle and a 1.3A current phase rating.
* **Power Assembly**: Dedicated 24V Switch-Mode Power Supply (SMPS) minimum 3A/4A connected to the CNC shield power terminals. Independent 5V supply connected directly to the ESP32 USB port for complete logic noise isolation.

### Branch-Specific Hardware

* **For `main` branch**: I2C OLED Display Module.
* **For `tft-st7789-update` branch**: ST7789 240x320 SPI TFT Display Panel powered at 3.3V.

---

## Hardware Pin Configurations

The GPIO mapping matches an ESP32 routed through an Arduino CNC Shield V3 breakout structure:

| Signal / Pin Name | ESP32 GPIO Pin | CNC Shield Route |
| --- | --- | --- |
| Azimuth Step Pin (`AZ_STEP`) | GPIO 32 | X.STEP |
| Azimuth Direction Pin (`AZ_DIR`) | GPIO 14 | X.DIR |
| Elevation Step Pin (`EL_STEP`) | GPIO 27 | Y.STEP |
| Elevation Direction Pin (`EL_DIR`) | GPIO 26 | Y.DIR |
| Driver Enable Pin (`ENABLE_PIN`) | GPIO 25 | EN (Common Enable Rail) |

### Display Interconnects

#### OLED Interface (`main` branch)

* **VCC** -> 3.3V / 5V
* **GND** -> GND
* **SCL** -> GPIO 22
* **SDA** -> GPIO 21

#### ST7789 TFT Interface (`tft-st7789-update` branch)

* **VCC** -> 3.3V
* **GND** -> GND
* **SCL (CLK)** -> GPIO 18
* **SDA (MOSI)** -> GPIO 23
* **RES (RST)** -> GPIO 4
* **DC (RS)** -> GPIO 2
* **CS** -> GPIO 15

---

## Software Installation and Dependencies

### 1. Arduino IDE Board Manager Configuration

Add the official Espressif ESP32 core repository string to your Additional Boards Manager URLs under **File -> Preferences**:

```text
[https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json](https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json)
```

Install the **esp32** platform library via the Boards Manager. Validated profiles utilize Espressif core framework package version 2.0.x.

### 2. Mandatory Library Dependencies

Ensure the following exact libraries are fetched and compiled into your local compiler toolchain:

* `AccelStepper` by Mike McCauley (Handles high-frequency step pulse generation and velocity profiles).
* `Adafruit_GFX` & `Adafruit_ST7789` (Required for display processing under the TFT branch).
* `AsyncTCP` & `ESPAsyncWebServer` (Asynchronous HTTP processing engine for the telemetry endpoints).
* `ArduinoJson` by Benoit Blanchon (Telemetry payload serialization/deserialization).
* `NTPClient` by Fabian Schurig (Reference atomic timestamp synchronization).
* `Sgp4` by Hoang Nguyen (SGP4/SDP4 Keplerian orbital mechanics propagation library).

---

## Configuration Protocol

Before uploading the source sketch, you must populate a configuration header file named `secrets.h` inside the root build directory to handle network interface linkage:

```cpp
#ifndef SECRETS_H
#define SECRETS_H

const char* ssid     = "YOUR_STA_ROUTER_SSID";
const char* password = "YOUR_STA_ROUTER_PASSWORD";

#endif
```

### Onboard Non-Volatile Memory Parsing (`Preferences`)

The tracker relies on internal EEPROM emulation via the ESP32 `Preferences` framework to maintain baseline configuration fields across hardware resets. On initial boot, the following parameters default into operational parameters if unpopulated:

* **Maidenhead Grid Locator**: Defaults to `MM71dl` (Lahore baseline coordinates: Lat 31.52° N, Lon 74.35° E). Can be reconfigured at runtime over the web instrumentation terminal to match your true ground station coordinates.

---

## Operational Mechanics

The ground station changes behavior seamlessly based on connection state and tracking context:

### 1. Onboard SGP4 Autonomous Engine

When tracking mode is configured to **Onboard Internal SGP4**, the ESP32 parses raw Two-Line Element (TLE) datasets uploaded directly into the onboard file system (`LittleFS`). It synchronizes time via local NTP servers and uses the Sgp4 execution vectors to update stepper coordinate targets unconditionally every second.

* **Geostationary (GEO) Loop Protection**: The system automatically executes a mean motion and distance vector trace during TLE loading. Satellites identified as stationary/geosynchronous break the loop prediction metrics instantly, bypass infinite T-minus clock computations, lock layout strings to `GEO` or `STATIC`, and clamp look vectors directly to static geodetic targets matching your exact horizon look angle.

### 2. Network Remote Control (Look4Sat Integration)

When set to **External Mode**, the ESP32 spins up a background shadow TCP listening socket on port `4533`. This interfaces directly with telemetry routing tools like Look4Sat (running on Android/PC) or generic Hamlib clients using EasyComm II format commands over the network.

* Command parsing captures raw string arguments like `AZ[position] EL[position]` and translates them into mechanical pulse sequences inside Core 1 runtime execution blocks.

### 3. Captive Portal Network Provisioning Gateway

If the station fails to hook to the wireless network designated in `secrets.h` within a strict 15-second tracking watchdog timeout window, it throws a hardware state fallback interrupt:

* Standard motor, radar, and physics tasks suspend instantly.
* The system shifts into Access Point mode, broadcasting the network SSID `AEROSPACE-TRACKER` (Password: `groundstation`).
* The local HTTP endpoint drops all tracking interfaces and hosts a dedicated Captive Portal interface at `http://192.168.4.1` where you can input new wireless credentials. Saving variables through this interface permanently modifies the internal non-volatile memory registers and forces an internal hardware restart cycle.

### 4. Dynamic Current Isolation Engine

To mitigate thermal build-up inside NEMA 17 stators during protracted idle windows, the stepper driver enable rail is actively managed. The microcontroller keeps drivers active exclusively when the motors are executing active speed changes or when target satellite elevation calculations rise above the horizon tracking envelope. The second target coordinates set and step queues finish deceleration back to null velocity, the `ENABLE_PIN` toggles HIGH, putting the driver ICs into a completely unenergized standby state.

---

## Technical Maintenance Notes

* **Core Coupling Optimization**: Core 0 handles the memory footprint of background networking loops, WebSocket pushes, async client connections, and hardware watchdogs. Core 1 runs uninhibited, dedicated solely to generating smooth stepper motor pulse trains, preventing telemetry or network load spikes from stuttering axis positioning.
* **Watchdog Timers**: Hardware watchdogs are configured to execute a hard reset on the chip within 30 seconds if either execution task hangs inside a blocking instruction sequence.

---

## License

This project is licensed under the MIT License - see below for details:

```text
MIT License

Copyright (c) 2026 Muhammad Uzzam Butt

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
```
