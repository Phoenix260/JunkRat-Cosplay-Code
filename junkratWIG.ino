#include "SoftPWM.h"
#define LEDpin 13
static const uint8_t pins[] = {2, 3, 9, 10, 11, A2, A3, A4, A5};

void setup() {
  pinMode(LEDpin,OUTPUT);
  SoftPWMBegin(); // Initialize Software PWM for LEDs
  
  for(int i=0;i<9;i++){
    pinMode(pins[i],OUTPUT);
    SoftPWMSet(pins[i], 0); // Create and set pin XX to value (0=off, 100=ON)
    SoftPWMSetFadeTime(pins[i], 150, 400); //fade time for pin: X2 ms fade-up, X3 ms fade-down
  }
}

void loop() {
  static uint8_t fire[9];
  static uint8_t timer[9];
  static long delay_time_check[9];
  static bool checked[9];
  static float j=0.0;

  for(int i=0;i<9;i++){
    if(checked[i]){
      fire[i] = random(0,100);    // 0-100% LED brightness
      timer[i] = random(10,1000);  // 10ms to xx00 ms delay on fire value
    }
    if(millis() - delay_time_check[i] > timer[i]){
      SoftPWMSetPercent(pins[i], fire[i]);
      delay_time_check[i] = millis();
      checked[i] = true;
    }
  }
  digitalWrite(LEDpin,(int)j);
  j=j+0.01;if(j>2)j=0;
}
