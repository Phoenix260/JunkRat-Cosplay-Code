#include <esp32-hal-ledc.h> // Mostly used For motor manipulation (yes, I know that's not the original purpose)
#include <BLEDevice.h> //For Bluetooth
#include <BLEUtils.h>  //For Bluetooth
#include <BLEServer.h> //For Bluetooth

#define SERVICE_UUID        "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"

#define RedLED_pin      33 // analog_1_in_out - NOTE BLE does not allow Analog_2
#define RedLED_Chan     1 // Analog Channel to mimic AnalogWrite on ESP32
#define BlueLED_pin     32 // analog_1_in_out - NOTE BLE does not allow Analog_2

#define TopToggle_pin   12

#define Vert_High       17
#define Vert_sense      37 // analog_1_in - NOTE BLE does not allow Analog_2
#define Vert_Low        2

#define Horr_Left       27
#define Horr_sense      36 // analog_1_in - NOTE BLE does not allow Analog_2
#define Horr_Right      25

BLEServer* pServer = NULL;
BLECharacteristic* pCharacteristic = NULL;

void setupBLE(){
  Serial.println("Starting BLE work!");
  BLEDevice::init("DropBOB RipTire Remote");
  
  pServer = BLEDevice::createServer();
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                               CHARACTERISTIC_UUID,
                               BLECharacteristic::PROPERTY_READ |
                               BLECharacteristic::PROPERTY_WRITE
                             );
  pCharacteristic->setValue("Hello World says DropBOB RipTire!");
  pService->start();
  
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  
  BLEDevice::startAdvertising();
  Serial.println("Characteristic defined! Now you can read it in your phone!");
}

void setup() {
  Serial.begin(9600);
  
  pinMode(RedLED_pin,OUTPUT); digitalWrite(RedLED_pin,HIGH); delay(1000); digitalWrite(RedLED_pin,LOW);
  ledcSetup(RedLED_Chan, 12000, 12);            // 12 kHz PWM, 12-bit resolution, 0-4095 duty cycle
  ledcAttachPin(RedLED_pin, RedLED_Chan);
  
  pinMode(BlueLED_pin,OUTPUT); digitalWrite(BlueLED_pin,HIGH);
  
  pinMode(Vert_High,OUTPUT); digitalWrite(Vert_High,HIGH);
  pinMode(Vert_Low,OUTPUT); digitalWrite(Vert_Low,LOW);
  pinMode(Vert_sense,INPUT);

  pinMode(Horr_Right,OUTPUT); digitalWrite(Horr_Right,HIGH);
  pinMode(Horr_Left,OUTPUT); digitalWrite(Horr_Left,LOW);
  pinMode(Horr_sense,INPUT);

  pinMode(TopToggle_pin,INPUT_PULLUP);
  
  setupBLE();
}

void loop() {

  //Main loop is focused on Reading Bluetooth and tracking other things (no delay)

  static long last_check = millis();
  if(!connectToServer() && millis()-last_check > 10000){
    last_check = millis();
    Serial.println();Serial.println("================ Retrying Connection ==================");
    SetupBLE();
  }

  //Local Variables (declare them once and zero them)
  static int8_t Vert_Value = 0; //(Values of -256 to +256 allowable, Values of -100 to +100 expected to use
  static int8_t Horr_Value = 0; //(Values of -256 to +256 allowable, Values of -100 to +100 expected to use
  static bool TopToggle_Value = 0; // On or Off
  static uint16_t Red_LED_Value = 0; //(Values of 0 to +65536 allowable, Values expected for RED_LED = 0-4095)

  //Read sensors
  Vert_Value = map(analogRead(Vert_sense),0,4095,-100,+100); if (abs(Vert_Value) < 8) Vert_Value = 0;
  Horr_Value = map(analogRead(Horr_sense),0,4095,-100,+100); if (abs(Horr_Value) < 8) Horr_Value = 0;
  TopToggle_Value = !digitalRead(TopToggle_pin); //pin is pulled high, so digital read off is high, when on is low.

  //Print Serial Logs
  Serial.println("Vert_sense: " + String(Vert_Value) + "%" + 
                 "    Horr_sense: " + String(Horr_Value) + "%"  +  
                 "    Toggle: " + String(TopToggle_Value));

  //Change LED brightness depending on Combined Horrizontal & Vertical tilt of the Joystick
  if(abs(Vert_Value) > abs(Horr_Value)) Red_LED_Value = abs(Vert_Value)/100.0*4095;
  else Red_LED_Value = abs(Horr_Value)/100.0*4095;
  ledcWrite(RedLED_Chan, Red_LED_Value); //0 to 4095 duty

  //Control Tire Speed with Joystick
  pCharacteristic->setValue(String(Vert_Value).c_str());
  pCharacteristic->notify();
  delay(3);// bluetooth stack will go into congestion, if too many packets are sent, in 6 hours test i was able to go as low as 3ms
}
