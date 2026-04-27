#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// Your Pinout
#define TFT_CS    D3  
#define TFT_RST   D0  
#define TFT_DC    D1  
#define BUZZER_PIN D5 

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Musical Notes
#define NOTE_C5  523
#define NOTE_E5  659
#define NOTE_G5  784
#define NOTE_C6  1047
#define NOTE_G6  1568

// Jingle Structure: {Frequency, Duration}
struct Note {
  int freq;
  int duration;
};

Note startupJingle[] = {
  {NOTE_C5, 100}, {NOTE_E5, 100}, {NOTE_G5, 100}, {NOTE_C6, 150}, {NOTE_G6, 300}
};

void setup() {
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);
  
  runSplashWithJingle();
}

void loop() {
  // Stay in loop or reset for testing
}

void runSplashWithJingle() {
  const char* text = "DigiESP";
  int startX = (tft.width() - (strlen(text) * 12)) / 2;
  int startY = tft.height() / 2 - 8;

  // ANIMATION & JINGLE SYNC
  for (int i = 0; i < 5; i++) {
    // Play the note
    tone(BUZZER_PIN, startupJingle[i].freq, startupJingle[i].duration);
    
    // Draw text progressively brighter with each note
    uint16_t brightness = (i + 1) * 6; // Ramp up to 31
    uint16_t color = (brightness << 11) | ((brightness * 2) << 5) | brightness;
    
    tft.setCursor(startX, startY);
    tft.setTextColor(color);
    tft.setTextSize(2);
    tft.print(text);
    
    // Wait for the note to finish before next one
    delay(startupJingle[i].duration + 20); 
  }

  // Final flourish: underline at full white
  for (int w = 0; w < (int)strlen(text) * 12; w += 4) {
    tft.drawFastHLine(startX, startY + 18, w, ST7735_WHITE);
    delay(10);
  }

  delay(2000); 
}