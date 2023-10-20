//make sure to upload with ESP32 Dev Module selected as the board under tools>Board>ESP32 Arduino

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h> //by dvarrel
#elif defined(ESP8266)
#include <ESPAsyncTCP.h> //by dvarrel
#endif
#include <ESPAsyncWebSrv.h> //by dvarrel

#include <ESP32Servo.h> //by Kevin Harrington
#include <iostream>
#include <sstream>

const char* ssid     = "ProfBoots MiniDump 1";


#define steeringServoPin  23

#define dumpServoPin 22

Servo steeringServo;
Servo dumpServo;

int dumpBedServoValue = 185;
struct MOTOR_PINS
{
  int pinIN1;
  int pinIN2;
};

std::vector<MOTOR_PINS> motorPins =
{
  {25, 26},  //N20 Motor Pins(IN1, IN2)
};

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

bool horizontalScreen;//When screen orientation is locked vertically this rotates the D-Pad controls so that forward would now be left.
bool removeArmMomentum = false;



AsyncWebServer server(80);
AsyncWebSocket wsCarInput("/CarInput");

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(
<!DOCTYPE html>
<html>
  <head>
  <meta name="viewport" content="width=device-width, initial-scale=.9, maximum-scale=1, user-scalable=yes">
    <style>
    .arrows {
      font-size:50px;
      color:grey;
    }
    td.button {
      background-color:black;
      border-radius:20%;
      box-shadow: 5px 5px #888888;
    }
    td.button:active {
      transform: translate(5px,5px);
      box-shadow: none; 
    }
    .auxButton {
  background-color: black; /* Background color of the button */
  box-shadow: 5px 5px #888888;
  color: grey; /* Text color */
  padding: 30px 40px; /* Padding for the button */
  border: none; /* Remove the button border */
  border-radius: 20%; /* Rounded corners */
  font-size: 24px; /* Font size */
  cursor: pointer; /* Cursor style on hover */
  transform: rotate(90deg); /* Rotate the text vertically */
  transform-origin: left center; /* Adjust the origin to change the rotation pivot */
  margin-left: 60px;
}
    .auxButton:active {
      transform: translate(-5px,5px);
      transform: rotate(90deg); /* Rotate the text vertically */
      transform-origin: left center;
      
      box-shadow: none; 
    }

    .noselect {
      -webkit-touch-callout: none; /* iOS Safari */
        -webkit-user-select: none; /* Safari */
         -khtml-user-select: none; /* Konqueror HTML */
           -moz-user-select: none; /* Firefox */
            -ms-user-select: none; /* Internet Explorer/Edge */
                user-select: none; /* Non-prefixed version, currently
                                      supported by Chrome and Opera */
    }
        .slidecontainer {
      width: 100%;
    }

    .slider {
      -webkit-appearance: none;
      width: 150%;
      height: 20px;
      border-radius: 5px;
      background: #d3d3d3;
      outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
    }

    .slider:hover {
      opacity: 1;
    }
  
    .slider::-webkit-slider-thumb {
      -webkit-appearance: none;
      appearance: none;
      width: 65px;
      height: 65px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

    .slider::-moz-range-thumb {
      width: 60px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }

.vertical-slider-container {
  display: flex;
  flex-direction: column;
  align-items: center;
  margin-top: 165px;
  width: 5px; /* Adjust the width as needed */
  height: 100px; /* Adjust the height as needed */
}

.vertical-slider {
  writing-mode: bt-lr; /* IE/Edge specific property for vertical text */
  -webkit-appearance: none;
  width: 400px;
  height: 20px;
  transform: rotate(270deg);
     background: #d3d3d3; /* Background color of the slider */
        outline: none;
      opacity: 0.7;
      -webkit-transition: .2s;
      transition: opacity .2s;
}

.vertical-slider:hover {
      opacity: 1;
    }
.vertical-slider::-webkit-slider-thumb {
        -webkit-appearance: none;
      appearance: none;
  width: 65px; /* Adjust the width to make the slider thumb thicker */
  height: 65px; /* Adjust the height to make the slider thumb thicker */
  background-color: red; /* Background color of the slider thumb */
  border: none; /* Remove the default border */
  //margin-top: -5px; /* Center the thumb vertically within the track */
}
    .vertical-slider::-moz-range-thumb {
      width: 60px;
      height: 40px;
      border-radius: 50%;
      background: red;
      cursor: pointer;
    }


    

    </style>
  
  </head>
  <body class="noselect" align="center" style="background-color:white" >
    <h1 style="color: black; text-align:center;">MINI-DUMP</h1>
    
    <table id="mainTable" style="width:400px;margin:auto;table-layout:fixed" CELLSPACING=10>
    <tr>
        <td colspan=2 style="text-align: center;">
         <div class="slidecontainer">
            <input type="range" min="-255" max="255" value="0" class="slider" id="throttle" oninput='sendButtonInput("throttle",value)'>
          </div>
        </td>
      </tr>  
      <tr/>
      <tr/>
      <tr/><tr/>
      <tr>
        <td class="button"
    ontouchstart='startSendingButtonInput("dump", "5")'
    onmousedown='startSendingButtonInput("dump", "5")'
    ontouchend='stopSendingButtonInput()'
    onmouseup='stopSendingButtonInput()'>
    <span class="arrows">&#8678;</span></td>
        <td class="button"></td>    
        <td class="button"
    ontouchstart='startSendingButtonInput("dump", "6")'
    onmousedown='startSendingButtonInput("dump", "6")'
    ontouchend='stopSendingButtonInput()'
    onmouseup='stopSendingButtonInput()'>
    <span class="arrows">&#8680;</span></td>
      </tr>
      <tr/>
      <tr/>
      <tr/><tr/>
<tr>
  <td style="text-align: left; font-size: 25px"><b></b></td>
  <td>
    <div class="vertical-slider-container">
      <input type="range" min="60" max="140" value="100" class="vertical-slider" id="steering" oninput='sendButtonInput("steering", value)'>
    </div>
  </td>
  <td>
    <button id="auxButton" class="auxButton">AUX</button>
  </td>
</tr>
    </table>
  
    <script>
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";      
      var websocketCarInput;
      const throttleSlider = document.getElementById('throttle');
      const steeringSlider = document.getElementById('steering');
      
      function initCarInputWebSocket() 
      {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onclose   = function(event){setTimeout(initCarInputWebSocket, 2000);};
        websocketCarInput.onmessage = function(event){};        
      }
      
      function sendButtonInput(key, value) 
      {
       var data = key + "," + value;
       websocketCarInput.send(data);
      }
      let intervalId = null;

    function startSendingButtonInput(action, value) {
    sendButtonInput(action, value); // Send the initial input when the button is pressed
    intervalId = setInterval(function() {
        sendButtonInput(action, value); // Continuously send the input as long as the button is pressed
    }, 50); // You can adjust the interval (in milliseconds) to control the rate of sending
    }

    function stopSendingButtonInput() {
    clearInterval(intervalId); // Stop sending the input when the button is released
}
      function handleKeyDown(event) {
        if (event.keyCode === 38) {
            sendButtonInput("MoveCar", "1");
        }
        if (event.keyCode === 40)
        {
          sendButtonInput("MoveCar", "2");
        }
        if (event.keyCode ===37)
        {
          sendButtonInput("MoveCar", "3");
        }
        if (event.keyCode ===39)
        {
          sendButtonInput("MoveCar", "4");
        }
        if (event.keyCode === 87)
        {
          sendButtonInput("MoveCar", "5");
        }
        if (event.keyCode === 83)
        {
          sendButtonInput("MoveCar", "6");
        }
        }
      function handleKeyUp(event) {
        if (event.keyCode === 37 || event.keyCode === 38 || event.keyCode === 39 || event.keyCode === 40 || event.keyCode === 87 || event.keyCode === 83) {
            sendButtonInput("MoveCar", "0");
        }
    }  
      
  
      window.onload = initCarInputWebSocket;
      document.getElementById("mainTable").addEventListener("touchend", function(event){
        event.preventDefault()
      });
      document.addEventListener('keydown', handleKeyDown);
      document.addEventListener('keyup', handleKeyUp); 
           
    </script>
  </body>    
</html>
)HTMLHOMEPAGE";


void rotateMotor(int motorNumber, int motorDirection)
{
  if (motorDirection == FORWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, HIGH);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);    
  }
  else if (motorDirection == BACKWARD)
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, HIGH);     
  }
  else
  {
    digitalWrite(motorPins[motorNumber].pinIN1, LOW);
    digitalWrite(motorPins[motorNumber].pinIN2, LOW);       
  }
}

void moveCar(int inputValue)
{
  Serial.printf("Got value as %d\n", inputValue); 
  if(!(horizontalScreen))
  { 
  switch(inputValue)
  {

    case UP:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);                  
      break;
  
    case DOWN:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
  
    case LEFT:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);  
      break;
  
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD); 
      break;

    case STOP:
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;
      
    default:   
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP); 
      break;
  }
  }else {
      switch(inputValue)
  {
     case UP:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, FORWARD);                  
      break;
  
    case DOWN:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
  
    case LEFT:
      rotateMotor(RIGHT_MOTOR, BACKWARD);
      rotateMotor(LEFT_MOTOR, BACKWARD);  
      break;
  
    case RIGHT:
      rotateMotor(RIGHT_MOTOR, FORWARD);
      rotateMotor(LEFT_MOTOR, FORWARD); 
      break;

    case STOP:
      rotateMotor(ARM_MOTOR, STOP); 
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP);    
      break;

    case ARMUP:
      rotateMotor(ARM_MOTOR, FORWARD); 
      break;
      
    case ARMDOWN:
      rotateMotor(ARM_MOTOR, BACKWARD);
      removeArmMomentum = true;
      break; 
      
    default:
      rotateMotor(ARM_MOTOR, STOP);    
      rotateMotor(RIGHT_MOTOR, STOP);
      rotateMotor(LEFT_MOTOR, STOP); 
      break;
  }
  }
}


void steeringControl(int steeringServoValue)
{
  steeringServo.write(steeringServoValue); 
}
void dumpControl(int dumpServoValue){
  if(dumpServoValue == 5 && dumpBedServoValue < 185)
  {
    dumpBedServoValue = dumpBedServoValue + 5;
    dumpServo.write(dumpBedServoValue);
  }
  if(dumpServoValue == 6 && dumpBedServoValue > 5)
  {
    dumpBedServoValue = dumpBedServoValue - 5;
    dumpServo.write(dumpBedServoValue);
  }
}
void throttleControl(int throttleValue)
{
  if(throttleValue > 15)
  {
    analogWrite(32, throttleValue);
    analogWrite(33, LOW);
  }
  else if(throttleValue < -15)
  {
    throttleValue = throttleValue*-1;
    analogWrite(33, throttleValue);
    analogWrite(32, LOW);
  }
  else
  {
    analogWrite(33, LOW);
    analogWrite(32, LOW);
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
      moveCar(STOP);
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
        if (key == "MoveCar")
        {
          moveCar(valueInt);        
        }
        else if (key == "steering")
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
      
  for (int i = 0; i < motorPins.size(); i++)
  {   
    pinMode(motorPins[i].pinIN1, OUTPUT);
    pinMode(motorPins[i].pinIN2, OUTPUT);  
  }
  moveCar(STOP);
  steeringServo.attach(steeringServoPin);
  dumpServo.attach(dumpServoPin);
  steeringControl(90);
  dumpServo.write(dumpBedServoValue);
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
