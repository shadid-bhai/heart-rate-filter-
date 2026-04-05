##ESP32 Real-Time ECG Monitor & Recorder

This project provides a complete hardware and software solution for monitoring and logging ECG signals using an ESP32 and the AD8232 heart rate monitor module. It features a web-based dashboard that serves a real-time visualization of the heart's electrical activity and allows for structured data export for clinical or analytical use.

## Key Features

    Real-Time Visualization: High-fidelity waveform rendering on a web-based canvas with a built-in grid system.

    Web Server Architecture: Hosted directly on the ESP32 via a Soft Access Point (SoftAP); no external internet connection required.

    Intelligent Lead Detection: Automatic monitoring of electrode contact status using the AD8232's Lead-Off (LO+ / LO-) pins.

    Data Export: Captures patient-specific recording sessions and exports them into a formatted Excel (.xls) file containing timestamps and raw ECG values.

    Optimized Sampling: Integrated lead-off logic that zeros data during disconnection to prevent signal noise interpretation.

## Technical Specifications

Microcontroller	- ESP32
Input Resolution - 12-bit ADC (0-4095)

Sampling Rate	- ~20ms interval for web updates

Wireless Mode - Wi-Fi SoftAP (SSID: ESP32-ECG)

Default Gateway	- 192.168.4.1

## Hardware Setup

    ECG Module: Connect an AD8232 to the ESP32.

    Pin Configuration:

        ECG Signal: GPIO 34 

        LO+: GPIO 26 

        LO-: GPIO 27 

    Power: Ensure proper 3.3V and GND connections between the module and the ESP32.

    ## Getting Started

    Flash the Firmware: Upload the provided .ino code to your ESP32.

    Connect to Wi-Fi: On your laptop or smartphone, connect to the network ESP32-ECG using the password 12345678.

    Access Dashboard: Open your browser and navigate to 192.168.4.1.

    Record Data: Enter the patient's name and age, then click Start Recording.

    Download: Once finished, stop the session to generate and download the Excel data sheet.

  ## Future Improvements

    Implementation of digital filters (Notch/Bandpass) to reduce 50/60Hz power line interference.

    Integration of BPM (Beats Per Minute) calculation logic.

    Support for multiple lead configurations.
