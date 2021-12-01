#include <esp32cam.h>
#include <WebServer.h>
#include <WiFi.h>
#include "Arduino.h"
#include "ESPAsyncWebServer.h"

// GPIO Setting
int gpLb =  2; // Left 1
int gpLf = 14; // Left 2
int gpRb = 15; // Right 1
int gpRf = 13; // Right 2
int gpLed =  4; // Light

void WheelAct(int nLf, int nLb, int nRf, int nRb);
void handleMjpeg(void);

const char* WIFI_SSID = "FAMILIA CORREIA";
const char* WIFI_PASS = "35750710";

const char* streamUsername = "esp32";
const char* streamPassword = "pass32";
const char* streamRealm = "ESP32-CAM, please log in!";
const char* authFailResponse = "Sorry, login failed!";

const char* streamPath = "/stream";

static auto loRes = esp32cam::Resolution::find(320, 240);
static auto midRes = esp32cam::Resolution::find(350, 530);
static auto hiRes = esp32cam::Resolution::find(800, 600);

const uint8_t jpgqal = 80;
const uint8_t fps = 12;    //sets minimum delay between frames, HW limits of ESP32 allows about 12fps @ 800x600

AsyncWebServer serverHTTP(80);
WebServer serverStream(81);

void setCrossOrigin(){
    serverStream.sendHeader(F("Access-Control-Allow-Origin"), F("*"));
    serverStream.sendHeader(F("Access-Control-Max-Age"), F("600"));
    serverStream.sendHeader(F("Access-Control-Allow-Methods"), F("PUT,POST,GET,OPTIONS"));
    serverStream.sendHeader(F("Access-Control-Allow-Headers"), F("*"));
};

void setupMotors() {
  pinMode(gpLb, OUTPUT); //Left Backward
  pinMode(gpLf, OUTPUT); //Left Forward
  pinMode(gpRb, OUTPUT); //Right Forward
  pinMode(gpRf, OUTPUT); //Right Backward
  pinMode(gpLed, OUTPUT); //Light
}
 

void setup()
{
  Serial.begin(115200);
  Serial.setDebugOutput(true);

  setupMotors();

  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(midRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(jpgqal);

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? F("CAMERA OK") : F("CAMERA FAIL"));
  }

  Serial.println(String(F("JPEG quality: ")) + jpgqal);
  Serial.println(String(F("Framerate: ")) + fps);

  Serial.print(F("Connecting to WiFi"));

  WiFi.begin(WIFI_SSID, WIFI_PASS);
 
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(500);
  }

  Serial.print(F("\nCONNECTED!\nhttp://"));
  Serial.print(WiFi.localIP());
  Serial.print(":81");
  Serial.println(streamPath);

  serverStream.enableCORS();
  serverStream.on(streamPath, handleMjpeg);

  serverHTTP.on("/go", HTTP_GET, [](AsyncWebServerRequest *request) {
    WheelAct(HIGH, LOW, HIGH, LOW);
    Serial.println("Go!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "GO");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });
  serverHTTP.on("/back", HTTP_GET, [](AsyncWebServerRequest *request) {
    WheelAct(LOW, HIGH, LOW, HIGH);
    Serial.println("Back!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "BACK");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });
  serverHTTP.on("/left", HTTP_GET, [](AsyncWebServerRequest *request) {
    WheelAct(HIGH, LOW, LOW, HIGH);
    Serial.println("Left!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "LEFT");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });
  serverHTTP.on("/right", HTTP_GET, [](AsyncWebServerRequest *request) {
    WheelAct(LOW, HIGH, HIGH, LOW);
    Serial.println("Right!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "RIGHT");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });
  serverHTTP.on("/stop", HTTP_GET, [](AsyncWebServerRequest *request) {
    WheelAct(LOW, LOW, LOW, LOW);
    Serial.println("Stop!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "STOP");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });
  serverHTTP.on("/ledon", HTTP_GET, [](AsyncWebServerRequest *request) {
    WheelAct(LOW, LOW, LOW, LOW);
    Serial.println("Led on!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "LED ON");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });
  serverHTTP.on("/ledoff", HTTP_GET, [](AsyncWebServerRequest *request) {
    digitalWrite(gpLed, LOW);
    Serial.println("Led off!");
    AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", "LEF OFF");
    response->addHeader("Access-Control-Allow-Origin","*");
    request->send(response);
  });

  serverStream.begin();
  serverHTTP.begin();
}

void loop()
{
  serverStream.handleClient();
}

void WheelAct(int nLf, int nLb, int nRf, int nRb)
{
  digitalWrite(gpLf, nLf);
  digitalWrite(gpLb, nLb);
  digitalWrite(gpRf, nRf);
  digitalWrite(gpRb, nRb);
}


void handleMjpeg()
{
  setCrossOrigin();
  if (!esp32cam::Camera.changeResolution(midRes)) {
    Serial.println(F("SET RESOLUTION FAILED"));
  }

  struct esp32cam::CameraClass::StreamMjpegConfig mjcfg;
  mjcfg.frameTimeout = 10000;
  mjcfg.minInterval = 1000 / fps;
  mjcfg.maxFrames = -1;
 
  Serial.println(String (F("STREAM BEGIN @ ")) + fps + F("fps (minInterval ") + mjcfg.minInterval + F("ms)") );
  WiFiClient client = serverStream.client();
  auto startTime = millis();
  int res = esp32cam::Camera.streamMjpeg(client, mjcfg);
  if (res <= 0) {
    Serial.printf("STREAM ERROR %d\n", res);
    return;
  }
  auto duration = millis() - startTime;
  Serial.printf("STREAM END %dfrm %0.2ffps\n", res, 1000.0 * res / duration);
}
