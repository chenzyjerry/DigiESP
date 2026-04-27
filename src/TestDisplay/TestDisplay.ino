#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library for ST7735
#include <SPI.h>

// Pin definitions based on the table above
#define TFT_CS     D3
#define TFT_RST    D0 
#define TFT_DC     D1

// Initialize Adafruit ST7735
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

void setup() {
  // Use INITR_BLACKTAB for most 1.8" ST7735 displays
  // If colors look weird, try INITR_GREENTAB or INITR_REDTAB
  tft.initR(INITR_BLACKTAB); 
  tft.fillScreen(ST7735_BLACK);
  tft.setCursor(0, 0);
  tft.setTextColor(ST7735_WHITE);
  tft.setTextSize(1);
  tft.print("Hello Dad!");
}

void loop() {
  // Your code here
}