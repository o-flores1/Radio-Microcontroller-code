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

//MMA8451
#include <Adafruit_MMA8451.h>
#include <Adafruit_Sensor.h>

Adafruit_MMA8451 mma = Adafruit_MMA8451();

//MicroSD Card
#include <SD.h>

const int SD_CS = 9; //Only if CS is connected to D9

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

//---------------------------------Functions--------------------------------//
//SD File
void SDfile() {

  //deselect LoRa, and select SD card
  digitalWrite(RFM95_CS, HIGH);
  digitalWrite(SD_CS, LOW);

  //Build file name
  char filename[] = "VSD.CSV"; //very special data

      // reads accelerometer axis readings
      mma.read();

      // used to read acceleration
      sensors_event_t event;
      mma.getEvent(&event);

      //checks orientation
      uint8_t o = mma.getOrientation();

      File f = SD.open(filename, FILE_WRITE);

      if (f) {

      // write one line to file
      f.print(millis()); 
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

      f.close();
      }
      Serial.println("f"); // Serial monitor check
      digitalWrite(SD_CS, HIGH); // Deselects when SD is done writing
  }


//creates and transmits the packet.
//data only viewable if reciever is programmed to "decode"
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

  Serial.println("tx"); //serial monitor check

}

//-------------------------------------------------------------------------//

//defines layout
//MISO / D12, CS, SCK, MOSI
SPIClassRP2040 sdSPI(spi1, 12, 9, 14, 15);   //12 to DO

void setup() {
  Serial.begin(115200);
  delay(3000);

  //defines I2C layout
  Wire.setSCL(3);
  Wire.setSDA(2);
  Wire.begin();

  //Initializes SPI priority
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
  //might want to increase
  rf95.setTxPower(13, false);

  //mma check
  if (!mma.begin()) {
    Serial.println("Could not find MMA8451");
    while (1) {
      delay(10);
    }
  }

  mma.setRange(MMA8451_RANGE_2_G);

  sdSPI.setMISO(12);
  sdSPI.setMOSI(15);
  sdSPI.setSCK(14);
  
  //SD check
  if (!SD.begin(SD_CS, 4000000, sdSPI)) {
    Serial.println("SD init failed");
    while (1) {
      delay(10);
    }
  }

  // Creates header for CSV file.
  File f = SD.open("VSD.CSV", FILE_WRITE);
  if (f) {
    // CSV header
    f.println("time_ms,raw_x,raw_y,raw_z,accel_x,accel_y,accel_z,orientation");
    f.close();
  }

  Serial.println("Setup Complete");

}

void loop() {
  SDfile();

  sendAccelHex();
  delay(100);

}
