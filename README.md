# smart_gps_tracker-esp32_spi_sdcard
## Assembled picture
![image](https://user-images.githubusercontent.com/7022841/200153958-cd9e775e-ebb5-474f-ba50-3310228cacdf.png)

## Buy list
1. TTGO T1 ESP-32 V1.0 Rev1
2. GT-U8 GPS module
3. <=32 GB SDHC Micro SD card
4. Lithium polymer battery

## Software tool included
1. csv2kml.py #convert gps data CSV file to KML

## Usage
1. Open Arduino, compile sketch, upload to ESP chip
2. Go outside, when the GPS positioning light blinks, it will start to record the coordinates

## Result
1. Files in SDcard
![image](https://user-images.githubusercontent.com/7022841/200154198-dc9d215d-371b-473d-9ed6-412519f1c2e0.png)
2. Content of CSV file
![image](https://user-images.githubusercontent.com/7022841/200154239-f586d396-4909-4b16-9a03-bfc41929b021.png)

3. Open KML file in Google Earth
![image](https://user-images.githubusercontent.com/7022841/200154227-06fcb245-a86d-465f-af21-27473c6b477d.png)
