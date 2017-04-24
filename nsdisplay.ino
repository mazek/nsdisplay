#include <ArduinoJson.h>
#include <ArduinoHttpClient.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>

#include <config.h>

#include "memorysaver.h"
#if !(defined ESP8266 )
#error Please select the ArduCAM ESP8266 UNO board in the Tools/Board
#endif

// https://forum.arduino.cc/index.php?topic=46900.0
#define NODEBUG


int min = 80;
int max = 160;

WiFiClient wifi;
HttpClient client = HttpClient(wifi, serverAddress, port);

String response;
char response2[200];

int statusCode = 0;
DynamicJsonBuffer  jsonBuffer;

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
    delay(500);
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

void printDisplayStr(String numbers) {
  int bitToSet = 0;

  //Fill string with empty characters.
  for ( int i = numbers.length(); i<8; i++ ){
    numbers = " " + numbers;
  }
  
  for (int i=7; i >= 0; i--){
    if (numbers.charAt(i) == ' ') {
      bitToSet = 10;
    }
    else {
      bitToSet = numbers[i] - 48;
    }
    DEBUG_PRINTLN(numbers[i]);
    digitalWrite(latchPin, LOW);
    // shift the bits out:
    shiftOut(dataPin, clockPin, MSBFIRST, Tab[bitToSet]);
    // turn on the output so the LEDs can light up:
    digitalWrite(latchPin, HIGH); 
  }
}


void loop() {
  DEBUG_PRINTLN("making GET request");
  client.get("/pebble");

  // read the status code and body of the response
  statusCode = client.responseStatusCode();
  response = client.responseBody();
  response.remove(0, 3);
  response = response.substring(0, response.length()-5);
  response.toCharArray(response2,response.length()+1);
  
  DEBUG_PRINT("Response2: -");
  DEBUG_PRINT(response2);
  DEBUG_PRINTLN("-");
    
  JsonObject& _data = jsonBuffer.parseObject(response2);

  if (!_data.success()) {
    DEBUG_PRINTLN("parseObject() failed:( ");
    delay(5000);
    return;
  }
  else {
    DEBUG_PRINTLN("parseObject() success! ");
  }
 
  String cur_time_s = _data["status"][0]["now"];
  String read_time_s = _data["bgs"][0]["datetime"];

  DEBUG_PRINT("1 cur_time_s: ");
  DEBUG_PRINTLN(cur_time_s);
  
  DEBUG_PRINT("1 read_time_s: ");
  DEBUG_PRINTLN(read_time_s);
  
  cur_time_s = cur_time_s.substring(0, cur_time_s.length()-3);
  read_time_s = read_time_s.substring(0, read_time_s.length()-3);

  DEBUG_PRINT("2 cur_time_s: ");
  DEBUG_PRINTLN(cur_time_s);
  
  DEBUG_PRINT("2 read_time_s: ");
  DEBUG_PRINTLN(read_time_s);
  
  unsigned long  cur_time = cur_time_s.toFloat();
  unsigned long  read_time = read_time_s.toFloat();
  
  DEBUG_PRINT("3 cur_time: ");
  DEBUG_PRINTLN(cur_time);
  
  DEBUG_PRINT("4 read_time: ");
  DEBUG_PRINTLN(read_time);

  unsigned long parakeet_last_seen = cur_time - read_time ;
  DEBUG_PRINT("I seen parakeet more then ");
  DEBUG_PRINT(parakeet_last_seen);
  DEBUG_PRINTLN(" seconds.");
  
  if (parakeet_last_seen > 900) {
    // Lost parakeet signal after 900 seconds.
    DEBUG_PRINTLN("I lost parakeet signal :(");
    clearDisplay();
    printDisplayStr("00000000");
  }
  else {
    // Parakeet operational.
    DEBUG_PRINTLN("I got parakeet signal :)");
    unsigned long bwpo = _data["bgs"][0]["sgv"];
    DEBUG_PRINT("BWPO: ");
    DEBUG_PRINTLN(bwpo);
    long bgdelta = _data["bgs"][0]["bgdelta"];
    DEBUG_PRINT("BGDELTA: ");
    DEBUG_PRINTLN(bgdelta);

    
    if (bgdelta > 0) {
      // Sugar is growing.
      //      bgdelta_s = "+%s" % bgdelta
    }
    else {
      // Sugar is dropping.
      //      bgdelta_s = "%s" % bgdelta
    }
    
    if (bwpo < min) {
      // Sugar below minimum level.
      DEBUG_PRINT("Sugar: ");
      DEBUG_PRINT(bwpo);
      DEBUG_PRINT(", change:  ");
      DEBUG_PRINTLN(bgdelta);
      DEBUG_PRINTLN("Sugar below minimum level."); 
    }
    else if (bwpo > max) {
      // Sugar above maximum level.
      DEBUG_PRINT("Sugar: ");
      DEBUG_PRINT(bwpo);
      DEBUG_PRINT(", change:  ");
      DEBUG_PRINTLN(bgdelta);
      DEBUG_PRINTLN("Sugar above maximum level.");
    }
    else {
      // Sugar ok. 
    }
    clearDisplay();
    DEBUG_PRINTLN("-----");
    DEBUG_PRINTLN(bwpo);
    String displayString = String(bwpo, DEC);
    DEBUG_PRINTLN(displayString);
    DEBUG_PRINTLN("-----");
    clearDisplay();
    printDisplayStr(displayString);

  }
  
  DEBUG_PRINTLN("Wait five seconds");
  delay(60000);
  
}
