//General
#include <Wire.h>
#include <SPI.h>

//RP 2040 Feather RFM95 LoRa
#include <RH_RF95.h>

#define RFM95_CS   16
#define RFM95_INT  21
#define RFM95_RST  17

    //frequency
#define RF95_FREQ 915.0

RH_RF95 rf95(RFM95_CS, RFM95_INT);

struct Payload {
  int16_t x;
  int16_t y;
  int16_t z;
  int16_t o; //orientation
};

//LCD
#include <Adafruit_LiquidCrystal.h>

//MMA8451
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();

//MicroSD Card
#include <SD.h>

const int SD_CS = 9; //Only if CS is connected to D9
int count = 1;

const unsigned long RECORD_TIME_MS = 30000;    // 30 seconds
const unsigned long SAMPLE_INTERVAL_MS = 100;  // sample every 100 ms

const char* orientationToString(uint8_t o) {
  switch (o) {
    case MMA8451_PL_PUF: return "Portrait Up Front";
    case MMA8451_PL_PUB: return "Portrait Up Back";
    case MMA8451_PL_PDF: return "Portrait Down Front";
    case MMA8451_PL_PDB: return "Portrait Down Back";
    case MMA8451_PL_LRF: return "Landscape Right Front";
    case MMA8451_PL_LRB: return "Landscape Right Back";
    case MMA8451_PL_LLF: return "Landscape Left Front";
    case MMA8451_PL_LLB: return "Landscape Left Back";
    default: return "Unknown";
  }
}

//SD File
void SDfile() {

//deselect LoRa while writing
digitalWrite(RFM95_CS, HIGH);

//Build file name
char filename[13];
snprintf(filename, sizeof(filename), "LOG%03d.CSV", count);

//on reboot deletes pre-existing files with the same file name.
if (SD.exists(filename)) {
  SD.remove(filename);
}

  //Write file
  File f = SD.open(filename, FILE_WRITE);
  if (!f) {
    Serial.print("Could not open ");
    Serial.println(filename);
    return;
    }

  // CSV header
  f.println("time_ms,raw_x,raw_y,raw_z,accel_x,accel_y,accel_z,orientation");

  unsigned long startTime = millis();
  unsigned long lastSample = 0;

  while (millis() - startTime < RECORD_TIME_MS) {
    if (millis() - lastSample >= SAMPLE_INTERVAL_MS) {
      lastSample = millis();

      // reads accelerometer axis readings
      mma.read();

      // used to read acceleration
      sensors_event_t event;
      mma.getEvent(&event);

      //checks orientation
      uint8_t o = mma.getOrientation();

      unsigned long elapsed = millis() - startTime;

      // write one line to file
      f.print(elapsed);
      f.print(",");

      f.print(mma.x);
      f.print(",");
      f.print(mma.y);
      f.print(",");
      f.print(mma.z);
      f.print(",");

      f.print(event.acceleration.x);
      f.print(",");
      f.print(event.acceleration.y);
      f.print(",");
      f.print(event.acceleration.z);
      f.print(",");

      f.println(orientationToString(o));
    }
  }
  f.close();

    Serial.print("File ");
    Serial.print(filename);
    Serial.println(" finished");

    count++;
  }


//creates and transmits the packet.
//data only viewable if reciever is programmed to decode.
void sendAccelHex() {

  //deselect SD before transmitting
  digitalWrite(SD_CS, HIGH);

  mma.read();

  Payload p;
  p.x = mma.x;
  p.y = mma.y;
  p.z = mma.z;
  p.o = mma.getOrientation();

  //send packet
  rf95.send((uint8_t*)&p, sizeof(p));
  rf95.waitPacketSent();

  //Serial Check
  uint8_t* raw = (uint8_t*)&p;
  Serial.print("TX hex: ");
  for (size_t i = 0; i < sizeof(p); i++) {
    if (raw[i] < 0x10) Serial.print("0");
    Serial.print(raw[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

}



void setup() {
  Serial.begin(115200);
  delay(3000);

  Wire.begin();

  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  //radio check
  if (!rf95.init()) {
    Serial.println("LoRa init failed");
    while (1) delay(10);
  }

  //freq check
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1) delay(10);
  }

  //transmittion power
  rf95.setTxPower(20, false);

  //mma check
  if (!mma.begin()) {
    Serial.println("Could not find MMA8451");
    while (1) {
      delay(10);
    }
  }

  mma.setRange(MMA8451_RANGE_2_G);

  //SD check
  if (!SD.begin(SD_CS)) {
    Serial.println("SD init failed");
    while (1) {
      delay(10);
    }
  }

  Serial.println("Setup Complete");

}

void loop() {
  SDfile();
  delay(5000);

  // optional serial output so you can see it live
      Serial.print("X:");
      Serial.print(mma.x);
      Serial.print(" Y:");
      Serial.print(mma.y);
      Serial.print(" Z:");
      Serial.print(mma.z);
      Serial.print("  Orientation: ");
      Serial.println(orientationToString(mma.getOrientation()));

  sendAccelHex();
  delay(1000);

}
