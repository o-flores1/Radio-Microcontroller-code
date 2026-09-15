// Feather9x_TX
// -*- mode: C++ -*-
// Example sketch showing how to create a simple messaging client (transmitter)
// with the RH_RF95 class. RH_RF95 class does not provide for addressing or
// reliability, so you should only use RH_RF95 if you do not need the higher
// level messaging abilities.
// It is designed to work with the other example Feather9x_RX

#include <SPI.h>
#include <RH_RF95.h>
#include <Adafruit_STHS34PF80.h> // Sensor Library

// First 3 here are boards w/radio BUILT-IN. Boards using FeatherWing follow.

//#if defined(ARDUINO_ADAFRUIT_FEATHER_RP2040_RFM)  // Feather RP2040 w/Radio [this one]
  #define RFM95_CS   16
  #define RFM95_INT  21
  #define RFM95_RST  17
 // Change to 434.0 or other frequency, must match RX's freq!
#define RF95_FREQ 915.0

// Singleton instance of the radio driver
RH_RF95 rf95(RFM95_CS, RFM95_INT);
Adafruit_STHS34PF80 sths = Adafruit_STHS34PF80();


void setup() {
  pinMode(RFM95_RST, OUTPUT);
  digitalWrite(RFM95_RST, HIGH);
  //a visual indicator for me to know that the code is working.
  pinMode(LED_BUILTIN, OUTPUT);

  Serial.begin(115200);
  delay(100);

  Serial.println("Feather LoRa TX + STHS Test");
  // -- Initializes Sensor --
  if(!sths.begin()) {
    Serial.println("Could not find STHS34PF80 sensor");
    while (1) delay (10);
  }

  // -- Initializes Radio --
  // manual reset
  digitalWrite(RFM95_RST, LOW);
  delay(10);
  digitalWrite(RFM95_RST, HIGH);
  delay(10);

  while (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    Serial.println("Uncomment '#define SERIAL_DEBUG' in RH_RF95.cpp for detailed debug info");
    while (1);
  }
  Serial.println("LoRa radio init OK!");

  // Defaults after init are 434.0MHz, modulation GFSK_Rb250Fd250, +13dbM
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1);
  }
  Serial.print("Set Freq to: "); Serial.println(RF95_FREQ);

  // Defaults after init are 434.0MHz, 13dBm, Bw = 125 kHz, Cr = 4/5, Sf = 128chips/symbol, CRC on

  // The default transmitter power is 13dBm, using PA_BOOST.
  // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then
  // you can set transmitter powers from 5 to 23 dBm:
  rf95.setTxPower(23, false);
}


int16_t packetnum = 0;  // packet counter, we increment per xmission

void loop() {
  delay(1000); // Wait 1 second between transmits, could also 'sleep' here!
  Serial.println("Transmitting..."); // Send a message to rf95_server

  int16_t presence = sths.readPresence();
    if (presence > 400){
      Serial.print("Detected");
      Serial.println(presence);

      digitalWrite(LED_BUILTIN, HIGH);

      String radiopacket = "#";          
      radiopacket.concat(packetnum++);
      Serial.print("Sent: "); 
      Serial.println(radiopacket);

      //Serial.println("Sending...");
      delay(10);
      rf95.send((uint8_t *)radiopacket.c_str(), radiopacket.length()+1);  

      //Serial.println("Waiting for packet to complete...");
      delay(10);
      rf95.waitPacketSent();

      delay(500);
      digitalWrite(LED_BUILTIN, LOW);

    }
  

  Serial.println("Waiting for packet to complete...");
  delay(10);
  rf95.waitPacketSent();
  // Now wait for a reply
  uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t len = sizeof(buf);

  Serial.println("Waiting for reply...");
  if (rf95.waitAvailableTimeout(1000)) {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      Serial.print("Got reply: ");
      Serial.println((char*)buf);
      Serial.print("RSSI: ");
      Serial.println(rf95.lastRssi(), DEC);
    } else {
      Serial.println("Receive failed");
    }
  } else {
    Serial.println("No reply, is there a listener around?");
  } 

}
