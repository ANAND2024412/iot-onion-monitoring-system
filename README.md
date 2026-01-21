# IoT-Based Onion Storage Monitoring System

## Overview
A multi-node IoT system designed to monitor and control onion storage conditions
to reduce spoilage and improve shelf life.

## System Architecture
The system consists of:
- End Nodes - 2 (Arduino-based sensing and control)
- Edge Node (ESP32-based gateway)
- Server & Cloud with Web UI

## Communication Flow
- End Node ↔ Edge Node : BLE
- Edge Node ↔ Server : WiFi / Internet

## Technologies Used
- Arduino, ESP32
- Embedded C
- BLE, WiFi
- Cloud database & Web UI

## Author
Anand PS  
MTech Embedded Systems
