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

- ## System Summary
This project demonstrates a complete end-to-end IoT architecture:
- Two Arduino-based End Nodes for sensing and actuation
- One ESP32 Edge Node acting as a BLE–WiFi gateway
- Cloud backend and Web UI for monitoring and data visualization

The design emphasizes modularity, scalability, and real-world deployment considerations.


## Author
Anand PS  
MTech Embedded Systems
