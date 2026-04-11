#include <ESP32-TWAI-CAN.hpp>

#define SYS_LED1 3
#define SYS_LED2 38

#define RS485_TX 43
#define RS485_RX 44

#define CAN_TXD 40
#define CAN_RXD 39

#define EN_SIG 2

CanFrame rxFrame;

void setup() {
  // put your setup code here, to run once:
  pinMode(SYS_LED1, OUTPUT);
  pinMode(SYS_LED2, OUTPUT);
  digitalWrite(SYS_LED1, HIGH);
  digitalWrite(SYS_LED2, HIGH);

  pinMode(EN_SIG, INPUT);
  //digitalWrite(EN_SIG, HIGH);

  // put your setup code here, to run once:
  Serial.begin(115200);
  //Serial1.begin(115200);
  Serial1.begin(115200, SERIAL_8N1, 41, 42);
  //Serial1.begin(115200, SERIAL_8N1, 44, 43);

  // CAN.onReceive(onReceive);
  // Set pins
  ESP32Can.setPins(CAN_TXD, CAN_RXD);
  // You can set custom size for the queues - those are default
  ESP32Can.setRxQueueSize(5);
  ESP32Can.setTxQueueSize(5);
  // .setSpeed() and .begin() functions require to use TwaiSpeed enum,
  // but you can easily convert it from numerical value using .convertSpeed()
  ESP32Can.setSpeed(ESP32Can.convertSpeed(500));
  // You can also just use .begin()..
  if (ESP32Can.begin()) {
    Serial.println("CAN bus started!");
  } else {
    Serial.println("CAN bus failed!");
  }
}

void loop() {
  // put your main code here, to run repeatedly:
  static uint32_t lastStamp = 0;
  uint32_t currentMillis = millis();

  if (currentMillis - lastStamp > 100) {
    lastStamp = currentMillis;
    CanFrame obdFrame = { 0 };
    obdFrame.identifier = 0x014;  // Default OBD2 address;
    obdFrame.extd = 0;
    obdFrame.data_length_code = 8;
    obdFrame.data[0] = (uint8_t)('h');
    obdFrame.data[1] = (uint8_t)('e');
    obdFrame.data[2] = (uint8_t)('l');
    obdFrame.data[3] = (uint8_t)('l');
    obdFrame.data[4] = (uint8_t)('o');
    obdFrame.data[5] = (uint8_t)(' ');
    obdFrame.data[6] = (uint8_t)('!');
    obdFrame.data[7] = (uint8_t)('!');
    // Accepts both pointers and references
    ESP32Can.writeFrame(obdFrame);  // timeout defaults to 1 ms

    Serial.println(digitalRead(EN_SIG));
    //Serial.println(analogRead(EN_SIG));
  }

  if (ESP32Can.readFrame(rxFrame, 10)) {
    Serial.print("CAN Recv");
    Serial.print(rxFrame.identifier);
    Serial.print("  ");
    for (int i = 0; i < 8; i++) {
      Serial.write(rxFrame.data[i]);
    }
    Serial.println();
  }

  if(Serial1.available()){
   char a = Serial1.read();
   Serial1.write(a);
   Serial.write(a);
  }
}
