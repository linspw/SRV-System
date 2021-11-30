//based on https://github.com/yoursunny/esp32cam
#include <esp32cam.h>
#include <WebServer.h>
#include <WiFi.h>
#include "Arduino.h"

// GPIO Setting
extern int gpLb =  2; // Left 1
extern int gpLf = 14; // Left 2
extern int gpRb = 15; // Right 1
extern int gpRf = 13; // Right 2
extern int gpLed =  4; // Light

void WheelAct(int nLf, int nLb, int nRf, int nRb);
void handleMjpeg(void);

const char* WIFI_SSID = "FAMILIA CORREIA";
const char* WIFI_PASS = "35750710";

const char* streamUsername = "esp32";
const char* streamPassword = "pass32";
const char* streamRealm = "ESP32-CAM, please log in!";
const char* authFailResponse = "Sorry, login failed!";

const char* streamPath = "/stream";

static auto hiRes = esp32cam::Resolution::find(800, 600);

const uint8_t jpgqal = 80;
const uint8_t fps = z;    //sets minimum delay between frames, HW limits of ESP32 allows about 12fps @ 800x600

WebServer server(80);

void setup()
{
  Serial.begin(115200);
  Serial.println();

  {
    using namespace esp32cam;
    Config cfg;
    cfg.setPins(pins::AiThinker);
    cfg.setResolution(hiRes);
    cfg.setBufferCount(2);
    cfg.setJpeg(jpgqal);

    bool ok = Camera.begin(cfg);
    Serial.println(ok ? F("CAMERA OK") : F("CAMERA FAIL"));
  }

  Serial.println(String(F("JPEG quality: ")) + jpgqal);
  Serial.println(String(F("Framerate: ")) + fps);

  Serial.print(F("Connecting to WiFi"));
  WiFi.persistent(false);
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(F("."));
    delay(500);
  }

  Serial.print(F("\nCONNECTED!\nhttp://"));
  Serial.print(WiFi.localIP());
  Serial.println(streamPath);

  server.on(streamPath, handleMjpeg);
  server.on("/go", HTTP_GET, []() {
    WheelAct(HIGH, LOW, HIGH, LOW);
    server.send(200, "text/html", "GO");
  });
  server.on("/back", HTTP_GET, []() {
    WheelAct(LOW, HIGH, LOW, HIGH);
    server.send(200, "text/html", "BACK");
  });
  server.on("/left", HTTP_GET, []() {
    WheelAct(HIGH, LOW, LOW, HIGH);
    server.send(200, "text/html", "LEFT");
  });
  server.on("/right", HTTP_GET, []() {
    WheelAct(LOW, HIGH, HIGH, LOW);
    server.send(200, "text/html", "RIGHT");
  });
  server.on("/stop", HTTP_GET, []() {
    WheelAct(LOW, LOW, LOW, LOW);
    server.send(200, "text/html", "STOP");
  });
  server.on("/ledon", HTTP_GET, []() {
    WheelAct(LOW, LOW, LOW, LOW);
    server.send(200, "text/html", "LED ON");
  });
  server.on("/leoff", HTTP_GET, []() {
    digitalWrite(gpLed, LOW);
    server.send(200, "text/html", "LEF OFF");
  });

  server.begin();
}

void loop()
{
  server.handleClient();
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
  // if(!server.authenticate(streamUsername, streamPassword)) {
  //   Serial.println(F("STREAM auth required, sending request"));
  //   return server.requestAuthentication(BASIC_AUTH, streamRealm, authFailResponse);
  // }   
  
  if (!esp32cam::Camera.changeResolution(hiRes)) {
    Serial.println(F("SET RESOLUTION FAILED"));
  }

  struct esp32cam::CameraClass::StreamMjpegConfig mjcfg;
  mjcfg.frameTimeout = 10000;
  mjcfg.minInterval = 1000 / fps;
  mjcfg.maxFrames = -1;
  Serial.println(String (F("STREAM BEGIN @ ")) + fps + F("fps (minInterval ") + mjcfg.minInterval + F("ms)") );
  WiFiClient client = server.client();
  auto startTime = millis();
  int res = esp32cam::Camera.streamMjpeg(client, mjcfg);
  if (res <= 0) {
    Serial.printf("STREAM ERROR %d\n", res);
    return;
  }
  auto duration = millis() - startTime;
  Serial.printf("STREAM END %dfrm %0.2ffps\n", res, 1000.0 * res / duration);
}