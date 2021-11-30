#include <Arduino.h> // NTRIPClient
#include <WiFi.h>
#include <NTRIPClient.h>
// #include <BLEDevice.h>
// #include <BLEUtils.h>
// #include <BLEServer.h>
#include "BluetoothSerial.h"

// WIFI AND BLE VALUE
int16_t N;
#define SSID_HOME "Fame AIS_2.4G"
#define PASS_HOME "Lqn6b8hyF"
#define SSID_PHON "FT"
#define PASS_PHON "Lqn6b8hyF"
#define SSID_ANDA "ULTRA ANUNDA"
#define PASS_ANDA "anunda7953"
char matchSSID[30];
char matchPASS[20];
WiFiClient client;

// NBTC Board Peripheral
#define SDCARD_MOSI 26 // TXD_3V3 26 
#define SDCARD_MISO 27 // RXD_3V3 27
#define SDCARD_SCK  25
#define SDCARD_CS   34
#define TX1_F9P     33
#define RX1_F9P     32 
#define TX2_F9P     18
#define RX2_F9P     19
#define LED_RTK     23
#define LED_SPP     21
#define LED_ON      22
#define BAUDRATE    38400

// BLE UUID
#define SERVICE_UUID "2f294793-978d-4bbf-826c-229d0aefb522"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26a8"
#define ROVER_BLE_NAME "NBTC-ROVER"
#define ROVER_BLE_PASS "1234"
#if !defined(CONFIG_BT_ENABLED) || !defined(CONFIG_BLUEDROID_ENABLED)
#error Bluetooth is not enabled! Please run `make menuconfig` to and enable it
#endif
BluetoothSerial SerialBT;


// KMIT6 NTRIP Parameter
char* host    = "cssrg-pc.telecom.kmitl.ac.th"; // "27.254.207.85";
int httpPort  = 2101; // 12101;
char* mntpnt  = "KMIT6"; // "DPT9"; // "ANDA";
char* user    = "anda"; //"jirapoom"; // "anundaUser";
char* passwd  = "z8px5f"; //"cssrg"; // "a2n5u6DA";

NTRIPClient ntrip_c;
char corrGet[4096];
char corrSend[sizeof(corrGet)];
int idx;
char nmea[1024];
int idx_nmea;
int idx_wifi = 0;

void bleSetup()
{
  SerialBT.begin(ROVER_BLE_NAME);
  SerialBT.setPin(ROVER_BLE_PASS);
}

void ntripSetup()
{
  Serial.println("Requesting SourceTable.");
  if(ntrip_c.reqSrcTbl(host,httpPort)){
    char buffer[512];
    delay(5);
    while(ntrip_c.available()){
      ntrip_c.readLine(buffer,sizeof(buffer));
      Serial.print(buffer); 
    }
    Serial.print("Requesting SourceTable is OK\n");
  }
  else{
    Serial.println("SourceTable request error");
  }
  ntrip_c.stop(); //Need to call "stop" function for next request.
  
  Serial.println("Requesting MountPoint's Raw data");
  if(!ntrip_c.reqRaw(host,httpPort,mntpnt,user,passwd)){
    delay(15000);
    ESP.restart();
  }
  Serial.println("Requesting MountPoint is OK");
}

void listWifi()
{
  Serial.println("scan start");
  N = WiFi.scanNetworks();
  char checkname[30];
  if (N == 0) 
  {
    Serial.println("no networks found");
  }
  else 
  {
    Serial.print(N);
    Serial.println(" networks found");
    for (int i = 0; i < N; ++i) 
    {
      // Print SSID and RSSI for each network found
      memset(checkname,0,sizeof(checkname));
      strcpy(checkname,WiFi.SSID(i).c_str());
      // result = strcmp(checkname,SSID_HOME); 
      if (strcmp(checkname,SSID_HOME) == 0) //(result == 0)
      {
        memset(matchSSID,0,sizeof(matchSSID));
        memset(matchPASS,0,sizeof(matchPASS));
        strcpy(matchSSID,SSID_HOME);
        strcpy(matchPASS,PASS_HOME);
        Serial.printf("We found home network, ");
        Serial.println(checkname);
      }
      else if (strcmp(checkname,SSID_ANDA) == 0)
      {
        memset(matchSSID,0,sizeof(matchSSID));
        memset(matchPASS,0,sizeof(matchPASS));
        strcpy(matchSSID,SSID_ANDA);
        strcpy(matchPASS,PASS_ANDA);
        Serial.printf("We found office network, ");
        Serial.println(checkname);
      }
      else if (strcmp(checkname,SSID_PHON) == 0)
      {
        memset(matchSSID,0,sizeof(matchSSID));
        memset(matchPASS,0,sizeof(matchPASS));
        strcpy(matchSSID,SSID_PHON);
        strcpy(matchPASS,PASS_PHON);
        Serial.printf("We found hotspot network, ");
        Serial.println(checkname);
      }
      else
      {
        Serial.printf("This network is not in a list!!");
      }
      delay(100);
    }
  }
}

void wifiConnect()
{
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(matchSSID);
  WiFi.begin(matchSSID, matchPASS);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void readCorrection(void *parameter){
  while(1){
    memset(corrGet, 0, sizeof(corrGet));
    idx = 0;
    while(ntrip_c.available()) {
        corrGet[idx++] = ntrip_c.read();        
    }
    Serial.print("RTCM Correction Data is ");
    Serial.println(corrGet);
    if ((strlen(corrGet) > 0) && (idx >= 255)){
      memcpy(corrSend,corrGet,sizeof(corrGet));
      Serial.println("Copy Correction Done!!");
    }
    vTaskDelay(200/portTICK_PERIOD_MS);
  }
}

void sendCorrection(void *parameter){
  while(1){
    if (strlen(corrSend)>0){
      Serial1.write(corrSend,sizeof(corrSend));
      Serial.println("Send Correction Done!!");
      memset(corrSend, 0, sizeof(corrSend));
    }
    vTaskDelay(200/portTICK_PERIOD_MS);
  }
}

void sendNMEA(void *parameter)
{
  // while(1)
  // {
    // SerialBT.available()
  // if (SerialBT.available()) //(SerialBT.availableForWrite())
  // {
    // SerialBT.write(nmea);
  SerialBT.print(nmea);
  // SerialBT.print("\n");
  Serial.println("Send BLE Done!!");
  // }
  //   vTaskDelay(200/portTICK_PERIOD_MS);
  // }
}

void readNMEA(void *parameter){
  while(1){
    memset(nmea, 0x00, sizeof(nmea));
    idx_nmea = 0;
    if (Serial1.available()>0){
      while(Serial1.available()>0){
        nmea[idx_nmea++]=Serial1.read();
      }
    }
    // Serial.print("NMEA Solution = ");
    sendNMEA(nmea);
    // Serial.println(nmea);
    vTaskDelay(200/portTICK_PERIOD_MS);
  }
}

void setup() {
  Serial.begin(115200); // USB Monitor
  Serial1.begin(BAUDRATE,SERIAL_8N1,TX1_F9P,RX1_F9P); // Send RTCM and Receive NMEA from F9P

  // Define LED output
  pinMode(LED_ON,OUTPUT);
  pinMode(LED_SPP,OUTPUT);
  pinMode(LED_RTK,OUTPUT);

  // LED Status
  digitalWrite(LED_ON,HIGH);
  digitalWrite(LED_SPP,LOW);
  digitalWrite(LED_RTK,LOW);

  // WIFI Setup
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);
  Serial.println("WIFI setup done");
  listWifi();
  wifiConnect();

  // BLE
  bleSetup();

  // NTRIP Setup
  ntripSetup();

  // Serial.print(ntrip_c.reqRaw(host,httpPort,mntpnt,user,passwd));

  xTaskCreatePinnedToCore(
                          readCorrection,
                          "READ NTRIP CORRECTION",
                          8192,
                          NULL,
                          1,
                          NULL,
                          1);
  xTaskCreatePinnedToCore(
                          sendCorrection,
                          "SEND CORRECTION TO F9P",
                          8192,
                          NULL,
                          1,
                          NULL,
                          1);                          
  xTaskCreatePinnedToCore(
                          readNMEA,
                          "READ NMEA SOLUTION",
                          2048,
                          NULL,
                          1,
                          NULL,
                          1);
  // xTaskCreatePinnedToCore(
  //                         sendNMEA,
  //                         "SEND NMEA SOLUTION",
  //                         2048,
  //                         NULL,
  //                         1,
  //                         NULL,
  //                         1);
}

void loop() 
{
}

