# NTC RGB Controller with ESP-IDF

## Overview

This project implements a multitask embedded system using **ESP32-C6**, **ESP-IDF**, and **FreeRTOS**.

The system combines two independent RGB control subsystems running concurrently:

1. **Automatic RGB temperature control**

   * Uses an NTC thermistor.
   * Changes RGB color according to temperature ranges.
   * Configurable in real time through UART commands.

2. **Manual RGB control**

   * Uses a potentiometer and push buttons.
   * Allows selecting RGB channels individually.
   * Adjusts PWM intensity dynamically.

The architecture was designed using:

* FreeRTOS tasks
* Queues
* Modular programming
* Hardware abstraction

---

# Features

* ESP-IDF framework
* FreeRTOS multitasking
* UART command interface
* ADC oneshot driver
* ADC calibration support
* PWM LED control using LEDC
* Dynamic RGB configuration
* Temperature monitoring using NTC
* Real-time RGB intensity control
* Queue-based inter-task communication

---

# Hardware Requirements

## Main Components

* ESP32-C6
* NTC Thermistor (4.7kΩ)
* Fixed resistor (4.7kΩ)
* RGB LED (automatic mode)
* RGB LED (manual mode)
* Potentiometer
* Push buttons
* USB-UART connection

---

# System Architecture

## FreeRTOS Tasks

| Task                 | Function                   |
| -------------------- | -------------------------- |
| `sensor_task`        | Reads NTC temperature      |
| `rgb_task`           | Controls automatic RGB LED |
| `uart_task`          | Receives UART commands     |
| `command_task`       | Processes UART commands    |
| `potentiometer_task` | Controls manual RGB LED    |

---

# Inter-Task Communication

The system uses FreeRTOS queues for synchronization and communication.

| Queue        | Purpose                  |
| ------------ | ------------------------ |
| `temp_queue` | Sends temperature values |
| `cmd_queue`  | Sends UART commands      |

---

# Temperature Control Logic

The NTC subsystem:

1. Reads ADC voltage.
2. Calculates thermistor resistance.
3. Computes temperature using the Beta equation.
4. Activates RGB channels depending on configured ranges.

## Example

| Temperature Range | Color |
| ----------------- | ----- |
| 10°C – 25°C       | Blue  |
| 25°C – 35°C       | Green |
| 35°C – 80°C       | Red   |

---

# UART Commands

The system supports dynamic runtime configuration through UART.

## Example Commands

```text
ROJO_MIN_18
ROJO_MAX_30
VERDE_MIN_20
VERDE_MAX_35
AZUL_MIN_5
AZUL_MAX_15
PWM_50
```

## Description

| Command       | Function                      |
| ------------- | ----------------------------- |
| `ROJO_MIN_X`  | Sets minimum red threshold    |
| `ROJO_MAX_X`  | Sets maximum red threshold    |
| `VERDE_MIN_X` | Sets minimum green threshold  |
| `VERDE_MAX_X` | Sets maximum green threshold  |
| `AZUL_MIN_X`  | Sets minimum blue threshold   |
| `AZUL_MAX_X`  | Sets maximum blue threshold   |
| `PWM_X`       | Sets RGB intensity percentage |

---

# Manual RGB Control

The manual subsystem uses:

* One potentiometer
* Four buttons

## Buttons

| Button   | Function             |
| -------- | -------------------- |
| `BTN_R`  | Select red channel   |
| `BTN_G`  | Select green channel |
| `BTN_B`  | Select blue channel  |
| `BTN_OK` | Show final RGB mix   |

## Operation

1. Select a color channel.
2. Rotate the potentiometer.
3. PWM intensity is stored for the selected channel.
4. Press `BTN_OK` to display the final RGB mixture.

---

# Project Structure

```text
project/
│
├── main/
│   ├── main.c
│   ├── libraries.c
│   └── libraries.h
│
├── CMakeLists.txt
├── sdkconfig
└── README.md
```

---

# File Description

## `main.c`

Responsible for:

* system startup
* task creation
* global configuration

## `libraries.c`

Contains:

* ADC drivers
* PWM configuration
* UART communication
* RGB control logic
* FreeRTOS tasks
* helper functions

## `libraries.h`

Contains:

* macros
* structs
* enums
* function prototypes

---

# ADC Configuration

The project uses:

* ADC OneShot driver
* ADC calibration (curve fitting)

The ADC is shared between:

* NTC sensor
* potentiometer

using different ADC channels.

---

# PWM Configuration

PWM is generated using:

* LEDC peripheral
* Low Speed Mode
* 5 kHz frequency



