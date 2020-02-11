/*
Welcome to the NebulaSmatLight Ardiono code. It is currently under development and the
Nextion screen implementetion has not happened yet. Soon TM :)

YOU ARE NOT PERMITTED TO REMOVE THE FOLLOWING BANNER FROM THE CODE

VERSION: 0.2
DATE: 11/02/2020
Author: Eduardo M (Github: Aguilaair)
 */

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>

ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define LED_PIN    D1
#define LED_COUNT 60



int R_Addr = 0;
int G_Addr = 10;
int B_Addr = 15;
int Br_Addr = 20;
int on_Addr = 25;

Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_GRB + NEO_KHZ800);
//   NEO_KHZ800  800 KHz bitstream (most NeoPixel products w/WS2812 LEDs)
//   NEO_KHZ400  400 KHz (classic 'v1' (not v2) FLORA pixels, WS2811 drivers)
//   NEO_GRB     Pixels are wired for GRB bitstream (most NeoPixel products)
//   NEO_RGB     Pixels are wired for RGB bitstream (v1 FLORA pixels, not v2)
//   NEO_RGBW    Pixels are wired for RGBW bitstream (NeoPixel RGBW products)

uint32_t color = strip.Color(255, 255, 255);
int red = 255;
int green = 255;
int blue = 255;
int brightness = 255;
bool on = true;

#define MyApiKey "xxxxxxxxxxxxxxxxxxx" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "xxxxxxxxxxx" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "xxxxxxxxxxxxxx" // TODO: Change to your Wifi network password

#define API_ENDPOINT "http://sinric.com"
#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

void turnOn(String deviceId) {
  if (deviceId == "xxxxxxxxxxxx") // Device ID of first device
  {
    on = true;
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    setBrightness(brightness);
    RGB_color(red, green, blue);
    EEPROM.put(on_Addr, on);
    EEPROM.commit();
  }
  else if (deviceId == "5axxxxxxxxxxxxxxxxxxx") // Device ID of second device
  {
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);
  }
}

void turnOff(String deviceId) {
  if (deviceId == "xxxxxxxxxxxxxxx") // Device ID of first device
  {
    on = false;
    Serial.print("Turn off Device ID: ");
    Serial.println(deviceId);
    strip.clear();
    strip.show();
    EEPROM.put(on_Addr, on);
    EEPROM.commit();
  }
  else if (deviceId == "5axxxxxxxxxxxxxxxxxxx") // Device ID of second device
  {
    Serial.print("Turn off Device ID: ");
    Serial.println(deviceId);
  }
  else {
    Serial.print("Turn off for unknown device id: ");
    Serial.println(deviceId);
  }
}

void webSocketEvent(WStype_t type, uint8_t * payload, size_t length) {
  switch (type) {
    case WStype_DISCONNECTED:
      isConnected = false;
      Serial.printf("[WSc] Webservice disconnected from sinric.com!\n");
      break;
    case WStype_CONNECTED: {
        isConnected = true;
        Serial.printf("[WSc] Service connected to sinric.com at url: %s\n", payload);
        Serial.printf("Waiting for commands from sinric.com ...\n");
        chase(strip.Color(0, 255, 0));
        chase(strip.Color(0, 255, 0));

        RGB_color(red, green, blue);
      }
      break;
    case WStype_TEXT: {
        Serial.printf("[WSc] get text: %s\n", payload);

        /*
           {"deviceId":"5e28764500886b552b7519fc","action":"action.devices.commands.OnOff","value":{"on":true}}

        */

#if ARDUINOJSON_VERSION_MAJOR == 5
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject((char*)payload);
#endif
#if ARDUINOJSON_VERSION_MAJOR == 6
        DynamicJsonDocument json(1024);
        deserializeJson(json, (char*) payload);
#endif
        String deviceId = json ["deviceId"];
        String action = json ["action"];

        if (action == "action.devices.commands.OnOff") { // Switch or Light
          String value = json ["value"]["on"];
          if (value == "true") {
            turnOn(deviceId);
          } else {
            turnOff(deviceId);
          }
        }
        else if (action == "action.devices.commands.ColorAbsolute") {
          // Alexa, set the device name to red
          // get text: {"deviceId":"xxxx","action":"SetColor","value":{"hue":0,"saturation":1,"brightness":1}}
          int inOLE = json ["value"]["color"]["spectrumRGB"];

          Serial.println(inOLE);
          OLEtoRBG(inOLE);
          RGB_color(red, green, blue);

        }
        else if (action == "action.devices.commands.BrightnessAbsolute") {
          Serial.print("Brightness set to: ");
          brightness = json ["value"]["brightness"];
          setBrightness(brightness);

        }
        else if (action == "test") {
          Serial.println("[WSc] received test command from sinric.com");
        }
      }
      break;
    case WStype_BIN:
      Serial.printf("[WSc] get binary length: %u\n", length);
      break;
    default: break;
  }
}

void OLEtoRBG(int ole) {
  red = round((ole / 65536) % 256);
  green = round((ole / 256) % 256);
  blue = round(ole % 256);
}


void RGB_color(int red_light_value, int green_light_value, int blue_light_value)
{
  if (on == true) {
    Serial.println("Setting color to: ");
    Serial.print(red);
    Serial.print(",");
    Serial.print(green);
    Serial.print(",");
    Serial.println(blue);
    color = strip.Color(red, green, blue);
    strip.fill(color);
    strip.show();
  }
  else if (on == false){
    color = strip.Color(0, 0, 0);
    strip.fill(color);
    strip.show();
  }
  EEPROM.put(R_Addr, red_light_value);
  EEPROM.put(B_Addr, blue_light_value);
  EEPROM.put(G_Addr, green_light_value);
  EEPROM.commit();
}

static void chase(uint32_t c) {
  strip.clear();
  strip.show();
  for (uint16_t i = 0; i < strip.numPixels() + 4; i++) {
    strip.setPixelColor(i  , c); // Draw new pixel
    strip.setPixelColor(i - 4, 0); // Erase pixel a few steps back
    strip.show();
    delay(10);
  }
}

void setBrightness(int br) {
  EEPROM.put(Br_Addr, br);
  EEPROM.commit();
  Serial.print(br);
  Serial.println("%");
  br = br * 2.55;
  Serial.print("So, multiplier is: ");
  Serial.println(br);
  strip.setBrightness(br);
  strip.show();

}

void setup() {
  Serial.begin(115200);
  EEPROM.begin(30);

  strip.begin();           // INITIALIZE NeoPixel strip object (REQUIRED)


  Serial.println(EEPROM.get(R_Addr, red));
  Serial.println(EEPROM.get(B_Addr, blue));
  Serial.println(EEPROM.get(G_Addr, green));
  Serial.println(EEPROM.get(Br_Addr, brightness));
  Serial.println(EEPROM.get(on_Addr, on));


  RGB_color(red, green, blue);
  setBrightness(brightness);
  Serial.println("Setting color to: ");
  Serial.print(red);
  Serial.print(",");
  Serial.print(green);
  Serial.print(",");
  Serial.println(blue);
  Serial.print("At brightness: ");
  Serial.println(brightness);

  WiFiMulti.addAP(MySSID, MyWifiPassword);
  Serial.println();
  Serial.print("Connecting to Wifi: ");
  Serial.println(MySSID);

  // Waiting for Wifi connect
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    chase(strip.Color(255, 0, 0));

  }
  if (WiFiMulti.run() == WL_CONNECTED) {
    Serial.println("");
    Serial.print("WiFi connected. ");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());

  }

  // server address, port and URL
  webSocket.begin("iot.sinric.com", 80, "/");

  // event handler
  webSocket.onEvent(webSocketEvent);
  webSocket.setAuthorization("apikey", MyApiKey);

  // try again every 5000ms if connection has failed
  webSocket.setReconnectInterval(5000);   // If you see 'class WebSocketsClient' has no member named 'setReconnectInterval' error update arduinoWebSockets
}

void loop() {
  webSocket.loop();

  if (isConnected) {
    uint64_t now = millis();

    // Send heartbeat in order to avoid disconnections during ISP resetting IPs over night. Thanks @MacSass
    if ((now - heartbeatTimestamp) > HEARTBEAT_INTERVAL) {
      heartbeatTimestamp = now;
      webSocket.sendTXT("H");
    }
  }
}
