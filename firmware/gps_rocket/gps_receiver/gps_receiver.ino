//General
#include <Wire.h>

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

  //GPS
  float lat;
  float lon;
  uint8_t fix;
  uint8_t sats;
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


//SD
#include <SD.h>

const int SD_CS = 9; //Only if CS is connected to D9

//OLED
#include <Adafruit_SSD1306.h>
#include <Adafruit_GFX.h> //I believe only needed if we decide to display graphics

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

//Buzzer
#define BUZZER_PIN 6
//HIGH = OFF
//LOW = ON

//Tracking using Buzzer
unsigned long lastBeepTime = 0;
int currentRSSI = -140; // Default weak signal

//Turn off buzzer if it hasn't gotten a packet in a certain amount of time
unsigned long lastPacketTime = 0;
const int timeoutLimit = 5000; // if nothing is gotten in 5 seconds turn off buzzer.

Payload p; //Global so RX and SDfile can reach

//---------------------------------Functions--------------------------------//
void RX() {
  digitalWrite(RFM95_CS, LOW);
  digitalWrite(SD_CS, HIGH);

  if (rf95.available()) {
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)) {
      if (len == sizeof(Payload)) {
        memcpy(&p, buf, sizeof(Payload));
        
        currentRSSI = rf95.lastRssi();
        updateOLED(p, currentRSSI);
        lastPacketTime = millis();

        // prints on one line to serial monitor
        Serial.print("X:"); Serial.print(p.x);
        Serial.print(" Y:"); Serial.print(p.y);
        Serial.print(" Z:"); Serial.print(p.z);
        Serial.print(" RSSI:"); Serial.println(currentRSSI);
        Serial.print("Sats: "); Serial.print(p.sats);
        Serial.print(" Lat: "); Serial.print(p.lat, 6);
        Serial.print(" Lon: "); Serial.println(p.lon, 6);
      }
    }
  }
  digitalWrite(RFM95_CS, HIGH);
}

void SDfile() {

  digitalWrite(RFM95_CS,HIGH);
  digitalWrite(SD_CS, LOW);

  char filename[] = "RX_VSD.CSV";

  File f = SD.open(filename, FILE_WRITE);
    if (f) {
      f.print(p.x); f.print(",");
      f.print(p.y); f.print(",");
      f.print(p.z); f.print(",");
      f.print(p.o); f.print(",");
      f.println(rf95.lastRssi());
      f.close();
    }
  digitalWrite(SD_CS, HIGH);
}

void updateOLED(Payload p, int rssi) {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  display.setCursor(0,0);
  display.print("RSSI: "); display.println(rssi);
  display.print("  Sats: "); display.println(p.sats);
  display.println("-----------");

  //GPS
  if (p.fix) {
    display.print("Lat: "); display.println(p.lat, 6);
    display.print("Lon: "); display.println(p.lon, 6);
  } else {
    display.println("WAITING FOR GPS FIX");
  }

  display.println("---------------------");
  display.print("X:"); display.print(p.x);
  display.print(" Y:"); display.print(p.y);
  display.print(" Z:"); display.println(p.z);
  display.print("O: "); display.println(orientationToString(p.o));
  
  display.display();
}

void handleBeep(){

  //Checks if anything has been transmitted in the last 5 seconds if no then stop beeping.
  if (millis() - lastPacketTime > timeoutLimit) {
    digitalWrite(BUZZER_PIN, HIGH);
    
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("No Transmission");
    display.display();

    currentRSSI = -140;
    return;
  }
  
  // Map RSSI (-140 to -30) to a delay (2000ms to 80ms)
  // Closer to 0 = stronger signal = shorter delay
  int interval = map(currentRSSI, -140, -30, 2000, 80);
  interval = constrain(interval, 80, 2000);

  if (millis() - lastBeepTime >= interval) {
    digitalWrite(BUZZER_PIN, LOW); 
    delay(30);                      
    digitalWrite(BUZZER_PIN, HIGH); 
    lastBeepTime = millis();
  }
}

//-------------------------------------------------------------------------//

SPIClassRP2040 sdSPI(spi1, 12, 9, 14, 15);

void setup() {
  //Initialize Buzzer
  pinMode(BUZZER_PIN, OUTPUT);
  digitalWrite(BUZZER_PIN, HIGH);

  //Pin Mode for selecting SPI usage
  pinMode(RFM95_CS, OUTPUT);
  digitalWrite(RFM95_CS, HIGH);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, HIGH);

  Serial.begin(250000);
  delay(3000); 

  //Initialize I2C pins
  Wire.setSCL(3);
  Wire.setSDA(2);
  Wire.begin();

  //radio check
  if (!rf95.init()) {
    Serial.println("LoRa radio init failed");
    while (1) {
      delay(10);
    }
  }
  Serial.println("LoRa radio init OK");

  //OLED check
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("SSD1306 allocation failed");
  }
  display.clearDisplay();
  display.display();

  display.clearDisplay();
  display.setTextSize(2);               
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(10, 15);            
  display.println(F("TRACKING"));      
  
  display.setTextSize(1);               
  display.setCursor(20, 40);
  display.println(F("SYSTEM STARTING..."));
  
  display.display();

  // freq check
  if (!rf95.setFrequency(RF95_FREQ)) {
    Serial.println("setFrequency failed");
    while (1) {
      delay(10);
    }
  }

  Serial.println("Listening...");

  //SD check
  if (!SD.begin(SD_CS, 4000000, sdSPI)) {
    Serial.println("SD init failed");
    while (1) {
      delay(10);
    }
  }

  // Creates header for CSV file.
  File f = SD.open("RX_VSD.CSV", FILE_WRITE);
  if (f) {
    // CSV header
    f.println("time_ms,raw_x,raw_y,raw_z,accel_x,accel_y,accel_z,orientation,fix,sats,lat,lon,rssi");
    f.close();
  }

  delay(2000);
  display.clearDisplay();

}

void loop() {
  RX();
  
  //Only record on SD card when signal is strong enough to warrant recording
  if (currentRSSI > -120) {
    SDfile();
  }

  handleBeep();
}