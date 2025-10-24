# ESP32 Serial Projects

This repository contains a collection of DIY projects focused on serial communication, primarily using ESP32 microcontrollers and single-board computers like the Raspberry Pi.

## 1. ESP32_WebSerial (WiFi-to-Serial Bridge)

This project turns an ESP32 into a wireless bridge, allowing you to access a Raspberry Pi's serial console (or any other serial device) from a web browser on any device on your network.

It's perfect for "headless" setups where you need console access without plugging in a monitor or USB-to-serial cable.

### Features


- **Web-Based Terminal:** Access a full-featured `xterm.js` terminal in your browser.


- **WebSocket Bridge:** Provides a low-latency, bi-directional link between the web terminal and the ESP32's serial port.


- **WiFi Manager (AP Mode):** On first boot or if WiFi fails, the ESP32 creates an Access Point (`ESP32-Serial-Config`). You can connect to it, and a captive portal will automatically open, allowing you to scan for and save your home WiFi credentials.


- **Station Mode (STA):** Once configured, it automatically connects to your WiFi and serves the web terminal.


- **OLED Status Screen:** An SSD1306 OLED display shows the current mode (AP/STA), IP address, WiFi SSID, client count, and system uptime. When a web client connects, the screen displays "CLIENT CONNECTED" for privacy and awareness.


- **Dynamic Baud Rate:** You can change the bridge's serial speed by sending a special command from the web terminal (e.g., `CMD::BAUD SET 9600 8N1`).



### Hardware Required


- ESP32 (WROOM, S2, S3, etc.)


- Raspberry Pi (or other device with a 3.3V serial console)


- SSD1306 I2C OLED Display (128x64)


- Jumper wires



### Pin Connections (ESP32)


- **Serial1 (to Pi):**


- `GPIO 20 (PI_RX_PIN)` -> Pi `TXD`


- `GPIO 9 (PI_TX_PIN)` -> Pi `RXD`




- **OLED (I2C):**


- `SCL` -> `GPIO 22` (default)


- `SDA` -> `GPIO 21` (default)




- **Ground:**


- `GND` -> Pi `GND`


- `GND` -> OLED `GND`





### How to Use


1. **Flash:** Compile and flash the code to your ESP32 using the Arduino IDE or PlatformIO.


1. **Connect (First Time):**


- Power on the ESP32.


- On your phone or laptop, connect to the `ESP32-Serial-Config` WiFi network.


- A captive portal page should open. (If not, go to `192.168.4.1`).


- Scan, select your home WiFi network, enter the password, and click "Save & Reboot".




1. **Access Terminal (Normal Use):**


- The ESP32 will reboot and connect to your WiFi.


- The OLED screen will display the new IP address.


- Open a web browser on a device on the *same network* and navigate to that IP address (e.g., `http://192.168.1.123`).


- You will be greeted by the web terminal, connected directly to your Raspberry Pi's serial console.





## 2. USBSerial (USB Serial Dongle)

This folder contains the project files for a native USB Serial Dongle.

### Description

This project uses an ESP32-S2 or S3's native USB-OTG capabilities to act as a high-performance, cross-platform USB-to-Serial (UART) adapter. It's a modern, open-source alternative to traditional FTDI or CH340/CH341 dongles.

### Features


- **Native USB-CDC:** Uses the built-in USB controller, appearing as a standard "Communications Device" on modern operating systems.


- **No Drivers:** No special drivers are needed for Windows 10/11, macOS, or Linux.


- **Selectable Logic Level:** A physical jumper allows you to select between 3.3V and 5V logic for compatibility with a wide range of devices.


- **Activity LEDs:** On-board RX and TX LEDs show serial activity at a glance.


- **High Speed:** Supports high baud rates for fast data transfer.



### Hardware Required


- ESP32-S2 or ESP32-S3 (a module with USB pins, like the S3-Mini)


- Bi-directional logic level shifter (e.g., TXS0108E or similar)


- 3-pin header for logic level selection (3.3V, 5V)


- 2x LEDs (e.g., blue for RX, green for TX)


- 2x current-limiting resistors (e.g., 220-330 Ohm)


- A USB-C or Micro-USB connector and a custom PCB (or careful perfboard wiring).



### Setup


1. **Flash:** Compile and flash the `USBSerial` code to your ESP32-S2/S3. The code utilizes the `USB-CDC` library (part of the Arduino core or ESP-IDF).


1. **Plug In:** Connect the ESP32 to your computer via its USB port. Your computer will recognize it as a new COM port (e.g., `COM3` on Windows, `/dev/ttyACM0` on Linux).


1. **Set Voltage:** Set the logic level jumper on the dongle to 3.3V or 5V to match your target device.


1. **Connect Target:** Connect the dongle's `TX`, `RX`, and `GND` pins to your target device (e.g., a Raspberry Pi, Arduino, or other microcontroller).


1. **Use:** Open a serial terminal program (like PuTTY, Tera Term, or the Arduino Serial Monitor), select the new COM port, set your baud rate, and connect.
