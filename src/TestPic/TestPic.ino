#include "esp_camera.h"
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>
#include <SPI.h>

// TFT Pins
#define TFT_CS     D3
#define TFT_RST    D0
#define TFT_DC     D1

// Camera Pin Mapping (XIAO ESP32-S3 Sense)
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

void setup() {
  Serial.begin(115200);
  
  // 1. Initialize Display
  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1); // Landscape
  tft.fillScreen(ST7735_BLACK);
  
  // 2. Camera Configuration
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
  config.xclk_freq_hz = 20000000;
  config.frame_size = FRAMESIZE_QQVGA; // 160x120 (Fits nicely on 128x160 screen)
  config.pixel_format = PIXFORMAT_RGB565; // Matches TFT format
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = 1;

  if (esp_camera_init(&config) != ESP_OK) {
    tft.print("Camera Init Failed!");
    return;
  }
}

void loop() {
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) return;

  // We need to swap the bytes because ESP32 is Little-Endian 
  // and the TFT is Big-Endian.
  uint16_t *ptr = (uint16_t *)fb->buf;
  int pixel_count = fb->width * fb->height;

  for (int i = 0; i < pixel_count; i++) {
    // This flips the 8-bit halves of the 16-bit color
    ptr[i] = (ptr[i] << 8) | (ptr[i] >> 8);
  }

  // Now draw the "fixed" buffer
  tft.drawRGBBitmap(0, 0, ptr, fb->width, fb->height);

  esp_camera_fb_return(fb);
  delay(10); 
}