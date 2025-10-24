#define HW_SERIAL Serial1
#define BAUD_RATE 115200
#define RX_PIN 20
#define TX_PIN 9

void setup() {
  Serial.begin(BAUD_RATE);
  HW_SERIAL.begin(BAUD_RATE, SERIAL_8N1, RX_PIN, TX_PIN);
}

void loop() {
  while (Serial.available()) {
    HW_SERIAL.write(Serial.read());
  }
  while (HW_SERIAL.available()) {
    Serial.write(HW_SERIAL.read());
  }
}
