#include "soc/timer_group_struct.h"
#include "soc/timer_group_reg.h"
#include <WiFi.h>
#include <Stepper.h>
#include "stdint.h"

#define STEPS_PER_REVOLUTION 250
#define DELAY_MOTOR 10

int ConstStep = 600;
float radiusWheel = 0.1;
float lengthrobot = 2.0;

Stepper Leftstepper(ConstStep, 27, 26, 25, 33);
Stepper Rightstepper(ConstStep, 1, 2, 3, 4);

const char* SSID = "ENTER WIFI ID";
const char* PASSWORD  =  "ENTER WIFI PASSWORD";
const int PORT = 23;
const int LED = 26;

int8_t linear = 0;
int8_t angular = 0;

TaskHandle_t taskWifi;
TaskHandle_t taskMove;

WiFiServer wifiServer(PORT);

void initWifi() {
  WiFi.begin(SSID, PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {

    delay(3000);
    Serial.println("Scanning available networks...");
    printMacAddress();
    Serial.println("MacAddress...");
    listNetworks();
    Serial.println("Connecting to WiFi..");
  }

  Serial.println("Connected to the WiFi network");
  Serial.print("My IP: ");
  Serial.println(WiFi.localIP());
  Serial.print("WiFi status:");
  Serial.println(WiFi.status());
  wifiServer.begin();
}

void setup() {
  Serial.begin(115200); 
  delay(1000);
 
  initWifi();

  xTaskCreatePinnedToCore(threadWifi, "Wifi task", 10000, NULL, 1, &taskWifi, 0);
  delay(500);

  xTaskCreatePinnedToCore(threadMove, "Move task", 10000, NULL, 1, &taskMove, 1);
  delay(500);
}


void printMacAddress() {
  byte mac[6];
  WiFi.macAddress(mac);
  Serial.print("MAC: ");
  Serial.print(mac[5], HEX);
  Serial.print(":");
  Serial.print(mac[4], HEX);
  Serial.print(":");
  Serial.print(mac[3], HEX);
  Serial.print(":");
  Serial.print(mac[2], HEX);
  Serial.print(":");
  Serial.print(mac[1], HEX);
  Serial.print(":");
  Serial.println(mac[0], HEX);
}

void listNetworks() {
  Serial.println("** Scan Networks **");
  byte numSsid = WiFi.scanNetworks();
  Serial.print("number of available networks:");
  Serial.println(numSsid);
  for (int thisNet = 0; thisNet < numSsid; thisNet++) {
    Serial.print(thisNet);
    Serial.print(") ");
    Serial.print(WiFi.SSID(thisNet));
    Serial.print("\tSignal: ");
    Serial.print(WiFi.RSSI(thisNet));
    Serial.print(" dBm");
    Serial.print("\tEncryption: ");
    Serial.println(WiFi.encryptionType(thisNet));
  }
}

void feedWDT0()
{
  TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG0.wdt_feed = 1;
  TIMERG0.wdt_wprotect = 0;
}

void printThreadNum()
{
  Serial.print('[');
  Serial.print(xPortGetCoreID());
  Serial.print("] ");
}

void threadWifi( void * pvParameters ) {
  while(1) {
    feedWDT0();
    WiFiClient client = wifiServer.available();
    if (client) {
      if (client.connected()) {
        printThreadNum();
        Serial.println("Client connected.");
        client.write("You connected\r\n");
      }

      while (client.connected()) {
        feedWDT0();
        while (client.available() >= 3) {
          feedWDT0();
          if (client.read() == '!' && client.available() >= 2) 
          {
            linear = client.read() - '0';
            angular = client.read() - '0';
            printThreadNum();
            Serial.print("Received new data. Linear:" );
            Serial.print(linear);
            Serial.print(" angular:" );
            Serial.println(angular);
          }
        }
        delay(10);
      }
      client.stop();
      Serial.println("Client disconnected");
    }
  }
}

void feedWDT1()
{
  TIMERG1.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
  TIMERG1.wdt_feed = 1;
  TIMERG1.wdt_wprotect = 0;
}

void threadMove( void * pvParameters ) {

  while(1) {
    feedWDT1();
    float lastStepLeft = micros();
    float lastStepRight = micros();
    float rpsLeftlinear = (float)linear / radiusWheel;
    float rpsRightlinear = (float)linear / radiusWheel;
    float rpsLeftrotation = -(lengthrobot / radiusWheel)*((float)angular / 360.0);
    float rpsRightrotation = (lengthrobot / radiusWheel)*((float)angular / 360.0);
    float rpsLeft = rpsLeftlinear+rpsLeftrotation;
    float rpsRight = rpsRightlinear+rpsRightrotation;
    if (linear > 0 || angular > 0) {
    Leftstepper.setSpeed(((uint16_t)rpsLeft + 1)*10);
    Serial.println((int8_t)rpsLeft);    
    Rightstepper.setSpeed(((uint16_t)rpsRight + 1)*10);
    Serial.println((int8_t)rpsRight);
    delay(DELAY_MOTOR);
    Leftstepper.step((int8_t)rpsLeft); 
    Rightstepper.step((int8_t)rpsRight);
    } 
    delay(1000);
  }
}

void loop(){}