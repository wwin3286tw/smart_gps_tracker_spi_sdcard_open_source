String BUADRATE_CMD_LIST[8] = {"$PCAS01,0", "$PCAS01,1", "$PCAS01,2", "$PCAS01,3", "$PCAS01,4", "$PCAS01,5"};
int BUADRATE_LIST[8] = {4800, 9600, 19200, 38400, 57600, 115200};
int GPS_UPDATE_RATE[5] = {1, 2, 4, 5, 10};
String GPS_UPDATE_CMD_LIST[5] = {"$PCAS02,1000", "$PCAS02,500", "$PCAS02,250", "$PCAS02,200", "$PCAS02,100"};
#include <HTTPClient.h>

#include <base64.h>
#include <WiFiClientSecure.h>
bool DEBUG = true;
long timezone = 1;
byte daysavetime = 1;
// Filling your auth info bwlow
const char* ssid     = "";
const char* password = "";
const char* REST_SERVER = "";
String authUsername = "";
String authPassword = "";
String auth = base64::encode(authUsername + ":" + authPassword);
#define REST_SUPPORT false
#define GPS_BAND_RATE      9600
#define GREEN_LED_PIN 22
#define GPS_RX_PIN 16
#define GPS_TX_PIN 17
String event_uuid = "";
String  gps_logging_filename =  "";
String JsonPayload = "";
const char* rootCACertificate = "";


String getMACAddress() {
  String hexstring = "";
  uint8_t macAddr[6];
  int mac_size = sizeof(macAddr) - 1;
  WiFi.macAddress(macAddr);
  for (int i = 0; i <= mac_size; i++) {
    if (macAddr[i] < 0x10) {
      hexstring += '0';
    }
    hexstring += String(macAddr[i], HEX);
  }
  return hexstring;
}
int nmea0183_checksum(char *nmea_data)
{
  int crc = 0;
  int i;

  // ignore the first $ sign,  no checksum in sentence
  for (i = 1; i < strlen(nmea_data); i ++) { // removed the - 3 because no cksum is present
    crc ^= nmea_data[i];
  }

  return crc;
}


long detectRate(int rcvPin)  // function to return valid received baud rate
// Note that the serial monitor has no 600 baud option and 300 baud
// doesn't seem to work with version 22 hardware serial library
{
  long  rate = 10000, x = 2000;
  pinMode(rcvPin, INPUT);      // make sure serial in is a input pin
  digitalWrite (rcvPin, HIGH); // pull up enabled just for noise protection

  for (int i = 0; i < 5; i++) {
    x = pulseIn(rcvPin, LOW, 125000);  // measure the next zero bit width
    if (x < 1)continue;
    rate = x < rate ? x : rate;
  }
  if (DEBUG) {
    Serial.print(F("  detected pulse rate = "));
    Serial.println(rate);
  }
  return rate;
}

static void NTP_SYNC() {
  Serial.println("Contacting Time Server");
  configTime(3600 * timezone, daysavetime * 3600, "time.nist.gov", "0.pool.ntp.org", "1.pool.ntp.org");

}
static void WIFI_Connect() {

  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    digitalWrite(GREEN_LED_PIN, LOW);
    delay(50);
    digitalWrite(GREEN_LED_PIN, HIGH);
    delay(50);
    Serial.print(".");
  }
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());

}
String fillzero(int value) {
  if (value < 10) {
    return "0" + String(value);
  } else {
    return String(value);
  }
}
long autoBaud() {
  //try the various baud rates until one makes sense
  //should only output simple NMEA [$A-Z0-9*\r\c]
  //start with saved default
  Serial.print(F("   try autobaud .. "));
  long rate = detectRate(GPS_RX_PIN) + detectRate(GPS_RX_PIN) + detectRate(GPS_RX_PIN);
  rate = rate / 3l;
  long baud = 0;
  /*
     Time Baud Rate
    3333µs (3.3ms)300
    833µs   1200
    416µs   2400
    208µs   4800
    104µs   9600
    69µs  14400
    52µs  19200
    34µs  28800
    26µs  38400
    17.3µs  57600
    8µs   115200
    Megas min is about 10uS? so there may be some inaccuracy
  */
  if (rate < 12)
    baud = 115200;
  else if (rate < 20)
    baud = 57600;
  else if (rate < 30)
    baud = 38400;
  else if (rate < 40)
    baud = 28800;
  else if (rate < 60)
    baud = 19200;
  else if (rate < 80)
    baud = 14400;
  else if (rate < 150)
    baud = 9600;
  else if (rate < 300)
    baud = 4800;
  else if (rate < 600)
    baud = 2400;
  else if (rate < 1200)
    baud = 1200;
  else
    baud = 0;

  if (baud > 0) {
    Serial.print(F("OK at "));
    Serial.println(baud);

    return baud;
  } else {
    Serial.print(F("FAILED"));
  }


  if (DEBUG) Serial.println(F("   default to 4800.."));

  return 4800;
}
static void smartDelay(unsigned long ms)
{
  unsigned long start = millis();
  while (millis() - start < ms)
  {
    while (Serial2.available())
      gps.encode(Serial2.read());
    /*while (digitalRead(BUTTON_PIN) == LOW) {
      WIFI_Connect();
      NTP_SYNC();
      }*/
  }
}

void listDir(fs::FS &fs, const char * dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.print("  DIR : ");
      Serial.print (file.name());
      time_t t = file.getLastWrite();
      struct tm * tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, ( tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.print(file.size());
      time_t t = file.getLastWrite();
      struct tm * tmstruct = localtime(&t);
      Serial.printf("  LAST WRITE: %d-%02d-%02d %02d:%02d:%02d\n", (tmstruct->tm_year) + 1900, ( tmstruct->tm_mon) + 1, tmstruct->tm_mday, tmstruct->tm_hour , tmstruct->tm_min, tmstruct->tm_sec);
    }
    file = root.openNextFile();
  }
}

void createDir(fs::FS &fs, const char * path) {
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path)) {
    Serial.println("Dir created");
  } else {
    Serial.println("mkdir failed");
  }
}

void removeDir(fs::FS &fs, const char * path) {
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path)) {
    Serial.println("Dir removed");
  } else {
    Serial.println("rmdir failed");
  }
}

void readFile(fs::FS &fs, const char * path) {
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file) {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available()) {
    Serial.write(file.read());
  }
  file.close();
}

void writeFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message)) {
    Serial.println("File written");
  } else {
    Serial.println("Write failed");
  }
  file.close();
}

void appendFile(fs::FS &fs, const char * path, const char * message) {
  // Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message)) {
    // Serial.println("Message appended");
  } else {
    Serial.println("Append failed");
  }
  file.close();
}
void SmartAppendFile(fs::FS &fs, const char * path, const char * message) {
  Serial.printf("Smart Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file) {
    writeFile(fs, path, message);
  } else {
    appendFile(fs, path, message);
  }



  file.close();

}
void renameFile(fs::FS &fs, const char * path1, const char * path2) {
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2)) {
    Serial.println("File renamed");
  } else {
    Serial.println("Rename failed");
  }
}

void deleteFile(fs::FS &fs, const char * path) {
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path)) {
    Serial.println("File deleted");
  } else {
    Serial.println("Delete failed");
  }
}
void displayInfo()
{
  Serial.print(F("Location: "));
  if (gps.location.isValid()) {
    Serial.print(gps.location.lat(), 6);
    Serial.print(F(","));
    Serial.print(gps.location.lng(), 6);
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F("  Date/Time: "));
  if (gps.date.isValid()) {
    Serial.print(gps.date.month());
    Serial.print(F("/"));
    Serial.print(gps.date.day());
    Serial.print(F("/"));
    Serial.print(gps.date.year());
  } else {
    Serial.print(F("INVALID"));
  }

  Serial.print(F(" "));
  if (gps.time.isValid()) {
    if (gps.time.hour() < 10) Serial.print(F("0"));
    Serial.print(gps.time.hour());
    Serial.print(F(":"));
    if (gps.time.minute() < 10) Serial.print(F("0"));
    Serial.print(gps.time.minute());
    Serial.print(F(":"));
    if (gps.time.second() < 10) Serial.print(F("0"));
    Serial.print(gps.time.second());
    Serial.print(F("."));
    if (gps.time.centisecond() < 10) Serial.print(F("0"));
    Serial.print(gps.time.centisecond());
  } else {
    Serial.print(F("INVALID"));
  }
  Serial.println();
  /* Serial.print(",");
    Serial.print(event_uuid);
    Serial.print(",");
    Serial.println(getMACAddress());*/

  if (gps.location.isValid() && gps.date.isValid() && gps.time.isValid() ) {

    //Serial.println(gps.time.value());
    //Serial.println(gps.date.value());
    //    Serial.println(gps.location.value());
    digitalWrite(GREEN_LED_PIN, HIGH);

    String gps_date = fillzero(gps.date.year()) + "-" + fillzero(gps.date.month()) + "-" + fillzero(gps.date.day());
    String gps_time = fillzero(gps.time.hour()) + ":" + fillzero(gps.time.minute()) + ":" + fillzero(gps.time.second()) + "." + gps.time.centisecond();
    String gps_lat = String(gps.location.lat(), 6);
    String gps_lng = String(gps.location.lng(), 6);
    String gps_alt = String(gps.altitude.meters());
    String gps_data =  gps_date + "," + gps_time  + "," + gps_lat  + "," + gps_lng + "," + gps_alt;

    gps_data = gps_data + "\n";
    char charBuf[60];
    gps_data.toCharArray(charBuf, 60);
    Serial.println(charBuf);
    appendFile(SD, gps_logging_filename.c_str(), charBuf);
    digitalWrite(GREEN_LED_PIN, LOW);
    if (REST_SUPPORT) {
      StaticJsonDocument<256> doc;

      doc["latitude"] = gps_lat;
      doc["longitude"] = gps_lng;
      doc["altitude"] = String(gps.altitude.meters());
      doc["event_id"] = event_uuid;
      doc["device_id"] = getMACAddress();
      doc["gps_datetime"] = gps_date + "T" + gps_time + "Z";
      serializeJson(doc, JsonPayload);
      //Serial.print(JsonPayload);
      //Serial.println();
      WiFiClientSecure *client = new WiFiClientSecure;
      if (client) {
        client -> setCACert(rootCACertificate);

        HTTPClient https;

        //if (https.begin(REST_SERVER, 9000, "/gps_record/")) {
        if (https.begin(*client, REST_SERVER, 9000, "/gps_record/", true)) {
          https.addHeader("Content-Type", "application/json");
          https.addHeader("Connection", "Close");
          https.setAuthorization(authUsername, authPassword);
          int httpCode = https.POST(JsonPayload);
          Serial.println(httpCode);
          if (httpCode == HTTP_CODE_OK || httpCode == 201) {
            String payload = https.getString();
            Serial.println(payload);
          }
          else {
            Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
          }

          https.end();
        } else {
          Serial.printf("[HTTPS] Unable to connect\n");
        }

        delete client;
      }

    }
  }
  smartDelay(1000);
}
String getShortUUID() {
  byte uuidNumber[4];
  String hexstring = "";
  int uuid_size = sizeof(uuidNumber) - 1;
  for (int i = 0; i <= uuid_size; i++) {
    uuidNumber[i] = ESPTrueRandom.randomByte();
    if (uuidNumber[i] < 0x10) {
      hexstring += '0';
    }

    hexstring += String(uuidNumber[i], HEX);
  }
  return hexstring;
}
