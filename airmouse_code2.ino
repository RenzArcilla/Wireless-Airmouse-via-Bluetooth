#include <BleMouse.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Wire.h>

// --- PIN DEFINITIONS ---
#define LEFTBUTTON 19
#define RIGHTBUTTON 18

// --- CONFIGURATION SETTINGS ---
#define SENSITIVITY 17      // Higher = faster cursor
#define DEADZONE 0.05       // Ignore tiny movements (tremors)
#define SLEEP_TIMEOUT 1000 // Time in ms before sleep (30 seconds)
#define SMOOTHING 0.8       // Factor for Low Pass Filter (0.1 = very smooth/slow, 1.0 = raw/fast)

Adafruit_MPU6050 mpu;
BleMouse bleMouse("ESP32 Air Mouse", "Maker", 100);

// --- GLOBAL VARIABLES ---
unsigned long lastActivityTime = 0;
float filteredX = 0;
float filteredZ = 0;
bool sleepMPU = true;

void goToSleep() {
  Serial.println("Inactivity detected. Going to Deep Sleep...");
  delay(100);
  
  // Configure wake up: When LEFTBUTTON (GPIO 19) goes LOW
  // Note: Only specific pins work for ext0 wakeup, 19 usually works on most ESP32s
  esp_sleep_enable_ext0_wakeup((gpio_num_t)LEFTBUTTON, 0); 
  
  // Power down the MPU6050
  mpu.enableSleep(true);
  
  // Enter Deep Sleep
  esp_deep_sleep_start();
}

void setup() {
  Serial.begin(115200);

  pinMode(LEFTBUTTON, INPUT_PULLUP);
  pinMode(RIGHTBUTTON, INPUT_PULLUP);

  bleMouse.begin();

  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }

  // Optimize MPU6050 settings for mouse use
  mpu.setAccelerometerRange(MPU6050_RANGE_2_G);
  mpu.setGyroRange(MPU6050_RANGE_250_DEG);
  mpu.setFilterBandwidth(MPU6050_BAND_21_HZ); // Hardware noise reduction

  lastActivityTime = millis();
  Serial.println("Air Mouse Ready!");
}

void loop() {
  if (bleMouse.isConnected()) {
    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    // 1. SMOOTHING (Low Pass Filter)
    // Helps remove the "shaking" effect from your hand
    filteredX = (g.gyro.x * SMOOTHING) + (filteredX * (1.0 - SMOOTHING));
    filteredZ = (g.gyro.z * SMOOTHING) + (filteredZ * (1.0 - SMOOTHING));

    // 2. MOVEMENT LOGIC
    if (abs(filteredX) > DEADZONE || abs(filteredZ) > DEADZONE) {
      // Calculate move amounts
      int moveX = (int)(filteredZ * -SENSITIVITY);
      int moveY = (int)(filteredX * SENSITIVITY);
      
      bleMouse.move(moveX, moveY);
      lastActivityTime = millis(); // Reset sleep timer because we moved
    }

    // 3. LEFT BUTTON (Click & Drag / Long Press)
    if (digitalRead(LEFTBUTTON) == LOW) {
      if (!bleMouse.isPressed(MOUSE_LEFT)) {
        bleMouse.press(MOUSE_LEFT);
        Serial.println("Left Button Pressed");
      }
      lastActivityTime = millis(); // Reset sleep timer
    } else {
      if (bleMouse.isPressed(MOUSE_LEFT)) {
        bleMouse.release(MOUSE_LEFT);
        Serial.println("Left Button Released");
      }
    }

    // 4. RIGHT BUTTON (Click & Drag)
    if (digitalRead(RIGHTBUTTON) == LOW) {
      if (!bleMouse.isPressed(MOUSE_RIGHT)) {
        bleMouse.press(MOUSE_RIGHT);
        Serial.println("Right Button Pressed");
      }
      lastActivityTime = millis(); // Reset sleep timer
    } else {
      if (bleMouse.isPressed(MOUSE_RIGHT)) {
        bleMouse.release(MOUSE_RIGHT);
        Serial.println("Right Button Released");
      }
    }

    // 5. AUTO SLEEP
    // If no movement or clicks for SLEEP_TIMEOUT duration
    if (millis() - lastActivityTime > SLEEP_TIMEOUT) {
      goToSleep();
    }

    delay(10); // Stability delay
  }
}