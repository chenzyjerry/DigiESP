#include "esp_camera.h"
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <Adafruit_ST7735.h>

// --- Pins (XIAO ESP32-S3 Sense) ---
#define TFT_CS         D3
#define TFT_RST        D0
#define TFT_DC         D1
#define SD_CS          21   
#define BUTTON_PIN     D4

// --- Camera Pins ---
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
bool cameraReady = false;

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  
  pinMode(TFT_CS, OUTPUT);
  pinMode(SD_CS, OUTPUT);
  digitalWrite(TFT_CS, HIGH);
  digitalWrite(SD_CS, HIGH);

  tft.initR(INITR_BLACKTAB);
  tft.setRotation(1);
  tft.fillScreen(ST7735_BLACK);

  if (!psramFound()) {
    tft.println("PSRAM Error");
    return;
  }

  if (!SD.begin(SD_CS)) {
    tft.println("SD Fail");
  }

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
  
  // CRITICAL: 8MHz is much safer for stable transitions on the S3
  config.xclk_freq_hz = 8000000; 
  
  config.frame_size = FRAMESIZE_QQVGA;
  config.pixel_format = PIXFORMAT_RGB565; 
  config.grab_mode = CAMERA_GRAB_WHEN_EMPTY; // More stable for slow writes
  config.fb_location = CAMERA_FB_IN_PSRAM;
  config.fb_count = 1; // Reducing to 1 buffer to prevent DMA/PSRAM contention during FB-OVF

  if (esp_camera_init(&config) == ESP_OK) {
    cameraReady = true;
  }
}

void loop() {
  if (!cameraReady) return;

  // Viewfinder
  digitalWrite(SD_CS, HIGH); 
  camera_fb_t * fb = esp_camera_fb_get();
  if (fb) {
    tft.startWrite();
    tft.setAddrWindow(0, 0, 160, 120); 
    SPI.writeBytes((uint8_t*)fb->buf, fb->len);
    tft.endWrite();
    esp_camera_fb_return(fb);
  }

  // Capture Trigger
  if (digitalRead(BUTTON_PIN) == LOW) {
    sensor_t * s = esp_camera_sensor_get();
    
    // 1. HARD TRANSITION
    s->set_pixformat(s, PIXFORMAT_JPEG);
    s->set_framesize(s, FRAMESIZE_VGA);
    s->set_quality(s, 15); // Higher number = smaller file = less likely to overflow
    
    tft.fillScreen(ST7735_BLUE);
    tft.setCursor(10, 50);
    tft.print("SYNCING...");

    // 2. AGGRESSIVE PURGE
    // Give the sensor a full half-second to stabilize the new signal
    delay(500); 
    for(int i = 0; i < 8; i++) {
      camera_fb_t * d = esp_camera_fb_get();
      if (d) esp_camera_fb_return(d);
      delay(50);
    }

    // 3. CAPTURE WITH HEADER VALIDATION
    camera_fb_t * jpeg_fb = esp_camera_fb_get();
    
    if (jpeg_fb) {
      // Check if it's a real JPEG (0xFFD8)
      if (jpeg_fb->buf[0] == 0xFF && jpeg_fb->buf[1] == 0xD8) {
        
        digitalWrite(TFT_CS, HIGH); 
        digitalWrite(SD_CS, LOW);   

        String path = "/IMG_" + String(millis()) + ".jpg";
        File file = SD.open(path.c_str(), FILE_WRITE);
        if (file) {
          file.write(jpeg_fb->buf, jpeg_fb->len);
          file.close();
          Serial.println("Saved Successfully!");
          tft.fillScreen(ST7735_GREEN);
          delay(500);
        }
        digitalWrite(SD_CS, HIGH);
      } else {
        Serial.println("Invalid Data - Skipping Save");
        tft.fillScreen(ST7735_RED);
        delay(500);
      }
      esp_camera_fb_return(jpeg_fb);
    }

    // REVERT
    s->set_pixformat(s, PIXFORMAT_RGB565);
    s->set_framesize(s, FRAMESIZE_QQVGA);
    tft.fillScreen(ST7735_BLACK);
    while(digitalRead(BUTTON_PIN) == LOW); 
  }
}