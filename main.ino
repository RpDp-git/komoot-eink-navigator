#include <symbols.h>

/**
 * adapted from palto42's code for the ssd1306 display https://github.com/palto42/komoot-navi
 */

#include "BLEDevice.h"
#include <GxEPD.h>
#include <GxGDEH0213B73/GxGDEH0213B73.h>  // 2.13" b/w newer panel
#include <GxIO/GxIO_SPI/GxIO_SPI.h>
#include <GxIO/GxIO.h>
#include GxEPD_BitmapExamples
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>

//define display
GxIO_Class io(SPI, /*CS=5*/ SS, /*DC=*/ 17, /*RST=*/ 16); // arbitrary selection of 17, 16
GxEPD_Class display(io, /*RST=*/ 16, /*BUSY=*/ 4); // arbitrary selection of (16), 4




std::string value = "Start";
int timer = 0 ;
// The remote service we wish to connect to.
static BLEUUID serviceUUID("71C1E128-D92F-4FA8-A2B2-0F171DB3436C");
// The characteristic of the remote service we are interested in.
static BLEUUID    charUUID("503DD605-9BCB-4F6E-B235-270A57483026");

static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;
static BLERemoteCharacteristic* pRemoteCharacteristic;
static BLEAdvertisedDevice* myDevice;

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
   /** if(pRemoteCharacteristic->canRead()) {
      std::string value = pRemoteCharacteristic->readValue();
      Serial.print("The characteristic value was: ");
      Serial.println(value.c_str());
    }
  */
    if(pRemoteCharacteristic->canNotify())
     pRemoteCharacteristic->registerForNotify(notifyCallback);
    Serial.print("The characteristic value was: ");
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

    } 
  }
}; 




void showPartialUpdate_dir(uint8_t dir)
{
  uint16_t box_x = 30;
  uint16_t box_y = 5;
  uint16_t box_w = 60;
  uint16_t box_h = 60;
  display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
  display.drawBitmap(30,5,symbols[dir].bitmap,60,60,GxEPD_BLACK);
  display.updateWindow(box_x, box_y, box_w, box_h, true);
}

void showPartialUpdate_street(std::string a)
{
  uint16_t box_x = 0;
  uint16_t box_y = 80;
  uint16_t box_w = 235;
  uint16_t box_h = 100;
  uint16_t cursor_y = box_y + 20;
  display.setFont(&FreeMonoBold12pt7b);
  display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
  if (a.size()>8){display.setFont(&FreeMonoBold9pt7b);}
  display.setCursor((180-(a.size()*3))/2, cursor_y);
  display.print(a.c_str());
  display.updateWindow(box_x, box_y, box_w, box_h, true);
}


void showPartialUpdate_dist(uint32_t dist)
{
  uint16_t box_x = 125;
  uint16_t box_y = 30;
  uint16_t box_w = 110;
  uint16_t box_h = 35;
  uint16_t cursor_y = box_y+30;
  display.setFont(&FreeMonoBold18pt7b);
  display.fillRect(box_x, box_y, box_w, box_h, GxEPD_WHITE);
  display.setCursor(box_x, cursor_y);
  display.print(dist);
  display.print("m");
  display.updateWindow(box_x, box_y, box_w, box_h, true);
}


void setup() {
   //Display start
  display.init(); //disable debug output
  display.setTextColor(GxEPD_BLACK);
  display.setRotation(3); //orientation set to 1 to flip the display
  display.fillScreen(GxEPD_WHITE);
  display.update();
  delay(1000);
  display.setFont(&FreeMonoBold12pt7b);
  display.setCursor(40,50);
  display.println("BLe Navi v1.0"); //Boot Image
  display.setFont(&FreeMonoBold9pt7b);
  display.setCursor(35,100);
  display.println("Pair to continue..");
  display.update();
  delay(1000);
  display.fillScreen(GxEPD_WHITE);
  display.update();
  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("Superam");
	// Display end
	
	
  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 5 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->setActiveScan(true);
  pBLEScan->start(5, false);
  

  
  
} // End of setup.

std::string old_street ; 
uint8_t dir;
uint32_t dist2;


void loop() {

  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
      //display connected status
      display.setFont(&FreeMonoBold18pt7b);
      display.fillRect(5, 35, 180,60 , GxEPD_WHITE);
      display.setCursor(20, 65);
      display.print("Connected");
      display.updateWindow(5, 35, 205, 60,true);
      delay(500);
      display.fillRect(5, 35, 205, 60, GxEPD_WHITE);
      display.updateWindow(5, 35, 205, 60,true);
     
      
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
  std::string value = pRemoteCharacteristic->readValue();//this crashes sometimes, recieves the whole data packet
  
      if (value.length()>4){
      //in case we have update flag but characteristic changed due to navigation stop between
      
		std::string street;
		std::string firstWord ;
		street = value.substr(9);//this causes abort when there are not at least 9 bytes available
        if (street == old_street){}
        else {
          old_street = street;
          firstWord = street.substr(0, street.find(", "));
          showPartialUpdate_street(firstWord); } //extracts the firstword of the street name and displays it
      
		  std::string direction;
		  direction = value.substr(4,4);
		  uint8_t d=direction[0];
		  if (d == dir){}
			else {
			  dir = d;
			  showPartialUpdate_dir(dir);} //display direction
		  delay(10);
		  Serial.print("Direction: ");
		  Serial.println(d);
		  delay(10);
		  
		  std::string distance;
		  distance = value.substr(5,8);
		  uint32_t dist=distance[0] | distance[1] << 8 | distance[2] << 16 | distance[3] << 24;
		  if (dist2 == dist){}
		  else {
			  dist2 = dist;
			  showPartialUpdate_dist(dist2);} //display distance in metres
		
  }
  
  else if(doScan){
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000); // Delay a second between loops.
}} // End of loop