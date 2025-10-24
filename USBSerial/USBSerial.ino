/*
 * ESP32C3 Serial Dongle / Pass-through Sketch
 *
 * This sketch turns a Seeed Studio Xiao ESP32C3 into a USB-to-Hardware-Serial
 * bridge, matching the core settings of your stty output:
 * - 115200 baud
 * - 8 data bits, no parity, 1 stop bit (8N1)
 * - No hardware or software flow control
 *
 * Connections:
 * - Connect the Xiao ESP32C3 to your computer via USB.
 * This will appear as `Serial` (e.g., /dev/ttyACM0 or COM3).
 * - Connect the target device to the Xiao's hardware UART pins:
 * - Xiao D6 (GPIO20) -> Target Device's TX pin
 * - Xiao D7 (GPIO21) -> Target Device's RX pin
 * - Xiao GND         -> Target Device's GND
 */

// Define the hardware serial port we will use for the target device
// On the ESP32C3, Serial1 is the first hardware UART.
#define HW_SERIAL Serial1

// Define the baud rate to match the stty setting
#define BAUD_RATE 115200

// Define the hardware UART pins on the Xiao ESP32C3
// RX_PIN (GPIO20) is D6 on the Xiao
// TX_PIN (GPIO21) is D7 on the Xiao
#define RX_PIN 20
#define TX_PIN 9

void setup() {
  // Start the USB-CDC Serial port (what the computer sees)
  Serial.begin(BAUD_RATE);

  // Start the Hardware Serial port (what the target device connects to)
  // We explicitly set the config to SERIAL_8N1 to match:
  // - cs8 (8 data bits)
  // - -parenb (No parity)
  // - -cstopb (1 stop bit)
  // We also specify the RX and TX pins.
  HW_SERIAL.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);

  // Give a moment for ports to initialize
  delay(100);
  Serial.println("--- ESP32C3 Serial Dongle Initialized ---");
  Serial.print("Baud Rate: ");
  Serial.println(BAUD_RATE);
  Serial.println("Mode: 8N1, No Flow Control");
  Serial.println("Forwarding data between USB and Hardware Serial (D6/D7)...");
}

void loop() {
  // This is the core bridge logic.
  // It shuffles data from one port to the other as fast as it arrives.

  // 1. Check for data from USB (computer) and send to Hardware UART (device)
  // Use a while loop to empty the buffer in one go for better performance
  while (Serial.available()) {
    HW_SERIAL.write(Serial.read());
  }

  // 2. Check for data from Hardware UART (device) and send to USB (computer)
  while (HW_SERIAL.available()) {
    Serial.write(HW_SERIAL.read());
  }
}
