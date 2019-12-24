#include "BLEDevice.h"

// The remote service we wish to connect to.
static BLEUUID serviceUUID("4fafc201-1fb5-459e-8fcc-c5c9c331914b");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("beb5483e-36e1-4688-b7f5-ea07361b26a8");

#define stepPin   2   //digital
#define dirPin    15  //digital

#define M1  12        //digital
#define M2  14        //digital
#define M3  27        //digital

#define Motor_One_Power     22    //digital
#define Motor_Two_Power     19    //digital
#define Motor_Three_Power   23    //digital

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

int JoyStick_Value = 0;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic,
  uint8_t* pData,
  size_t length,
  bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    Serial.println("onDisconnect");
  }
};

bool connectToServer() {
    Serial.print("Forming a connection to ");
    Serial.println(myDevice->getAddress().toString().c_str());
    
    BLEClient*  pClient  = BLEDevice::createClient();
    Serial.println(" - Created client");

    pClient->setClientCallbacks(new MyClientCallback());

    // Connect to the remove BLE Server.
    pClient->connect(myDevice);  // if you pass BLEAdvertisedDevice instead of address, it will be recognized type of peer device address (public or private)
    Serial.println(" - Connected to server");

    // Obtain a reference to the service we are after in the remote BLE server.
    BLERemoteService* pRemoteService = pClient->getService(serviceUUID);
    if (pRemoteService == nullptr) {
      Serial.print("Failed to find our service UUID: ");
      Serial.println(serviceUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our service");


    // Obtain a reference to the characteristic in the service of the remote BLE server.
    pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
    if (pRemoteCharacteristic == nullptr) {
      Serial.print("Failed to find our characteristic UUID: ");
      Serial.println(charUUID.toString().c_str());
      pClient->disconnect();
      return false;
    }
    Serial.println(" - Found our characteristic");

    // Read the value of the characteristic.
    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }

    if(pRemoteCharacteristic->canNotify())
      pRemoteCharacteristic->registerForNotify(notifyCallback);

    connected = true;
}
/**
 * Scan for BLE servers and find the first one that advertises the service we are looking for.
 */
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
 /**
   * Called for each advertising BLE server.
   */
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    Serial.print("BLE Advertised Device found: ");
    Serial.println(advertisedDevice.toString().c_str());

    // We have found a device, let us now see if it contains the service we are looking for.
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {

      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
      doScan = true;

    } // Found our server
  } // onResult
}; // MyAdvertisedDeviceCallbacks

void SetupBLE(){
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("DropBOB RipTire Wheel");
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
}
void setup() {
  Serial.begin(9600);
  Serial.println("Starting DropBOB RipTire Tire...");
  
  // Sets the two pins as Outputs
  pinMode(stepPin,OUTPUT); 
  pinMode(dirPin,OUTPUT);
  
  pinMode(M1,OUTPUT); digitalWrite(M1,HIGH);
  pinMode(M2,OUTPUT); digitalWrite(M2,HIGH);
  pinMode(M3,OUTPUT); digitalWrite(M3,HIGH);
  
  pinMode(Motor_One_Power,OUTPUT); digitalWrite(Motor_One_Power,LOW);
  pinMode(Motor_Two_Power,OUTPUT); digitalWrite(Motor_Two_Power,LOW);
  pinMode(Motor_Three_Power,OUTPUT); digitalWrite(Motor_Three_Power,LOW);

  SetupBLE();

  xTaskCreatePinnedToCore(
  Stepper_Task
  ,  "Stepper_Task"   // A name just for humans
  ,  32768  // This stack size can be checked & adjusted by reading the Stack Highwater (360-380 typ)
  ,  NULL
  ,  1  // Priority, with 3 (config MAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  ,  NULL 
  ,  1);//*/ CPU 0 or 1 -- HTTP OTA Update stuff must be on the core #1 - Core 0 crashes the watchdogs

  xTaskCreatePinnedToCore(
  TicToc_Task
  ,  "TicToc_Task"   // A name just for humans
  ,  32768  // This stack size can be checked & adjusted by reading the Stack Highwater (360-380 typ)
  ,  NULL
  ,  2  // Priority, with 3 (config MAX_PRIORITIES - 1) being the highest, and 0 being the lowest.
  ,  NULL 
  ,  1);//*/ CPU 0 or 1 -- HTTP OTA Update stuff must be on the core #1 - Core 0 crashes the watchdogs

  
}

void TicToc_Task(void *pvParameters){
  (void) pvParameters;
  for (;;){ // A Task shall never return or exit.  
    delay(1); //feed the watchdog (min 1 ms) 

    //========================== Wheel
    //This is what makes the Stepper run, the tick, tick ... This does nothing if the controller power is off
    //Currently controlling only the Step resolution and the Controller Power
    //Future plans to make the control smoother is to modify the delay between Tic/Toc by the joystick
    digitalWrite(stepPin,HIGH);
    delayMicroseconds(500); 
    digitalWrite(stepPin,LOW); 
    delayMicroseconds(500); 
    //========================== Wheel
    
    // This is just to check on the stack size (comment out when done)
    static long CallOncePer1Min = millis();
    if(millis()-CallOncePer1Min>1*60*1000){
      UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL ); 
      Serial.println();
      Serial.println( "************************ TicToc_Task Memory: " + String(uxHighWaterMark) + " "+ String(esp_get_free_heap_size()));
      Serial.println();
      CallOncePer1Min = millis();
    }//*/
    
  }
}
void Stepper_Task(void *pvParameters){
  (void) pvParameters;
  for (;;){ // A Task shall never return or exit.
    delay(1); //feed the watchdog (min 1 ms) 

    if(JoyStick_Value == 0){
      //Serial.println("Motors Off");
      digitalWrite(Motor_One_Power,LOW);
      digitalWrite(Motor_Two_Power,LOW);
      digitalWrite(Motor_Three_Power,LOW);
    }
    else if(0 < abs(JoyStick_Value) && abs(JoyStick_Value) <= 30){
      //Serial.println("1/16 Step");
      digitalWrite(Motor_One_Power,HIGH);
      digitalWrite(Motor_Two_Power,HIGH);
      digitalWrite(Motor_Three_Power,HIGH);
      digitalWrite(M1,HIGH);
      digitalWrite(M2,HIGH);
      digitalWrite(M3,HIGH);
    }
    else if(30 < abs(JoyStick_Value) && abs(JoyStick_Value) <= 60){
      //Serial.println("1/8 Step");
      digitalWrite(Motor_One_Power,HIGH);
      digitalWrite(Motor_Two_Power,HIGH);
      digitalWrite(Motor_Three_Power,HIGH);
      digitalWrite(M1,HIGH);
      digitalWrite(M2,HIGH);
      digitalWrite(M3,LOW);
    }
    else if(60 < abs(JoyStick_Value) && abs(JoyStick_Value) <= 80){
      //Serial.println("1/4 Step");
      digitalWrite(Motor_One_Power,HIGH);
      digitalWrite(Motor_Two_Power,HIGH);
      digitalWrite(Motor_Three_Power,HIGH);
      digitalWrite(M1,LOW);
      digitalWrite(M2,HIGH);
      digitalWrite(M3,LOW);
    }
    else if(80 < abs(JoyStick_Value) && abs(JoyStick_Value) <= 90){
      //Serial.println("1/2 Step");
      digitalWrite(Motor_One_Power,HIGH);
      digitalWrite(Motor_Two_Power,HIGH);
      digitalWrite(Motor_Three_Power,HIGH);
      digitalWrite(M1,HIGH);
      digitalWrite(M2,LOW);
      digitalWrite(M3,LOW);
    }
    else if(90 < abs(JoyStick_Value) && abs(JoyStick_Value) <= 100){
      //Serial.println("FULL Step");
      digitalWrite(Motor_One_Power,HIGH);
      digitalWrite(Motor_Two_Power,HIGH);
      digitalWrite(Motor_Three_Power,HIGH);
      digitalWrite(M1,LOW);
      digitalWrite(M2,LOW);
      digitalWrite(M3,LOW);
    }
    else{
      Serial.println("ERROR: Joystick Value out of Range");
    }
    
    if(JoyStick_Value>0){
      //Serial.println("Forward -->");
      digitalWrite(dirPin,HIGH); // Enables the motor to move in a particular direction
    }
    else if (JoyStick_Value<0){
      //Serial.println("<-- Reverse");
      digitalWrite(dirPin,LOW); // Enables the motor to move in a particular direction
    }
    
    // This is just to check on the stack size (comment out when done)
    static long CallOncePer1Min = millis();
    if(millis()-CallOncePer1Min>1*60*1000){
      UBaseType_t uxHighWaterMark = uxTaskGetStackHighWaterMark( NULL ); 
      Serial.println();
      Serial.println( "************************ Stepper Task Memory: " + String(uxHighWaterMark) + " "+ String(esp_get_free_heap_size()));
      Serial.println();
      CallOncePer1Min = millis();
    }//*/
  }
}

void loop() {
  
  //Main loop is focused on Reading Bluetooth and tracking other things (no delay)

  //
  static long last_check = millis();
  if(connected == false && millis()-last_check > 500){
    last_check = millis();
    Serial.println();Serial.println("================ Retrying Connection (forced reboot) ==================");
    delay(50);
    ESP.restart();
  }//*/
  
  JoyStick_Value = BluetoothReading();

  //
  static int old_JoyStick_Value = JoyStick_Value;
  if(JoyStick_Value != old_JoyStick_Value){
    Serial.println("Joystick Value: " + String(JoyStick_Value));
    old_JoyStick_Value = JoyStick_Value;
  }//*/
}
int BluetoothReading(){
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  static int temp_int = 0;
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    //Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    //pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());

    if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      //Serial.print("The characteristic value is: ");
      String temp_string = value.c_str();
      temp_int = temp_string.toInt();
      //Serial.println(temp_int);
    }
    
  }else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just example to start scan after disconnect, most likely there is better way to do it in arduino
  }
  return temp_int;
}
