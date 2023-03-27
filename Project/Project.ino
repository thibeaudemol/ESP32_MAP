#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>

/*********
  Rui Santos
  Complete instructions at https://RandomNerdTutorials.com/esp32-ble-server-client/
  Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files.
  The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
*********/

//BLE Server name (the other ESP32 name running the server sketch)
#define bleServerName "AoA1"

/* UUID's of the service, characteristic that we want to read*/
// BLE Service
static BLEUUID bmeServiceUUID("e9ea0001-e19b-482d-9293-c7907585fc48");


// BLE Characteristics

static BLEUUID angleCharacteristicUUID("e9ea0002-e19b-482d-9293-c7907585fc48");

static BLEAdvertisedDevice* myDevice;
 

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;
 
//Characteristics that we want to read
static BLERemoteCharacteristic* angleCharacteristic;

BLERemoteService* pRemoteService = nullptr;

//Activate notify
const uint8_t notificationOn[] = {0x1, 0x0};
const uint8_t notificationOff[] = {0x0, 0x0};


//Variables to store temperature and humidity
char* angleChar;

//Flags to check whether new temperature and humidity readings are available
boolean newAngle = false;

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {

  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());
  
  BLEClient* pClient = BLEDevice::createClient();
  if (pClient == nullptr) {
    Serial.println("- Failed to create client");
    return false;
  }
  else {
    Serial.println(" - Created client");
    }
 
  // Connect to the remote BLE Server.
  pClient->connect(myDevice);

   delay(100);  // wait for it to connect

  long startTime = millis();
  while (!pClient->isConnected()) {
    yield();
    if (millis() - startTime > 5000) {
      Serial.println("Timeout");
      return false;
    }
  }
  if (!pClient->isConnected()) {
    // Check again !?
    Serial.println("Not connected");
    return false;
  }
 
  Serial.println(" - Connected to server");

 
  // Get the service associated with this characteristic.
  BLERemoteService *pRemoteService = pClient->getService("e9ea0001-e19b-482d-9293-c7907585fc48");
  if (pRemoteService == nullptr) {
   Serial.println("Failed to find our service UUID: ");
   Serial.println(bmeServiceUUID.toString().c_str());
   return false;
  }
  
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  angleCharacteristic = pRemoteService->getCharacteristic(angleCharacteristicUUID);
 
  if (angleCharacteristic == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");
 
  //Assign callback functions for the Characteristics
  angleCharacteristic->registerForNotify(angleNotifyCallback);
  return true;
}

//Callback function that gets called, when another device's advertisement has been received
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName && advertisedDevice.haveServiceUUID()&& advertisedDevice.isAdvertisingService(bmeServiceUUID)) { //Check if the name and UUID of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      //pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
      Serial.print("haveServiceUUID: "); Serial.println(advertisedDevice.haveServiceUUID());
  Serial.print("device's UUID: "); Serial.println(advertisedDevice.getServiceUUID().toString().c_str());
    }
  }
};
 
//When the BLE Server sends a new angle reading with the notify property
static void angleNotifyCallback(BLERemoteCharacteristic* pBLERemoteCharacteristic, 
                                        uint8_t* pData, size_t length, bool isNotify) {
  //store temperature value
  angleChar = (char*)pData;
  newAngle = true;
}

//function that prints the latest sensor readings in the OLED display
void printReadings(){
  
 
  Serial.print("Temperature:");
  Serial.print(angleChar);
    Serial.print("°");  
}

void setup() {
  //Start serial communication
  Serial.begin(115200);
  Serial.println("Starting Arduino BLE Client application...");

  //Init BLE device
  BLEDevice::init("");
 
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 30 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);//active scan uses more power, but get results faster
  pBLEScan->start(30);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer(myDevice->getAddress())) {
      Serial.println("We are now connected to the BLE Server.");
      //Activate the Notify property of each Characteristic
      angleCharacteristic->getDescriptor(BLEUUID((uint16_t)0x2902))->writeValue((uint8_t*)notificationOn, 2, true);
      connected = true;
    } else {
      Serial.println("We have failed to connect to the server; Restart your device to scan for nearby BLE server again.");
    }
    doConnect = false;
  }
  //if new temperature readings are available, print in the OLED
  if (newAngle){
    newAngle = false;
    printReadings();
  }
  delay(1000); // Delay a second between loops.
}
