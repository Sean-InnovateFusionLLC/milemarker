// Generated by   : Sean Matthews
// Device Info .  : Lilgo T-Display-S3 
// MCU .          : ESP32-S3R8 Dual-core LX7 microprocessor
// Wireless Connectivity:  Wi-Fi 802.11, BLE 5 + BT mesh
// Programming Platform  Arduino-ide、 Micropython
// Flash  16MB
// PSRAM 8MB [OPI]
// Partition Scheme  Huge APP (3MB No OTA/1MB SPIFFS
// Bat voltage detection IO04
// Onboard functions Boot + Reset + IO14 Button
// LCD 1.9" diagonal, Full-color TFT Display
// Drive Chip  ST7789V
// Resolution  170(H)RGB x320(V) 8-Bit Parallel Interface
// Working power supply  3.3v
// Support  STEMMA QT / Qwiic , JST-SH 1.0mm 4-PIN
// Connector JST-GH 1.25mm 2PIN
// Plug in device and upload scetch there is no need for an specail button press
// USB CDC On Boot  Enabled
// CPU Frequency 240MHz (WiFi)
// Core Debug Level  None
// USB DFU On Boot Enabled
// Events Run On Core 1
// Flash Mode  QIO 80MHz
// JTAG Adapter  Integrated USB JTAG
// Arduino Runs On Core 1
// USB Firmware MSC On Boot  Disabled
// USB Mode  Hardware CDC and JTAG


#include <esp_wifi.h>
#include <WiFi.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <TFT_eSPI.h>
#include "jatf.h"
#include "captive.h"
#include <AsyncEventSource.h>
#include <AsyncJson.h>
#include <SPIFFSEditor.h>
#include <WebHandlerImpl.h>
#include <WebAuthentication.h>
#include <AsyncWebSynchronization.h>
#include <AsyncWebSocket.h>
#include <WebResponseImpl.h>
#include <StringArray.h>
#include <ESPAsyncWebSrv.h>
#include <DNSServer.h>

///Main.ino
struct Config {
  const char* name;
  const char* password;
  const char* ip;
  const char* mac;
  const char* btName;
  const char* btUUID;
};

Config configs[] = {
  {"OFF", "", "", "", "", ""},
  {"AP1", "password1", "192.168.1.1", "DE:AD:BE:EF:FE:01", "", ""},
  {"AP2", "password2", "192.168.2.1", "DE:AD:BE:EF:FE:02", "", ""},
  {"AP3", "password3", "192.168.3.1", "DE:AD:BE:EF:FE:03", "", ""},
  {"BT1", "", "", "", "BT-01", "00001101-0000-1000-8000-00805F9B34FB"},
  {"BT2", "", "", "", "BT-02", "00001102-0000-1000-8000-00805F9B34FB"},
  {"BT3", "", "", "", "BT-03", "00001103-0000-1000-8000-00805F9B34FB"}
};

int currentConfig = 0;
const int numConfigs = sizeof(configs) / sizeof(Config);
const int buttonPin = 0;
const int interruptPin = 0;  // Adjust this to your button pin
volatile bool screenSleep = false;
const unsigned long debounceTime = 50;
int lastButtonState = HIGH;
unsigned long lastDebounceTime = 0;
unsigned long screenOffTime = 0;  // Timestamp to handle screen sleep
unsigned long deepSleepTime = 0;  // Timestamp to handle deep sleep
unsigned long lastChangeTime = 0;


BLEServer* pServer = NULL;
TFT_eSPI tft = TFT_eSPI();

void IRAM_ATTR handleInterrupt() {
  tft.writecommand(TFT_DISPON);  // Instead of tft.wakeup();
  screenSleep = false;
  screenOffTime = millis();  // Update the screen off timestamp
}


void setup() {
  Serial.begin(115200);
  pinMode(buttonPin, INPUT_PULLUP);
  // Attach the interrupt
  pinMode(interruptPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(interruptPin), handleInterrupt, RISING);
  tft.init();
  tft.setRotation(1);
  tft.fillScreen(TFT_BLACK);
  setupConfig();
  screenOffTime = millis();  // Initialize screen off timestamp
  deepSleepTime = millis();  // Initialize deep sleep timestamp
  
}

void loop() {
  int reading = digitalRead(buttonPin);
  if (reading != lastButtonState) {
    lastDebounceTime = millis();
  }
  if ((millis() - lastDebounceTime) > debounceTime) {
    if (reading == LOW) {
      currentConfig = (currentConfig + 1) % numConfigs;
      setupConfig();
      // Update timestamps
      screenOffTime = millis();
      deepSleepTime = millis();
    }
  }

  // If broadcasting (config not "OFF") and 2 minutes passed with no button press, turn off the screen
  if (currentConfig != 0 && millis() - screenOffTime >= 120000) {
    // Write your code here to turn off the screen
    tft.writecommand(TFT_DISPOFF);  // Turn off the screen (if your display supports this command)
  }

  // If not broadcasting (config "OFF") and 5 minutes passed with no button press, deep sleep
  if (currentConfig == 0 && millis() - deepSleepTime >= 300000) {
    WiFi.mode(WIFI_OFF);       // Turn off WiFi
    btStop();                  // Turn off Bluetooth
    esp_deep_sleep_start();    // Go to deep sleep

    }
  
  lastButtonState = reading;
  handleCaptivePortal();
}

void setupConfig() {
  WiFi.softAPdisconnect(true);
  if(pServer) {
    BLEDevice::deinit(false);
    pServer = NULL;
  }
  if(currentConfig == 0) {
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 20);  
    tft.setTextColor(0x21a1);  
    tft.setTextSize(1);
    tft.setFreeFont(&FreeMonoBold12pt7b);
    tft.println("JATF MILE MARKER V1");  
    tft.setTextColor(TFT_WHITE);
    tft.println("No broadcast");
    tft.setSwapBytes(true);
    tft.pushImage(110,60,103,103,jatf);
    tft.setCursor(230, 161);  
    tft.setTextColor(0x01df);  
    tft.setFreeFont(&FreeMonoBold9pt7b);
    tft.println("START>>>"); 
    esp_sleep_enable_timer_wakeup(5 * 60 * 1000000); // Set deep sleep if no operations after 5 minutes
  }
  else if(currentConfig < 4) {
    IPAddress ip;
    ip.fromString(configs[currentConfig].ip);
    uint8_t mac[6];
    sscanf(configs[currentConfig].mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    esp_wifi_set_mac(WIFI_IF_AP, mac);
    WiFi.softAP(configs[currentConfig].name, configs[currentConfig].password);
    WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
    setupCaptivePortal();
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 20);  
    tft.setTextColor(0x21a1);
    tft.setTextSize(1); 
    tft.setFreeFont(&FreeMonoBold12pt7b);
    tft.println("JATF MILE MARKER V1");
    tft.setTextColor(TFT_GREEN);
    tft.println("WiFi Broadast:");
    tft.setTextColor(TFT_WHITE);
    tft.println(configs[currentConfig].name);
    tft.println("IP:");
    tft.println(configs[currentConfig].ip);
    tft.println("MAC:");
    tft.println(configs[currentConfig].mac);
    esp_sleep_enable_timer_wakeup(2 * 60 * 1000000); // Set screen sleep if no button press after 2 minutes
    if (!screenSleep && millis() - lastChangeTime > 2 * 60 * 1000) {
      // Put screen to sleep after 2 minutes of no button press
      tft.writecommand(TFT_DISPOFF); 
      screenSleep = true;
    }
  } 
  else {
    BLEDevice::init(configs[currentConfig].btName);
    pServer = BLEDevice::createServer();
    BLEService *pService = pServer->createService(configs[currentConfig].btUUID);
    pService->start();
    BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
    pAdvertising->addServiceUUID(configs[currentConfig].btUUID);
    pAdvertising->setScanResponse(true);
    pAdvertising->setMinPreferred(0x06);
    pAdvertising->setMinPreferred(0x12);
    BLEDevice::startAdvertising();
    tft.fillScreen(TFT_BLACK);
    tft.setCursor(20, 20);
    tft.setTextColor(0x21a1);
    tft.setTextSize(1); 
    tft.setFreeFont(&FreeMonoBold12pt7b);
    tft.println("JATF MILE MARKER V1");
    tft.setTextColor(TFT_BLUE);
    tft.println("Bluetooth Broadcast:");
    tft.setTextColor(TFT_WHITE);
    tft.println(configs[currentConfig].btName);
    tft.println("UUID:");
    tft.println(configs[currentConfig].btUUID);
    esp_sleep_enable_timer_wakeup(2 * 60 * 1000000); // Set screen sleep if no button press after 2 minutes
    if (!screenSleep && millis() - lastChangeTime > 2 * 60 * 1000) {
      // Put screen to sleep after 2 minutes of no button press
      tft.writecommand(TFT_DISPOFF); 
      screenSleep = true; 
    }
  }
  lastChangeTime = millis();
 }
