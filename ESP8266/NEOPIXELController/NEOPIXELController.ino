#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <math.h>
#include <Adafruit_NeoPixel.h>
#include <EEPROM.h>
#include <Nextion.h>
#include <SoftwareSerial.h>


ESP8266WiFiMulti WiFiMulti;
WebSocketsClient webSocket;
WiFiClient client;

#define LED_PIN    D1
#define LED_COUNT 50



int R_Addr = 0;
int G_Addr = 10;
int B_Addr = 15;
int Br_Addr = 20;
int on_Addr = 25;

int rgb_colors[3];

float hue;
double saturation;
double bri;
int tempK;

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

#define MyApiKey "66470be6-244b-4602-9c5e-eec7a2d15aa5" // TODO: Change to your sinric API Key. Your API Key is displayed on sinric.com dashboard
#define MySSID "D-LINK" // TODO: Change to your Wifi network SSID
#define MyWifiPassword "12331233" // TODO: Change to your Wifi network password

#define API_ENDPOINT "http://sinric.com"
#define HEARTBEAT_INTERVAL 300000 // 5 Minutes 

String devid = "5e5a4867a23b266b59a95ecf";

uint64_t heartbeatTimestamp = 0;
bool isConnected = false;

SoftwareSerial nextion(D2, D3);// Nextion TX to pin 2 and RX to pin 3 of Arduino

Nextion myNextion(nextion, 9600); //create a Nextion object named myNextion using the nextion serial port @ 9600bps

int incomingByte = 0; // for incoming serial data


void turnOn(String deviceId) {
  if (deviceId == devid) // Device ID of first device
  {
    on = true;
    Serial.print("Turn on device id: ");
    Serial.println(deviceId);
    setBrightness(brightness);
    RGB_color(red, green, blue);
    myNextion.sendCommand("main.pwrmain.val=1");
    myNextion.sendCommand("colorpicker.pwrmini1.val=1");
    EEPROM.put(on_Addr, on);
    EEPROM.commit();
  }
  else {
    Serial.print("Turn on for unknown device id: ");
    Serial.println(deviceId);
  }
}

void turnOff(String deviceId) {
  if (deviceId == devid) // Device ID of first device
  {
    on = false;
    Serial.print("Turn off Device ID: ");
    Serial.println(deviceId);
    strip.clear();
    strip.show();
    myNextion.sendCommand("main.pwrmain.val=0");
    myNextion.sendCommand("colorpicker.pwrmini1.val=0");
    EEPROM.put(on_Addr, on);
    EEPROM.commit();
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
        if (deviceId == devid) // Device ID of first device
        {
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
          //Alexa
          else if (action == "setPowerState") { // Switch or Light
            String value = json ["value"];
            if (value == "ON") {
              turnOn(deviceId);
            } else {
              turnOff(deviceId);
            }
          }
          else if (action == "SetBrightness") {
            Serial.print("Brightness set to: ");
            brightness = json ["value"];
            setBrightness(brightness);

          }
          else if (action == "SetColor") {
            // Alexa, set the device name to red
            // get text: {"deviceId":"xxxx","action":"SetColor","value":{"hue":0,"saturation":1,"brightness":1}}

            hue = json ["value"]["hue"];
            saturation = json ["value"]["saturation"];
            bri = json ["value"]["brightness"];
            getRGB(hue, saturation, bri);
            RGB_color(red, green, blue);

          }
          else if (action == "SetColorTemperature") {
            // Alexa, set the device name to red
            // get text: {"deviceId":"xxxx","action":"SetColor","value":{"hue":0,"saturation":1,"brightness":1}}

            tempK = json ["value"];
            temp2rgb(tempK);
            RGB_color(red, green, blue);

          }
          else if (action == "test") {
            Serial.println("[WSc] received test command from sinric.com");
          }
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

void getRGB(int h, double s, double v) {
  //this is the algorithm to convert from RGB to HSV
  double r = 0;
  double g = 0;
  double b = 0;

  double hf = h / 60.0;

  int i = (int)floor(h / 60.0);
  double f = h / 60.0 - i;
  double pv = v * (1 - s);
  double qv = v * (1 - s * f);
  double tv = v * (1 - s * (1 - f));

  switch (i)
  {
    case 0: //rojo dominante
      r = v;
      g = tv;
      b = pv;
      break;
    case 1: //verde
      r = qv;
      g = v;
      b = pv;
      break;
    case 2:
      r = pv;
      g = v;
      b = tv;
      break;
    case 3: //azul
      r = pv;
      g = qv;
      b = v;
      break;
    case 4:
      r = tv;
      g = pv;
      b = v;
      break;
    case 5: //rojo
      r = v;
      g = pv;
      b = qv;
      break;
  }

  //set each component to a integer value between 0 and 255
  red = constrain((int)255 * r, 0, 255);
  green = constrain((int)255 * g, 0, 255);
  blue = constrain((int)255 * b, 0, 255);
}

void temp2rgb(unsigned int kelvin) {
  int temp = kelvin / 100;
  int r, g, b;

  if (temp <= 66) {
    r = 255;
    g = 99.4708025861 * log(temp) - 161.1195681661;
    if (temp <= 19) {
      b = 0;
    } else {
      b = 138.5177312231 * log(temp - 10) - 305.0447927307;
    }
  } else {
    if (temp < 60) {
      r = 0;
      g = 0;
    } else {
      r = 329.698727446 * pow(temp - 60, -0.1332047592);
      g = 288.1221695283 * pow(temp - 60, -0.0755148492);
    }
    b = 255;
  }

  red = (r > 0) ? r : 0;
  green = (g > 0) ? g : 0;
  blue = (b > 0) ? b : 0;
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
    myNextion.sendCommand("main.pwrmain.val=1");
    myNextion.sendCommand("colorpicker.pwrmini1.val=1");
  }
  else if (on == false) {
    color = strip.Color(0, 0, 0);
    strip.fill(color);
    strip.show();
    myNextion.sendCommand("main.pwrmain.val=0");
    myNextion.sendCommand("colorpicker.pwrmini1.val=0");
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
  myNextion.sendCommand(("main.br.val=" + String(br)).c_str() );
  myNextion.sendCommand(("colorpicker.br.val=" + String(br)).c_str());
  myNextion.sendCommand(("colorpicker.brslider.val=" + String(br)).c_str());
  br = br * 2.55;
  Serial.print("So, multiplier is: ");
  Serial.println(br);
  strip.setBrightness(br);
  strip.show();


}

void setup() {
  Serial.begin(9600);
  EEPROM.begin(30);
  myNextion.init("0");

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
    myNextion.sendCommand("page 1");
    myNextion.sendCommand("wifistatus.picc=2");
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

  String message = myNextion.listen(); //check for message
  if (message == "ON") {
    turnOn("5e5a4867a23b266b59a95ecf");
  }
  if (message == "OFF") {
    turnOff("5e5a4867a23b266b59a95ecf");
  }

}
