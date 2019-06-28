/*
#define ILI9341_WHITE       0x0000      
#define ILI9341_NAVY        0x000F      
#define ILI9341_DARKGREEN   0x03E0      
#define ILI9341_DARKCYAN    0x03EF      
#define ILI9341_MAROON      0x7800      
#define ILI9341_PURPLE      0x780F      
#define ILI9341_OLIVE       0x7BE0      
#define ILI9341_LIGHTGREY   0xC618       
#define ILI9341_DARKGREY    0x7BEF      
#define ILI9341_BLUE        0x001F      
#define ILI9341_GREEN       0x07E0       
#define ILI9341_CYAN        0x07FF      
#define ILI9341_RED         0xF800      
#define ILI9341_MAGENTA     0xF81F      
#define ILI9341_YELLOW      0xFFE0     
#define ILI9341_WHITE       0xFFFF      
#define ILI9341_ORANGE      0xFD20     
#define ILI9341_GREENYELLOW 0xAFE5     
#define ILI9341_PINK        0xF81F
*/


#include <ESP8266WiFi.h>
#include <Ticker.h>
#include <JsonListener.h>
#include "Adafruit_GFX.h"
#include "Adafruit_ILI9341.h"
#include "Wire.h"
#include "WundergroundClient.h"
//#include "WeatherStationFonts.h"
//#include "WeatherStationImages.h"
#include "TimeClient.h"
#include <SPI.h>
#include <SD.h>

#include "Fonts/FreeSansBold12pt7b.h"
#include "Fonts/FreeSansBold24pt7b.h"

int BACKGROUND  =   ((39 / 8) << 11) | ((42 / 4) << 5) | (51 / 8);

// WIFI
const char* WIFI_SSID = "ssid";
const char* WIFI_PWD = "pw";

// Setup
const int UPDATE_INTERVAL_SECS = 10 * 60; // Update every 10 minutes

// TimeClient settings
const float UTC_OFFSET = 1;

// Wunderground Settings
const boolean IS_METRIC = true;
const String WUNDERGRROUND_API_KEY = "apikey";
const String WUNDERGRROUND_LANGUAGE = "EN";
const String WUNDERGROUND_COUNTRY = "UK";
const String WUNDERGROUND_CITY = "London";

#// For the Adafruit shield, these are the default.
#define SD_CS  D3     // Chip Select for SD-Card
#define TFT_DC D4     // Data/Command pin for SPI TFT screen
#define TFT_CS D8     // Chip select for TFT screen

// Use hardware SPI (on Uno, #13, #12, #11) and the above for CS/DC
Adafruit_ILI9341 tft = Adafruit_ILI9341(TFT_CS, TFT_DC);

#define BUFFPIXEL 20    // Used for BMP file display

TimeClient timeClient(UTC_OFFSET);

// Set to false, if you prefere imperial/inches, Fahrenheit
WundergroundClient wunderground(IS_METRIC);

// flag changed in the ticker function every 10 minutes
bool readyForWeatherUpdate = false;

String lastUpdate = "--";

Ticker ticker;

void setup() {
  Serial.begin(115200);
  Serial.println();
  Serial.println();

  tft.begin();
  tft.setRotation(3);
    tft.setTextColor(ILI9341_BLACK);
  tft.fillScreen(ILI9341_GREEN);
  yield();
  //tft.flipScreenVertically();
  tft.setFont(&FreeSansBold12pt7b);
 
  WiFi.begin(WIFI_SSID, WIFI_PWD);

  int counter = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    tft.setCursor(42, 120);
    tft.print("Connecting to WiFi");

    counter++;
   
     initSD(); delay(250);
  }

  Serial.println("");

  ticker.attach(UPDATE_INTERVAL_SECS, setReadyForWeatherUpdate);
  updateData(&tft);
  
}

void initSD() {    
   Serial.print("Initializing SD card...");
   if (!SD.begin(SD_CS)) {
        Serial.println("failed!");
        delay(100);
        return;
   }
   Serial.println("OK!"); 
   delay(100);

} 

void updateData(Adafruit_ILI9341 *tft) {

  timeClient.updateTime();

  wunderground.updateConditions(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);

  wunderground.updateForecast(WUNDERGRROUND_API_KEY, WUNDERGRROUND_LANGUAGE, WUNDERGROUND_COUNTRY, WUNDERGROUND_CITY);
  lastUpdate = timeClient.getFormattedTime();
  readyForWeatherUpdate = false;

  delay(1000);
}

void drawDateTime() {
   tft.setTextColor(ILI9341_WHITE);
  tft.fillScreen(BACKGROUND);
  tft.setFont(&FreeSansBold12pt7b);
  String date = wunderground.getDate();

  tft.setCursor(64, 32);
  tft.print(date);
  tft.setFont(&FreeSansBold24pt7b);
  String time = timeClient.getFormattedTime().substring(0, 5);
  //String time = timeClient.getHours() + ":" + timeClient.getHours();
  tft.setCursor(70, 120);
  tft.print(time);
}

void drawCurrentWeather() {
     tft.setTextColor(ILI9341_WHITE);
  tft.fillScreen(BACKGROUND);
  tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(60, 40);
  tft.print(wunderground.getWeatherText());
  tft.setFont(&FreeSansBold24pt7b);
  String temp = wunderground.getCurrentTemp() + " ºC";
  tft.setCursor(60, 120);
  tft.print(temp);

  tft.setFont(&FreeSansBold24pt7b);
  String weatherIcon = wunderground.getTodayIcon();
  Serial.println(weatherIcon);
  drawIcon(weatherIcon, 32, 120);
}


void drawForecast() {
     tft.setTextColor(ILI9341_WHITE);
  tft.fillScreen(BACKGROUND);
  drawForecastDetails(0, 60, 0);
  drawForecastDetails(100, 60, 2);
  drawForecastDetails(200, 60, 4);
}

void drawForecastDetails(int x, int y, int dayIndex) {

  tft.setFont(&FreeSansBold12pt7b);
  String day = wunderground.getForecastTitle(dayIndex).substring(0, 3);
  day.toUpperCase();
  tft.setCursor(x + 20, y);
  tft.print(day);

  tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(x + 20, y + 30);
  tft.print(wunderground.getForecastIcon(dayIndex));

  tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(x + 20, y + 60);
  tft.print(wunderground.getForecastLowTemp(dayIndex) + "|" + wunderground.getForecastHighTemp(dayIndex));
}

void drawHeaderOverlay() {
  tft.setTextColor(ILI9341_WHITE);
  tft.setFont(&FreeSansBold12pt7b);
  String time = timeClient.getFormattedTime().substring(0, 5);
  tft.setCursor(0, 54);
  tft.print(time);

  String temp = wunderground.getCurrentTemp() + "°C";
  tft.setCursor(128, 54);
  tft.print(temp);
  tft.drawLine(0, 52, 128, 52, ILI9341_WHITE);
}

void setReadyForWeatherUpdate() {
  Serial.println("Setting readyForUpdate to true");
  readyForWeatherUpdate = true;
  updateData(&tft);
}

//----------------------------------------------------------------------------------------------------
// These read 16 and 32-bit data types from the SD card file.
// BMP data is stored little-endian, ESP8266 or Arduino are little-endian too.

uint16_t read16(File & f) {
  uint16_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read(); // MSB
  return result;
}

//----------------------------------------------------------------------------------------------------
uint32_t read32(File & f) {
  uint32_t result;
  ((uint8_t *)&result)[0] = f.read(); // LSB
  ((uint8_t *)&result)[1] = f.read();
  ((uint8_t *)&result)[2] = f.read();
  ((uint8_t *)&result)[3] = f.read(); // MSB
  return result;
}

//----------------------------------------------------------------------------------------------------
void bmpDraw(char *filename, uint8_t x, uint16_t y) {

  File     bmpFile;
  int      bmpWidth, bmpHeight;   // W+H in pixels
  uint8_t  bmpDepth;              // Bit depth (currently must be 24)
  uint32_t bmpImageoffset;        // Start of image data in file
  uint32_t rowSize;               // Not always = bmpWidth; may have padding
  uint8_t  sdbuffer[3*BUFFPIXEL]; // pixel buffer (R+G+B per pixel)
  uint8_t  buffidx = sizeof(sdbuffer); // Current position in sdbuffer
  boolean  goodBmp = false;       // Set to true on valid header parse
  boolean  flip    = true;        // BMP is stored bottom-to-top
  int      w, h, row, col;
  uint8_t  r, g, b;
  uint32_t pos = 0, startTime = millis();

  if((x >= tft.width()) || (y >= tft.height())) return;

  Serial.println();
  Serial.print(F("Loading image '"));
  Serial.print(filename);
  Serial.println('\'');

  // Open requested file on SD card
  if ((bmpFile = SD.open(filename)) == NULL) {
    Serial.println(F("File not found"));
    return;
  }

  // Parse BMP header
  if(read16(bmpFile) == 0x4D42) { // BMP signature
    Serial.print(F("File size: ")); Serial.println(read32(bmpFile));
    (void)read32(bmpFile); // Read & ignore creator bytes
    bmpImageoffset = read32(bmpFile); // Start of image data
    Serial.print(F("Image Offset: ")); Serial.println(bmpImageoffset, DEC);
    // Read DIB header
    Serial.print(F("Header size: ")); Serial.println(read32(bmpFile));
    bmpWidth  = read32(bmpFile);
    bmpHeight = read32(bmpFile);
    if(read16(bmpFile) == 1) { // # planes -- must be '1'
      bmpDepth = read16(bmpFile); // bits per pixel
      Serial.print(F("Bit Depth: ")); Serial.println(bmpDepth);
      if((bmpDepth == 24) && (read32(bmpFile) == 0)) { // 0 = uncompressed

        goodBmp = true; // Supported BMP format -- proceed!
        Serial.print(F("Image size: "));
        Serial.print(bmpWidth);
        Serial.print('x');
        Serial.println(bmpHeight);

        // BMP rows are padded (if needed) to 4-byte boundary
        rowSize = (bmpWidth * 3 + 3) & ~3;

        // If bmpHeight is negative, image is in top-down order.
        // This is not canon but has been observed in the wild.
        if(bmpHeight < 0) {
          bmpHeight = -bmpHeight;
          flip      = false;
        }

        // Crop area to be loaded
        w = bmpWidth;
        h = bmpHeight;
        if((x+w-1) >= tft.width())  w = tft.width()  - x;
        if((y+h-1) >= tft.height()) h = tft.height() - y;

        // Set TFT address window to clipped image bounds
        tft.setAddrWindow(x, y, x+w-1, y+h-1);

        for (row=0; row<h; row++) { // For each scanline...

          // Seek to start of scan line.  It might seem labor-
          // intensive to be doing this on every line, but this
          // method covers a lot of gritty details like cropping
          // and scanline padding.  Also, the seek only takes
          // place if the file position actually needs to change
          // (avoids a lot of cluster math in SD library).
          if(flip) // Bitmap is stored bottom-to-top order (normal BMP)
            pos = bmpImageoffset + (bmpHeight - 1 - row) * rowSize;
          else     // Bitmap is stored top-to-bottom
            pos = bmpImageoffset + row * rowSize;
          if(bmpFile.position() != pos) { // Need seek?
            bmpFile.seek(pos);
            buffidx = sizeof(sdbuffer); // Force buffer reload
          }

          for (col=0; col<w; col++) { // For each pixel...
            // Time to read more pixel data?
            if (buffidx >= sizeof(sdbuffer)) { // Indeed
              bmpFile.read(sdbuffer, sizeof(sdbuffer));
              buffidx = 0; // Set index to beginning
            }

            // Convert pixel from BMP to TFT format, push to display
            b = sdbuffer[buffidx++];
            g = sdbuffer[buffidx++];
            r = sdbuffer[buffidx++];
            tft.pushColor(tft.color565(r,g,b));
          } // end pixel
        } // end scanline
        Serial.print(F("Loaded in "));
        Serial.print(millis() - startTime);
        Serial.println(" ms");
      } // end goodBmp
    }
  }

  bmpFile.close();
  if(!goodBmp) Serial.println(F("BMP format not recognized."));
}


void drawIcon(String Icon, int x, int y) {
  if (Icon=="B") bmpDraw("clear.bmp", x, y);
  if (Icon=="M") bmpDraw("fog.bmp", x, y);
  if (Icon=="E") bmpDraw("fog.bmp", x, y);
}

void loop() {


  drawDateTime(); delay(5000);
  drawCurrentWeather(); delay(5000);
  drawForecast(); delay(5000);

}
