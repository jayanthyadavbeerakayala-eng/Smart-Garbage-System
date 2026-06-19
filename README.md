# Smart Garbage System for Gated Communities

## Overview

The Smart Garbage System is an IoT-based waste management solution designed to automate waste segregation and bin monitoring in gated communities. The system identifies wet and dry waste, directs it to the appropriate compartment, monitors fill levels, and sends SMS alerts when bins become full.

## Features

* Automatic wet/dry waste segregation
* IR sensor-based waste detection
* Servo motor controlled waste routing
* Dual ultrasonic sensor bin monitoring
* GSM-based SMS alerts
* Real-time monitoring through Arduino IDE Serial Monitor
* Low-cost embedded system implementation

## Hardware Components

* Arduino Uno
* FC-28 Moisture Sensor
* IR Sensor
* SG90 Servo Motor
* HC-SR04 Ultrasonic Sensors (2)
* SIM800L GSM Module
* Breadboard
* Jumper Wires
* 3.7V Li-ion Battery

## System Workflow

1. IR sensor detects incoming waste.
2. Moisture sensor classifies waste as wet or dry.
3. Servo motor directs waste into the appropriate compartment.
4. Ultrasonic sensors monitor dry and wet bin fill levels.
5. GSM module sends SMS alerts when a bin reaches 80% capacity.
6. System status is displayed on the Serial Monitor.

## Technologies Used

* Embedded Systems
* Arduino Programming
* IoT
* Sensor Interfacing
* GSM Communication
* Automation

## Project Team

* B. Jayanth
* Jinka Divij
* T. Sahith Varma

### Guide

* Dr. K. Deepti
* Mr. P. Bhargava

## Future Scope

* Cloud Dashboard using ESP32
* Mobile Application Integration
* AI-Based Waste Classification
* Solar-Powered Operation
* Multi-Bin Monitoring Network

## License

Academic Project - Vasavi College of Engineering
