# IoT Light Bulb Automation Project

A smart lighting solution designed to enhance convenience, energy efficiency, and security for residential and commercial spaces. This project integrates IoT technology for remote control, scheduling, and monitoring, providing a user-friendly interface for managing lighting systems.

## Table of Contents
- [System Overview](#system-overview)
- [Features](#features)
- [System Requirements](#system-requirements)
- [Installation](#installation)
  - [Hardware Setup](#hardware-setup)
  - [Software Setup](#software-setup)
- [Configuration](#configuration)
- [Testing](#testing)
- [Operation](#operation)
- [Schematic Diagram](#schematic-diagram)
- [Website Design](#website-design)

## System Overview

This project leverages an ESP32 microcontroller to automate light bulbs via a web interface. The system includes features such as:
- Remote control of light bulbs through Wi-Fi.
- Real-time current monitoring to track power consumption.
- User-configurable scheduling for automated operations.
- LED indicators for system status and diagnostics.

## Features

- **Smart Control**: Control light bulbs remotely using a mobile or desktop web interface.
- **Energy Monitoring**: Monitor power consumption in real-time.
- **Scheduling**: Automate lighting schedules for better energy efficiency.
- **LED Indicators**: Visual feedback for system states.
- **Secure Access**: Password-protected web interface.

## System Requirements

### Components
- **ESP32 WROOM-32**: Microcontroller for system control and Wi-Fi connectivity.
- **Current Sensor (ACS712)**: Monitors the current flowing through the light bulbs.
- **Relay Module**: Controls the on/off state of the light bulbs.
- **LED Indicators**: Displays system state.
- **XPJ-01D Power Module**: Supplies power to the components.
- **Light Bulbs and Outlet Cords**: For lighting control.
- **Enclosure Case**: Protects components.

## Installation

### Hardware Setup
1. Gather all required components.
2. Follow the schematic diagram to connect components, including:
   - ESP32 GPIO pins to sensors, LEDs, and relays.
   - ACS712 for current monitoring.
   - XPJ-01D for power distribution.
3. Secure the components in the enclosure case.

### Software Setup
1. Install the Arduino IDE and the ESP32 board package.
2. Install required libraries: `ArduinoJson`, `WiFi`, `ESPAsyncWebServer`, and `ACS712`.
3. Upload the provided code to the ESP32 using the Arduino IDE.

## Configuration

1. Edit Wi-Fi credentials and system settings in the code.
2. Upload the updated code to the ESP32.
3. Power on the system to initialize the components.

## Testing

1. Verify the LED indicators reflect the correct system state.
2. Test the web interface by accessing it through the ESP32's IP address or hostname.
3. Validate bulb control, scheduling, and real-time monitoring functionality.

## Operation

1. Power on the system.
2. Connect to the ESP32's Wi-Fi network.
3. Access the web interface to control bulbs, schedule operations, and view historical data.

## Schematic Diagram

![Schematic Diagram](/Schematic_Diagram.PNG)

## Website Design

![Website Design](/Website_Design.png)