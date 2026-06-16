# STR 2026 - Automated Environmental Control System

## Description

This project implements an IoT-based environmental control system using an ESP32-C6 microcontroller. The system allows remote monitoring and control of environmental variables through a web interface, integrating temperature measurement, ventilation control, automated curtains, RGB ambient lighting, Wi-Fi configuration, and Over-The-Air (OTA) firmware updates.

The project was developed as part of the **Real-Time Systems (STR)** course.

---

## Main Features

### Temperature Monitoring

The system continuously measures ambient temperature using an NTC sensor.

* Real-time temperature visualization.
* Temperature-based control algorithms.
* Alarm generation when critical thresholds are exceeded.

### Ventilation Control

A PWM-controlled fan operates in two modes:

#### Automatic Mode

The user configures:

* Desired Temperature
* Maximum Temperature

The fan speed is automatically adjusted according to:

* 0% speed when temperature ≤ desired temperature.
* 100% speed when temperature ≥ maximum temperature.
* Linear proportional control between both thresholds.

#### Manual Mode

The user directly selects the fan speed:

```text
0% - 100%
```

### Temperature Alarm

When the measured temperature exceeds the configured maximum temperature:

* A red LED blinks at 1 Hz.
* The alarm remains active until the temperature returns to a safe range.

---

### Curtain Control

A MG90S servo motor controls the curtain position.

#### Manual Mode

The user directly sets the opening percentage:

```text
0%   = Fully closed
100% = Fully open
```

#### Scheduled Mode

The system stores up to 8 schedule records.

Each record contains:

* Hour
* Minute
* Curtain opening percentage

The servo automatically moves the curtain when the scheduled time is reached.

---

### Ambient Lighting

An RGB LED provides user-controlled ambient lighting.

Configurable parameters:

* Red intensity
* Green intensity
* Blue intensity
* Brightness level

PWM control is used to generate different colors and light intensities.

---

### Wi-Fi Configuration

The device supports runtime network configuration through the web interface.

#### Station Mode (STA)

The user can configure:

* SSID
* Password

to connect the device to a local Wi-Fi network.

#### Access Point Mode (AP)

The user can modify:

* AP SSID
* AP Password

without recompiling the firmware.

---

### OTA Updates

Firmware updates can be uploaded directly through the web interface.

Benefits:

* No USB connection required.
* Faster deployment.
* Simplified maintenance.

---

## Hardware Components

| Component       | Description                     |
| --------------- | ------------------------------- |
| ESP32-C6 Nano   | Main controller                 |
| MG90S           | Servo motor for curtain control |
| BH1750 (GY-30)  | Ambient light sensor            |
| NTC Thermistor  | Temperature sensor              |
| DC Fan (130 mA) | Ventilation system              |
| RGB LED         | Ambient lighting                |
| Red LED         | Temperature alarm               |

---

## Software Architecture

```text
                 Web Interface
                        |
                        |
                  HTTP REST API
                        |
                        |
                +---------------+
                |  Web Server   |
                +---------------+
                        |
       -----------------------------------
       |          |          |          |
       |          |          |          |
       v          v          v          v

    Fan Task  Curtain    RGB Task  WiFi Task
              Task

                     |
                     |
               Sensor Task
```

---

## Project Structure

```text
main/
│
├── main.c
│
├── system_state.c
├── system_state.h
│
├── web_server.c
├── web_server.h
│
├── wifi_manager.c
├── wifi_manager.h
│
├── ota_manager.c
├── ota_manager.h
│
├── fan_control.c
├── fan_control.h
│
├── curtain_control.c
├── curtain_control.h
│
├── rgb_control.c
├── rgb_control.h
│
├── temperature_sensor.c
├── temperature_sensor.h
│
└── webpage/
    ├── index.html
    ├── style.css
    └── app.js
```

---

## REST API

### Fan Control

#### Manual Mode

```http
POST /api/fan/manual
```

Request:

```json
{
    "speed": 75
}
```

---

#### Automatic Mode

```http
POST /api/fan/auto
```

Request:

```json
{
    "desired_temp": 25,
    "max_temp": 35
}
```

---

### Temperature

```http
GET /api/temperature
```

Response:

```json
{
    "temperature": 28.4
}
```

---

### Curtain Control

#### Manual

```http
POST /api/curtain/manual
```

Request:

```json
{
    "position": 60
}
```

---

#### Schedule

```http
POST /api/curtain/schedule
```

Request:

```json
{
    "hour": 8,
    "minute": 30,
    "position": 100
}
```

---

### RGB Control

```http
POST /api/rgb
```

Request:

```json
{
    "r": 255,
    "g": 128,
    "b": 64,
    "brightness": 80
}
```

---

### Wi-Fi Configuration

#### Station Mode

```http
POST /api/wifi/sta
```

Request:

```json
{
    "ssid": "MyNetwork",
    "password": "mypassword"
}
```

---

#### Access Point Mode

```http
POST /api/wifi/ap
```

Request:

```json
{
    "ssid": "ESP32_Control",
    "password": "12345678"
}
```

---

### OTA Update

```http
POST /api/ota
```

Firmware file:

```text
firmware.bin
```

---

### System Status

```http
GET /api/status
```

Response:

```json
{
    "temperature": 28.5,
    "fan_mode": "AUTO",
    "fan_speed": 65,
    "curtain_position": 40,
    "alarm": false
}
```

---

## Development Environment

* ESP-IDF
* FreeRTOS
* ESP HTTP Server
* ESP Wi-Fi
* ESP OTA
* HTML
* CSS
* JavaScript

---

## Authors

Real-Time Systems Project

Electronic Engineering
