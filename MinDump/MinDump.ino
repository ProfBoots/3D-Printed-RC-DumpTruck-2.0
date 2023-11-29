// make sure to upload with ESP32 Dev Module selected as the board under tools>Board>ESP32 Arduino

#include <Arduino.h>

#include <ESP32Servo.h> // by Kevin Harrington
#include <ESPAsyncWebSrv.h> // by dvarrel
#include <iostream>
#include <sstream>

#if defined(ESP32)
#include <AsyncTCP.h> // by dvarrel
#include <WiFi.h>
#elif defined(ESP8266)
#include <ESPAsyncTCP.h> // by dvarrel
#endif

// defines

#define steeringServoPin  23
#define dumpServoPin 22
#define lightPin1 26
#define lightPin2 25

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define ARMUP 5
#define ARMDOWN 6
#define STOP 0

#define RIGHT_MOTOR 1
#define LEFT_MOTOR 0
#define ARM_MOTOR 2

#define FORWARD 1
#define BACKWARD -1

// global constants

extern const char* htmlHomePage PROGMEM;
const char* ssid = "ProfBoots MiniDump 1";

// global variables

Servo steeringServo;
Servo dumpServo;

int dumpBedServoValue = 5;
int lightSwitchTime = 0;
bool horizontalScreen; // when screen orientation is locked vertically this rotates the D-Pad controls so that forward would now be left.
bool removeArmMomentum = false;
bool lightsOff = true;

AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

void steeringControl(int steeringServoValue)
{
  if(dumpServo.attached())
  {
   dumpServo.detach();
  }
  if(!(steeringServo.attached()))
  {
    steeringServo.attach(steeringServoPin);
  }
  steeringServo.write(steeringServoValue);
}

void dumpControl(int dumpServoValue)
{
   if(steeringServo.attached())
  {
   steeringServo.detach();
  }
  if(!(dumpServo.attached()))
  {
    dumpServo.attach(dumpServoPin);
  }
  if (dumpServoValue == 5 && dumpBedServoValue < 185)
  {
    dumpBedServoValue = dumpBedServoValue + 5;
    dumpServo.write(dumpBedServoValue);
  }
  if (dumpServoValue == 6 && dumpBedServoValue > 5)
  {
    dumpBedServoValue = dumpBedServoValue - 5;
    dumpServo.write(dumpBedServoValue);
  }
}

void throttleControl(int throttleValue)
{
  if (throttleValue > 20)
  {
    analogWrite(32, throttleValue);
    analogWrite(33, LOW);
  }
  else if (throttleValue < -20)
  {
    throttleValue = throttleValue * -1;
    analogWrite(33, throttleValue);
    analogWrite(32, LOW);
  }
  else
  {
    analogWrite(33, LOW);
    analogWrite(32, LOW);
  }
}
void auxControl(int auxValue)
{
  if ((millis() - lightSwitchTime) > 200)
  {

    Serial.println("Made it to auxcontrol");
    if (auxValue == 1)
    {
      if (lightsOff)
      {
        Serial.println("Made it to if statement");
        digitalWrite(lightPin1, HIGH);
        digitalWrite(lightPin2, LOW);
        lightsOff = false;
      }
      else
      {
        digitalWrite(lightPin1, LOW);
        digitalWrite(lightPin2, LOW);
        lightsOff = true;
      }
      lightSwitchTime = millis();
    }
  }
}

void handleRoot(AsyncWebServerRequest *request)
{
  request->send_P(200, "text/html", htmlHomePage);
}

void handleNotFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "File Not Found");
}

void onCarInputWebSocketEvent(AsyncWebSocket *server,
                              AsyncWebSocketClient *client,
                              AwsEventType type,
                              void *arg,
                              uint8_t *data,
                              size_t len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      Serial.printf("WebSocket client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      AwsFrameInfo *info;
      info = (AwsFrameInfo*)arg;
      if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT)
      {
        std::string myData = "";
        myData.assign((char *)data, len);
        std::istringstream ss(myData);
        std::string key, value;
        std::getline(ss, key, ',');
        std::getline(ss, value, ',');
        Serial.printf("Key [%s] Value[%s]\n", key.c_str(), value.c_str());
        int valueInt = atoi(value.c_str());
        if (key == "steering")
        {
          steeringControl(valueInt);
        }
        else if (key == "throttle")
        {
          throttleControl(valueInt);
        }
        else if (key == "dump")
        {
          dumpControl(valueInt);
        }
        else if (key == "aux")
        {
          auxControl(valueInt);
        }
      }
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;
  }
}

void setUpPinModes()
{
  pinMode(lightPin1, OUTPUT);
  pinMode(lightPin2, OUTPUT); 
  dumpControl(5);
  delay(50);
  steeringControl(80);
}


void setup(void)
{
  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid );
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");
}

void loop()
{
  wsCarInput.cleanupClients();
}
