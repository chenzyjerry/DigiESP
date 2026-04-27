#include "esp_camera.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// --- Pins ---
#define TFT_CS         D3
#define TFT_RST        D0
#define TFT_DC         D1
#define SD_CS          21   
#define BUTTON_PIN     D4
#define BUZZER_PIN     D5

// --- Camera Pins (XIAO ESP32-S3 Sense) ---
#define PWDN_GPIO_NUM     -1
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM     10
#define SIOD_GPIO_NUM     40
#define SIOC_GPIO_NUM     39
#define Y9_GPIO_NUM       48
#define Y8_GPIO_NUM       11
#define Y7_GPIO_NUM       12
#define Y6_GPIO_NUM       14
#define Y5_GPIO_NUM       16
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM       17
#define Y2_GPIO_NUM       15
#define VSYNC_GPIO_NUM    38
#define HREF_GPIO_NUM     47
#define PCLK_GPIO_NUM     13

Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// --- Sound Helpers ---
void playShutterSound() {
  tone(BUZZER_PIN, 2000, 50);
  delay(50);
  noTone(BUZZER_PIN);
}

void playSuccessSound() {
  tone(BUZZER_PIN, 1500, 100);
  delay(120);
  tone(BUZZER_PIN, 2200, 150);
}

// --- Camera Manager ---
// This function handles the "Hard Reset" required to change buffer sizes
bool startCamera(pixformat_t format, framesize_t size, int fb_count) {
  esp_camera_deinit(); // Kill the current driver instance
  delay(50);

  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 10000000; // 10MHz for stability
  config.pixel_format = format;
  config.frame_size = size;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = fb_count;
  config.grab_mode = CAMERA_GRAB_LATEST;

  if (format == PIXFORMAT_JPEG) config.jpeg_quality = 12;

  return (esp_camera_init(&config) == ESP_OK);
}

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  if (!SD.begin(SD_CS)) {
    tft.println("SD Fail");
  }

  // Start initial viewfinder
  if (!startCamera(PIXFORMAT_RGB565, FRAMESIZE_QQVGA, 2)) {
    tft.println("Cam Init Fail");
  }
}

void loop() {
  // 1. Viewfinder Mode
  digitalWrite(SD_CS, HIGH);
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    tft.startWrite();
    tft.setAddrWindow(0, 0, 160, 120); 
    SPI.writeBytes((uint8_t*)fb->buf, fb->len);
    tft.endWrite();
    esp_camera_fb_return(fb);
  }

  // 2. Capture Mode
  if (digitalRead(BUTTON_PIN) == LOW) {
    playShutterSound();
    
    // UI Feedback
    tft.fillScreen(ST7735_BLACK); // Shutter blackout
    tft.setCursor(10, 50);
    tft.setTextColor(ST7735_WHITE);
    tft.print("CAPTURING VGA...");

    // Hard switch to JPEG VGA
    if (startCamera(PIXFORMAT_JPEG, FRAMESIZE_VGA, 1)) {
      // Allow sensor to stabilize after re-init
      delay(500); 
      for(int i = 0; i < 5; i++) {
        camera_fb_t * dummy = esp_camera_fb_get();
        if (dummy) esp_camera_fb_return(dummy);
      }

      // Final Capture
      camera_fb_t * jpeg_fb = esp_camera_fb_get();
      if (jpeg_fb && jpeg_fb->buf[0] == 0xFF) {
        digitalWrite(TFT_CS, HIGH);
        digitalWrite(SD_CS, LOW);

        String path = "/IMG_" + String(millis()) + ".jpg";
        File file = SD.open(path.c_str(), FILE_WRITE);
        if (file) {
          file.write(jpeg_fb->buf, jpeg_fb->len);
          file.close();
          Serial.println("Saved: " + path);
          playSuccessSound();
        }
        digitalWrite(SD_CS, HIGH);
        esp_camera_fb_return(jpeg_fb);
      }
    }

    // Return to fast Viewfinder
    startCamera(PIXFORMAT_RGB565, FRAMESIZE_QQVGA, 2);
    tft.fillScreen(ST7735_BLACK);
    while(digitalRead(BUTTON_PIN) == LOW); 
  }
}