/* -------------------------------------------------
IMPORTANT!
This code works only till ESP32 core (boards) version 3.2.0.
Downgrade if you have compilation errors.

Copyright (c)
Arduino project by Tech Talkies YouTube Channel.
https://www.youtube.com/@techtalkies1
-------------------------------------------------*/

#include <BleMouse.h>
#include <Adafruit_MPU6050.h>

#define LEFTBUTTON 19
#define RIGHTBUTTON 18
#define SPEED 11

Adafruit_MPU6050 mpu;
BleMouse bleMouse;

bool sleepMPU = true;
unsigned long lastActivityTime = 0;
const unsigned long sleepTimeout = 1000; 
long mpuDelayMillis;

void setup() {
  Serial.begin(115200);

  pinMode(LEFTBUTTON, INPUT_PULLUP);
  pinMode(RIGHTBUTTON, INPUT_PULLUP);

  bleMouse.begin();

  delay(1000);
  // Try to initialize MPU!
  if (!mpu.begin()) {
    Serial.println("Failed to find MPU6050 chip");
    while (1) {
      delay(10);
    }
  }
  Serial.println("MPU6050 Found!");

  //Sleep MPU library till Bluetooth is connected as it seems to interrupt connection
  mpu.enableSleep(sleepMPU);
}

void loop() {
  if (bleMouse.isConnected()) {
    if (sleepMPU) {
      delay(3000);
      Serial.println("MPU6050 awakened!");
      sleepMPU = false;
      mpu.enableSleep(sleepMPU);
      delay(500);
    }

    sensors_event_t a, g, temp;
    mpu.getEvent(&a, &g, &temp);

    lastActivityTime = millis();

    Serial.println(g.gyro.x);
    Serial.println(g.gyro.z);

    // Depending on how you connected the MPU6050
    // You might need to swap these axeses or make them positive or negative
    if (abs(g.gyro.x) > 0.1 || abs(g.gyro.z) > 0.1) {
      bleMouse.move(g.gyro.z * -SPEED, g.gyro.x * +SPEED);
    }

    if (!digitalRead(LEFTBUTTON)) {
      Serial.println("Left click");
      bleMouse.click(MOUSE_LEFT);
      delay(500);
    }

    if (!digitalRead(RIGHTBUTTON)) {
      Serial.println("Right click");
      bleMouse.click(MOUSE_RIGHT);
      delay(500);
    }
  }
}