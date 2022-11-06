
#include <TinyGPSPlus.h>
#include <FS.h>
#include "SD.h"
#include "SPI.h"
#include <time.h>
#include <WiFi.h>
#include <string.h>
#include "ESPTrueRandom.h"
#include <ArduinoJson.h>
TinyGPSPlus gps;
#include "utilities.h"
void logInfo(String msg) {
  Serial.println("[ Info ]" + msg);
}



void setup() {
  pinMode(GREEN_LED_PIN, OUTPUT);
  event_uuid = getShortUUID();
  digitalWrite(GREEN_LED_PIN, LOW);

  Serial.begin(115200);
  Serial.println("Starting...");
  Serial.print("Trying to get current GPS baudrate, baudrate:");
  long detected_gps_baud_rate = autoBaud();
  Serial2.begin(detected_gps_baud_rate, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial2.write("$PCAS01,5*19\r\n");
  Serial2.write("$PCAS02,200*1D\r\n");
  detected_gps_baud_rate = autoBaud();
  Serial2.begin(detected_gps_baud_rate, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
  Serial.println("** Smart GPS Tracker **");
  Serial.println("Starting SPI SDCARD");


  if (!SD.begin()) {
    Serial.begin(115200);
    Serial.println("Card Mount Failed");
    while (1);
    return;
  }
  logInfo("Check SD usage");
  int tBytes = SD.totalBytes();
  int uBytes = SD.usedBytes();
  Serial.print("Check SD usage(unit: Byte): Total:");
  Serial.print(tBytes);
  Serial.print(" / Used: ");
  Serial.println(uBytes);
  listDir(SD, "/", 0);


  //deleteFile(SD, gps_logging_filename.c_str());
  WIFI_Connect();

  NTP_SYNC();
  gps_logging_filename = "/location-" + getMACAddress() + "-" + event_uuid + ".csv";
  writeFile(SD, gps_logging_filename.c_str(), "Date, Time, Lat, Lng, Alt\n");
  digitalWrite(GREEN_LED_PIN, LOW);
}

void loop() {
  while (Serial2.available() > 0)
    if (gps.encode(Serial2.read())) {
      displayInfo();
    }

  if (millis() > 5000 && gps.charsProcessed() < 10) {
    Serial.println(F("No GPS detected: check wiring."));
    while (true);
  }

}
