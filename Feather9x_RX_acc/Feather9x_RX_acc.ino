// Feather9x_RX for Feather RP2040 RFM95
#include <SPI.h>
#include <RH_RF95.h>
#include <string.h>

#define RFM95_CS   16
#define RFM95_INT  21
#define RFM95_RST  17
#define RF95_FREQ  915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

// MUST match the sender exactly
struct Payload {
  int16_t x;
  int16_t y;
  int16_t z;
  int16_t o;   // orientation
};

const char* orientationToString(int16_t o) {
  switch (o) {
    case 0: return "Portrait Up Front";
    case 1: return "Portrait Up Back";
    case 2: return "Portrait Down Front";
    case 3: return "Portrait Down Back";
    case 4: return "Landscape Right Front";
    case 5: return "Landscape Right Back";
    case 6: return "Landscape Left Front";
    case 7: return "Landscape Left Back";
    default: return "Unknown";
  }
}

void setup() {
  pinMode(LED_BUILTIN, OUTPUT);
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);

  Serial.begin(115200);
  delay(3000);   // lets USB serial come up, but does not block forever

  Serial.println("Feather LoRa RX starting...");

  // Manual radio reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  if (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1) {
      delay(10);
    }
  }
  Serial.println("LoRa radio init OK");

  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1) {
      delay(10);
    }
  }
  Serial.print("Set frequency to: ");
  Serial.println(RF95_FREQ);

  // Only needed if you want to send ACK replies
  rf95.setTxPower(20, false);

  Serial.println("Listening...");
}

void loop() {
  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      digitalWrite(LED_BUILTIN, HIGH);

      Serial.print("Received ");
      Serial.print(len);
      Serial.println(" bytes");

      // Print raw bytes in hex
      Serial.print("HEX: ");
      for (uint8_t i = 0; i < len; i++) {
        if (buf[i] < 0x10) Serial.print("0");
        Serial.print(buf[i], HEX);
        Serial.print(" ");
      }
      Serial.println();

      if (len == sizeof(Payload)) {
        Payload p;
        memcpy(&p, buf, sizeof(Payload));

        Serial.print("X: ");
        Serial.print(p.x);
        Serial.print("  Y: ");
        Serial.print(p.y);
        Serial.print("  Z: ");
        Serial.print(p.z);
        Serial.print("  Orientation: ");
        Serial.print(p.o);
        Serial.print(" (");
        Serial.print(orientationToString(p.o));
        Serial.println(")");
      } else {
        Serial.print("Unexpected packet size. Expected ");
        Serial.print(sizeof(Payload));
        Serial.print(", got ");
        Serial.println(len);
      }

      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi());

      // Optional reply-------------------
      uint8_t ack[] = "ACK";
      rf95.send(ack, sizeof(ack));
      rf95.waitPacketSent();
      Serial.println("Sent ACK");
      //----------------------------------

      digitalWrite(LED_BUILTIN, LOW);
      Serial.println("Listening...");
    } else {
      Serial.println("Receive failed");
    }
  }
}