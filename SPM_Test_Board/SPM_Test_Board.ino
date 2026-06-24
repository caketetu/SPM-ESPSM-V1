//SPMの通信確認ボード用
//
//
//
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
  //システムLED
  pinMode(SYS_LED1, OUTPUT);
  pinMode(SYS_LED2, OUTPUT);
  digitalWrite(SYS_LED1, HIGH);
  digitalWrite(SYS_LED2, HIGH);
  //EN_SIG
  pinMode(EN_SIG, INPUT);
  //digitalWrite(EN_SIG, HIGH);

  //デバック用シリアル
  Serial.begin(115200);
  //RS485用シリアル
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
  static bool sys_led2_on;
  uint32_t currentMillis = millis();
  if (currentMillis - lastStamp > 500) {
    lastStamp = currentMillis;
    sys_led2_on = !sys_led2_on;
    digitalWrite(SYS_LED2, sys_led2_on);
  }

  digitalWrite(SYS_LED1, digitalRead(EN_SIG));  //EN信号=LED

  //シリアル受信時の処理…先頭文字で場合分け
  if (Serial.available()) {
    uint8_t rx_buf[50];
    int rx_len = 0;
    delay(10);
    while (Serial.available()) {
      rx_buf[rx_len++] = Serial.read();
    }
    //CRC確認
    uint16_t crc_check = convert_uint16(rx_buf[rx_len - 1], rx_buf[rx_len - 2]);
    if (crc_check == calc_crc(rx_buf, rx_len - 2)) {
      //先頭2文字でswitch
      uint16_t cmd = convert_uint16(rx_buf[0], rx_buf[1]);
      CanFrame obdFrame = { 0 };
      switch (cmd) {
        //RS485送信
        case 0x5223:
          uint8_t tx_buf[50];
          for (int i = 2; i < rx_len - 2; i++) {
            tx_buf[i - 2] = rx_buf[i];
          }
          Serial1.write(tx_buf, rx_len - 4);
          break;

        //CAN送信
        case 0x4323:
          obdFrame.identifier = rx_buf[2];  // Default OBD2 address;
          obdFrame.extd = 0;
          obdFrame.data_length_code = 8;
          obdFrame.data[0] = (uint8_t)rx_buf[3];
          obdFrame.data[1] = (uint8_t)rx_buf[4];
          obdFrame.data[2] = (uint8_t)rx_buf[5];
          obdFrame.data[3] = (uint8_t)rx_buf[6];
          obdFrame.data[4] = (uint8_t)rx_buf[7];
          obdFrame.data[5] = (uint8_t)rx_buf[8];
          obdFrame.data[6] = (uint8_t)rx_buf[9];
          obdFrame.data[7] = (uint8_t)rx_buf[10];
          // Accepts both pointers and references
          ESP32Can.writeFrame(obdFrame);  // timeout defaults to 1 ms
          break;

        //SPI送信
        // case 0x5323:
        //   break;
        default: break;
      }
    }
  }

  //RS485受信時の処理
  if (Serial1.available()) {
    uint8_t rx_buf[50];
    int rx_len = 0;
    rx_buf[rx_len++] = 'R';
    rx_buf[rx_len++] = '#';
    delay(10);
    while (Serial1.available()) {
      rx_buf[rx_len++] = Serial1.read();
    }
    uint16_t crc = calc_crc((uint8_t *)rx_buf, rx_len);
    rx_buf[rx_len++] = crc;
    rx_buf[rx_len++] = (uint8_t)(crc >> 8);
    Serial.write(rx_buf, rx_len);
  }

  //CAN受信時の処理
  if (ESP32Can.readFrame(rxFrame, 10)) {
    uint8_t rx_buf[50];
    int rx_len = 0;
    rx_buf[rx_len++] = 'C';
    rx_buf[rx_len++] = '#';
    rx_buf[rx_len++] = rxFrame.identifier;
    delay(10);
    for (int i = 0; i < 8; i++) {
      rx_buf[rx_len++] = rxFrame.data[i];
    }
    uint16_t crc = calc_crc((uint8_t *)rx_buf, rx_len);
    rx_buf[rx_len++] = crc;
    rx_buf[rx_len++] = (uint8_t)(crc >> 8);
    Serial.write(rx_buf, rx_len);
  }
}

uint16_t convert_uint16(uint8_t h_data, uint8_t l_data) {
  uint16_t conv_data = (uint16_t)(h_data << 8) + l_data;
  return conv_data;
}

// buf		受信データ
// length	受信データ長(CRCを除く)
uint16_t calc_crc(uint8_t *buf, int length) {
  uint16_t crc = 0xFFFF;
  int i, j;
  uint8_t carrayFlag;
  for (i = 0; i < length; i++) {
    crc ^= buf[i];
    for (j = 0; j < 8; j++) {
      carrayFlag = crc & 1;
      crc = crc >> 1;
      if (carrayFlag) {
        crc ^= 0xA001;
      }
    }
  }
  return crc;
}
