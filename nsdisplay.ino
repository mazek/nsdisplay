#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>


#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

// https://forum.arduino.cc/index.php?topic=46900.0
#define DEBUG

#include "config.h"

int min = 80;
int max = 160;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

long rssi = 0;

String response;
int statusCode = 0;

char cur_time_s[13];
char read_time_s[13];

unsigned long  cur_time;
unsigned long  read_time;

unsigned long sugar_level;
long sugar_level_delta;

unsigned long parakeet_last_seen;

//DynamicJsonBuffer  jsonBuffer;
StaticJsonBuffer<2000>  jsonBuffer;
  
//Pin connected to latch pin (ST_CP) of 74HC595
const int latchPin = 12;
//Pin connected to clock pin (SH_CP) of 74HC595
const int clockPin = 13;
//Pin connected to Data in (DS) of 74HC595
const int dataPin = 15;

//How to draw a digit 0 - 9 and blank character.
byte Tab[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90,0xff};

// For storing text to draw.
String displayString;

void setup() {
  Serial.begin(115200);
  DEBUG_PRINTLN("ArduCAM Start!");

  // Connect to WiFi network
  DEBUG_PRINT("Connecting to ");
  DEBUG_PRINTLN(ssid);
    
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(100);
    Serial.print(".");
  }
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN(WiFi.localIP());

  // Initialize display.
  pinMode(latchPin, OUTPUT);
  pinMode(dataPin, OUTPUT);  
  pinMode(clockPin, OUTPUT);
    
}

void clearDisplay() {
  for (int i=0; i <= 7; i++){
    digitalWrite(latchPin, LOW);
    // shift the bits out:
    shiftOut(dataPin, clockPin, MSBFIRST, Tab[10]);
    // turn on the output so the LEDs can light up:
    digitalWrite(latchPin, HIGH); 
  }
}

void printDisplayError() {
  byte Tab_error[]={0xff,0xff,0xff,0xAF,0xA3,0xAF,0xAF,0x86};
  for (int i=0; i <= 7; i++){
    digitalWrite(latchPin, LOW);
    // shift the bits out:
    shiftOut(dataPin, clockPin, MSBFIRST, Tab_error[i]);
    // turn on the output so the LEDs can light up:
    digitalWrite(latchPin, HIGH);      
  }
}

void printDisplayStr(String numbers) {
  int bitToSet = 0;

  //Fill string with empty characters.
  for ( int i = numbers.length(); i<8; i++ ){
    numbers = " " + numbers;
  }
  
  DEBUG_PRINT("printDisplayStr: -");
  for (int i=7; i >= 0; i--){
    if (numbers.charAt(i) == ' ') {
      bitToSet = 10;
    }
    else {
      bitToSet = numbers[i] - 48;
    }
    DEBUG_PRINT(numbers[i]);
    digitalWrite(latchPin, LOW);
    // shift the bits out:
    shiftOut(dataPin, clockPin, MSBFIRST, Tab[bitToSet]);
    // turn on the output so the LEDs can light up:
    digitalWrite(latchPin, HIGH); 
  }
  DEBUG_PRINTLN("- :printDisplayStr");
}

void parse_json(unsigned long&  cur_time, unsigned long&  read_time, unsigned long& sugar_level, long& sugar_level_delta, unsigned long& parakeet_last_seen) {

  //DynamicJsonBuffer  jsonBuffer;
  StaticJsonBuffer<1000>  jsonBuffer;
    
  DEBUG_PRINTLN("making GET request");
  client.get("/pebble");

  // read the status code and body of the response
  statusCode = client.responseStatusCode();
  response = client.responseBody();

  DEBUG_PRINT("statusCode: ");
  DEBUG_PRINTLN(statusCode);

  DEBUG_PRINT("Response: -");
  DEBUG_PRINT(response);
  DEBUG_PRINTLN("-");
      
  JsonObject& _data = jsonBuffer.parseObject(response);

  if (!_data.success()) {
    DEBUG_PRINTLN("parseObject() failed:( ");
    return;
  }
  else {
    DEBUG_PRINTLN("parseObject() success! ");
  }

  strncpy(cur_time_s, _data["status"][0]["now"],10);
  strncpy(read_time_s, _data["bgs"][0]["datetime"],10);  
  
  DEBUG_PRINT("1. cur_time_s: ");
  DEBUG_PRINT(cur_time_s);
  
  DEBUG_PRINT(", read_time_s: ");
  DEBUG_PRINTLN(read_time_s);
  
  cur_time = atof(cur_time_s);
  read_time = atof(read_time_s);
  
  DEBUG_PRINT("2. cur_time: ");
  DEBUG_PRINT(cur_time);
  
  DEBUG_PRINT(", read_time: ");
  DEBUG_PRINTLN(read_time);

  parakeet_last_seen = cur_time - read_time ;
  DEBUG_PRINT("I seen parakeet more then ");
  DEBUG_PRINT(parakeet_last_seen);
  DEBUG_PRINTLN(" seconds.");

  if (parakeet_last_seen > 900) {
    DEBUG_PRINTLN("I lost parakeet signal :(");
  }
  else {
    // Parakeet operational.
    DEBUG_PRINTLN("I got parakeet signal :)");
    sugar_level = _data["bgs"][0]["sgv"];
    DEBUG_PRINT("Sugar level: ");
    DEBUG_PRINTLN(sugar_level);
    sugar_level_delta = _data["bgs"][0]["bgdelta"];
    DEBUG_PRINT("sugar_level_delta: ");
    DEBUG_PRINTLN(sugar_level_delta);
  }
  return;
}



void loop() {
  DEBUG_PRINT("Free Heap: ");
  DEBUG_PRINTLN(ESP.getFreeHeap());

  rssi = WiFi.RSSI();  
  DEBUG_PRINT("RSSI: ");
  DEBUG_PRINTLN(rssi);  
  
  parse_json(cur_time, read_time, sugar_level, sugar_level_delta, parakeet_last_seen);
  
  if (parakeet_last_seen > 900) {
    // Lost parakeet signal after 900 seconds.
    DEBUG_PRINTLN("I lost parakeet signal :(");
    clearDisplay();
    //printDisplayStr("00000000");
    printDisplayError();  
  }
  else {
    // Parakeet operational.
    DEBUG_PRINTLN("I got parakeet signal :)");
    
    if (sugar_level_delta > 0) {
      // Sugar is growing.
      //      bgdelta_s = "+%s" % bgdelta
    }
    else {
      // Sugar is dropping.
      //      bgdelta_s = "%s" % bgdelta
    }
  
    if (sugar_level < min) {
      // Sugar below minimum level.
      DEBUG_PRINTLN("Sugar below minimum level."); 
    }
    else if (sugar_level > max) {
      // Sugar above maximum level.
      DEBUG_PRINTLN("Sugar above maximum level.");
    }
    else {
      // Sugar ok. 
    }
    clearDisplay();
    DEBUG_PRINT("Sugar lvl (dec): ");
    DEBUG_PRINTLN(sugar_level);
    String displayString = String(sugar_level, DEC);
     DEBUG_PRINT("Sugar lvl (str): ");
    DEBUG_PRINTLN(displayString);

    clearDisplay();
    printDisplayStr(displayString);
  }
  
  DEBUG_PRINTLN("Wait five seconds");
  delay(60000);
  DEBUG_PRINTLN();
  
}
