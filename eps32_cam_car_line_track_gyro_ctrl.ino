#include "esp_camera.h"
#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <iostream>
#include <sstream>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"


#define LIGHT_PIN 4
#define IN1 13
#define IN2 15
#define IN3 14
#define IN4 2
#define SLEEP 12

#define UP 1
#define DOWN 2
#define LEFT 3
#define RIGHT 4
#define STOP 0
#define FR 6
#define FL 5
#define RR 8
#define RL 7

const int PWMFreq = 1000; /* 1 KHz */
const int PWMResolution = 8;
const int nospeed = 0;
int sped;
int diff;
//Camera related constants
#define PWDN_GPIO_NUM     32
#define RESET_GPIO_NUM    -1
#define XCLK_GPIO_NUM      0
#define SIOD_GPIO_NUM     26
#define SIOC_GPIO_NUM     27
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       21
#define Y4_GPIO_NUM       19
#define Y3_GPIO_NUM       18
#define Y2_GPIO_NUM        5
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

const char* ssid     = "Ayken";
const char* password = "Jake1044";

AsyncWebServer server(80);
AsyncWebSocket wsCamera("/Camera");
AsyncWebSocket wsCarInput("/CarInput");
uint32_t cameraClientId = 0;

const char* htmlHomePage PROGMEM = R"HTMLHOMEPAGE(

<!DOCTYPE html>
<html>
  <head>
    <meta name="viewport" content="width=device-width, initial-scale=1, maximum-scale=1, user-scalable=no">
    <style>
      h1 { font-family: Papyrus, Fantasy; }
      .arrows { font-size:10vh; color: transparent; }
      td.button { background-color: #836FFF; border-radius: 5%; }
      td.button:active { transform: scale(1.1, 1.1); background-color: #15F5BA; box-shadow: 0 4px 8px 0 rgba(0, 0, 0, 0.2), 0 6px 20px 0 rgba(0, 0, 0, 0.19); }
      .noselect { -webkit-touch-callout: none; -webkit-user-select: none; -khtml-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; }
      .slidecontainer { width: 100%; }
      .slider { -webkit-appearance: none; width: 100%; height: 5px; border-radius: 5px; background: #211951; outline: none; opacity: 0.5; -webkit-transition: .2s; transition: opacity .2s; }
      .slider:hover { opacity: 1; }
      .slider::-webkit-slider-thumb { -webkit-appearance: none; appearance: none; width: 22px; height: 20px; border-radius: 20%; background: #836FFF; cursor: pointer; }
      .slider::-moz-range-thumb { width: 25px; height: 25px; border-radius: 5%; background: #211951; cursor: pointer; }
      #info { margin-top: 20px; }
      #leftRatio, #rightRatio, #direction { font-size: 20px; margin: 10px; }
    </style>
  </head>
  <body class="noselect" align="center" style="background-color:#F6F1E9">
    <table id="mainTable" style="width:90vw; height:50vh ;margin:auto;table-layout:fixed" CELLSPACING=10>
      <tr>
        <td style="color:#7469B6"><b>speed:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="255" value="0" class="slider" id="Speed" oninput='updateSpeedValue(this.value);'>
            <span id="speedValue">0</span>
          </div>
        </td>
      </tr>
      <tr>
        <td style="color:#7469B6"><b>light:</b></td>
        <td colspan=2>
          <div class="slidecontainer">
            <input type="range" min="0" max="100" value="0" class="slider" id="Light" oninput='sendButtonInput("Light",value)'>
          </div>
        </td>
      </tr>
      <tr>
        <img id="cameraImage" src="" style="width:90vw; height:35vh;"></td>
      </tr>
      <tr>
         <td style="color:#7469B6" colspan="3" class="button" id="toggleLineTracking" onclick="toggleLineTracking()">Line Tracking Disabled</td>
      </tr>
      <tr>
        <td style="color:#7469B6" colspan="3" class="button" id="togglegyro" onclick="togglegyro()">Gyro Disabled</td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","6")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","6")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","1")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","1")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","5")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","5")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","3")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","3")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8678;</span></td>
        <td class="button"></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","4")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","4")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8680;</span></td>
      </tr>
      <tr>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","8")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","8")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8681;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","2")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","2")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
        <td class="button" ontouchstart='sendButtonInput("MoveCar","7")' ontouchend='sendButtonInput("MoveCar","0")'onmousedown='sendButtonInput("MoveCar","7")'onmouseup='sendButtonInput("MoveCar","0")'><span class="arrows">&#8679;</span></td>
      </tr>
      <tr/><tr/>
    </table>
    <div id="info">
      <div id="leftRatio">Left Ratio: 0%</div>
      <div id="rightRatio">Right Ratio: 0%</div>
      <div id="beta">beta: 0</div>
      <div id="gamma">gamma: 0</div>
      <div id="direction">Direction: None</div>
    </div>
    <canvas id="canvas" style="display:none;"></canvas>
    <script>
      var webSocketCameraUrl = "ws:\/\/" + window.location.hostname + "/Camera";
      var webSocketCarInputUrl = "ws:\/\/" + window.location.hostname + "/CarInput";
      var websocketCamera;
      var websocketCarInput;
      var canvas, ctx;
      function updateSpeedValue(val) {
        document.getElementById("speedValue").textContent = val;
        sendButtonInput("Speed",val);
      }
      function updatediffValue(val) {
        document.getElementById("diffVal").textContent = val;
        sendButtonInput("diffValue",val);
      }

      function initCameraWebSocket() {
        websocketCamera = new WebSocket(webSocketCameraUrl);
        websocketCamera.binaryType = 'blob';
        websocketCamera.onopen = function(event) {};
        websocketCamera.onclose = function(event) { setTimeout(initCameraWebSocket, 2000); };
        websocketCamera.onmessage = function(event) {
          var imageId = document.getElementById("cameraImage");
          imageId.src = URL.createObjectURL(event.data);
          imageId.onload = processImage; // Ensure processImage runs when image is loaded
          //requestGyroAccess();
        };
      }

      function initCarInputWebSocket() {
        websocketCarInput = new WebSocket(webSocketCarInputUrl);
        websocketCarInput.onopen = function(event) {
          var speedButton = document.getElementById("Speed");
          sendButtonInput("Speed", speedButton.value);
          var diffButton = document.getElementById("diffValue");
          sendButtonInput("diff", diffButton.value);
          console.log(diffButton);
          var lightButton = document.getElementById("Light");
          sendButtonInput("Light", lightButton.value);

        };
        websocketCarInput.onclose = function(event) { setTimeout(initCarInputWebSocket, 2000); };
        websocketCarInput.onmessage = function(event) {};
      }

      function initWebSocket() {
        initCameraWebSocket();
        initCarInputWebSocket();
        canvas = document.getElementById("canvas");
        ctx = canvas.getContext("2d");
      }

      function sendButtonInput(key, value) {
        var data = key + "," + value;
        websocketCarInput.send(data);
      }
      
      var lineTrackingEnabled = false; // Variable to keep track of line tracking state
      var gyro = false; //Variable to keep track of gyro control

      function toggleLineTracking() {
         lineTrackingEnabled = !lineTrackingEnabled;
         gyro = false;
         document.getElementById("toggleLineTracking").innerText = lineTrackingEnabled ? "Line Tracking Enabled" : "Line Tracking Disabled";
         document.getElementById("togglegyro").innerText = gyro ? "Gyro Enabled"  : "Gyro Disabled";
         if (!lineTrackingEnabled) {
         sendButtonInput("MoveCar", "0"); 
         document.getElementById("direction").innerText = "Direction: None";
            }
        }
        function togglegyro() {
            gyro = !gyro;
            lineTrackingEnabled = false;

            document.getElementById("togglegyro").innerText = gyro ? "Gyro Enabled"  : "Gyro Disabled";
            document.getElementById("toggleLineTracking").innerText = lineTrackingEnabled ? "Line Tracking Enabled" : "Line Tracking Disabled";
     
            if (!gyro) {
            sendButtonInput("MoveCar", "0"); 
            document.getElementById("direction").innerText = "Direction: None";
               }
            window.addEventListener('deviceorientation',
      function(event) {
        console.log("handle orientation");
        if(!gyro){
            return;
        }

        document.getElementById('beta').textContent = (event.beta || 0).toFixed(2);
        document.getElementById('gamma').textContent = (event.gamma || 0).toFixed(2);
      
    
        const beta = (event.beta || 0).toFixed(2);  // Tilt front-to-back in degrees
        const gamma = (event.gamma || 0).toFixed(2); // Tilt left-to-right in degrees

     
        const threshold = 20;  // Threshold for detecting movement
        
        let direction = 'center';
        
        
        if (Math.abs(beta) > threshold || Math.abs(gamma) > threshold) {
          // Compare the absolute values of beta and gamma
          if (Math.abs(beta) > Math.abs(gamma)) {
              // If beta is larger in magnitude, check if it's forward or backward
              direction = beta > threshold ? 'backward' : 'forward';
              document.getElementById("direction").innerText = direction;
              sendButtonInput("MoveCar", beta > threshold ? "2" : "1");
          } else {
              // If gamma is larger in magnitude, check if it's left or right
              direction = gamma > threshold ? 'right' : 'left';
              document.getElementById("direction").innerText = direction;
              sendButtonInput("MoveCar", gamma > threshold ? "4" : "3");
          }
          return;
        } else {
          // If neither beta nor gamma exceeds the threshold, set the direction to center
          direction = 'center';
          document.getElementById("direction").innerText = direction; // Exit the loop after deciding the direction
          
      }
  });
           }

      function processImage() {
        if (!lineTrackingEnabled) return;

        
        var image = document.getElementById("cameraImage");
        canvas.width = image.naturalWidth; // Use naturalWidth for original image dimensions
        canvas.height = image.naturalHeight;
        ctx.drawImage(image, 0, 0, canvas.width, canvas.height);
        var imageData = ctx.getImageData(0, 0, canvas.width, canvas.height);
        var data = imageData.data;

        var blackPixelCount = 0;
        var leftBlackPixelCount = 0;
        var rightBlackPixelCount = 0;
        var midPoint = canvas.width / 2;

        for (var y = 0; y < canvas.height; y++) {
          for (var x = 0; x < canvas.width; x++) {
            var index = (y * canvas.width + x) * 4;
            var r = data[index];
            var g = data[index + 1];
            var b = data[index + 2];

            if (r < 50 && g < 50 && b < 50) {
              blackPixelCount++;
              if (x < midPoint) {
                leftBlackPixelCount++;
              } else {
                rightBlackPixelCount++;
              }
            }
          }
        }

        var totalPixels = canvas.width * canvas.height;
        var leftRatio = (leftBlackPixelCount / totalPixels) * 100;
        var rightRatio = (rightBlackPixelCount / totalPixels) * 100;
        var blackPixelPercentage = (blackPixelCount / totalPixels) * 100;

        console.log(`Total Pixels: ${totalPixels}`);
        console.log(`Black Pixels: ${blackPixelCount}`);
        console.log(`Left Black Pixels: ${leftBlackPixelCount}`);
        console.log(`Right Black Pixels: ${rightBlackPixelCount}`);
        console.log(`Left Ratio: ${leftRatio}%`);
        console.log(`Right Ratio: ${rightRatio}%`);

        document.getElementById("leftRatio").innerText = `Left Ratio: ${leftRatio.toFixed(2)}%`;
        document.getElementById("rightRatio").innerText = `Right Ratio: ${rightRatio.toFixed(2)}%`;
        if (blackPixelPercentage > 90) {
            sendButtonInput("MoveCar", "0"); // Stop the car
            document.getElementById("direction").innerText = "Direction: Stopped (Black > 90%)";
        } else if (blackPixelPercentage < 5) {
            sendButtonInput("MoveCar", "0"); // Stop the car
            document.getElementById("direction").innerText = "Direction: Stopped (Black < 5%)";
        }else if (Math.abs(leftRatio - rightRatio) <= 10) {
          sendButtonInput("MoveCar", "1"); // Move forward
          document.getElementById("direction").innerText = "Direction: Forward";
        } else if (leftRatio > rightRatio) {
          sendButtonInput("MoveCar", "3"); // Move left
          document.getElementById("direction").innerText = "Direction: Left";
        } else {
          sendButtonInput("MoveCar", "4"); // Move right
          document.getElementById("direction").innerText = "Direction: Right";
        }
      }
            
        window.onload = function() {
        
        initWebSocket();
        
      };
    </script>
  </body>
</html>
)HTMLHOMEPAGE";
void moveCar(int inputValue)
{
  Serial.printf("Got value as %d\n", inputValue);  
  switch(inputValue)
  {

    case UP:
          ledcWrite(IN1, sped);
          ledcWrite(IN2, nospeed);
          ledcWrite(IN3, sped);
          ledcWrite(IN4, nospeed);
          digitalWrite(SLEEP,HIGH);
       break;
    case FR:
          ledcWrite(IN1, sped);
          ledcWrite(IN2, nospeed);
          ledcWrite(IN3, sped*0.5);
          ledcWrite(IN4, nospeed);
          digitalWrite(SLEEP,HIGH);
       break;
    case FL:
          ledcWrite(IN1, sped*0.5);
          ledcWrite(IN2, nospeed);
          ledcWrite(IN3, sped);
          ledcWrite(IN4, nospeed);
          digitalWrite(SLEEP,HIGH);
       break;
    case DOWN:
          ledcWrite(IN1, nospeed);
          ledcWrite(IN2, sped);
          ledcWrite(IN3, nospeed);
          ledcWrite(IN4, sped);
          digitalWrite(SLEEP,HIGH);  
      break;
    case RR:
          ledcWrite(IN1, nospeed);
          ledcWrite(IN2, sped);
          ledcWrite(IN3, nospeed);
          ledcWrite(IN4, sped*0.5);
          digitalWrite(SLEEP,HIGH);  
      break;
    case RL:
          ledcWrite(IN1, nospeed);
          ledcWrite(IN2, sped*0.5);
          ledcWrite(IN3, nospeed);
          ledcWrite(IN4, sped);
          digitalWrite(SLEEP,HIGH);  
      break;
  
    case LEFT:
          ledcWrite(IN1, sped);
          ledcWrite(IN2, nospeed);
          ledcWrite(IN3, nospeed);
          ledcWrite(IN4, sped); 
          digitalWrite(SLEEP,HIGH); 
      break;
  
    case RIGHT:
          ledcWrite(IN1, nospeed);
          ledcWrite(IN2, sped);
          ledcWrite(IN3, sped);
          ledcWrite(IN4, nospeed);
          digitalWrite(SLEEP,HIGH);
      break;
 
    case STOP:
          digitalWrite(SLEEP,LOW);
    
      
      break;
  
    default:
          digitalWrite(SLEEP,LOW);  
      break;
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
      moveCar(0);
      ledcWrite(LIGHT_PIN, 0);  
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
        else if (key == "Speed")
        {
          sped=valueInt;
        }
        else if (key == "Light")
        {
          ledcWrite(LIGHT_PIN, valueInt);         
        }   
        else if (key == "diff"){
          diff=sped-valueInt;
          }  
      }
      break;
    case WS_EVT_PONG:
      moveCar(0);
    case WS_EVT_ERROR:
      moveCar(0);
      break;
    default:
      moveCar(0);
      break;  
  }
}

void onCameraWebSocketEvent(AsyncWebSocket *server, 
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
      cameraClientId = client->id();
      break;
    case WS_EVT_DISCONNECT:
      Serial.printf("WebSocket client #%u disconnected\n", client->id());
      cameraClientId = 0;
      break;
    case WS_EVT_DATA:
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
    default:
      break;  
  }
}

void setupCamera()
{
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sscb_sda = SIOD_GPIO_NUM;
  config.pin_sscb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  
  config.frame_size = FRAMESIZE_SVGA;
  config.jpeg_quality = 24;
  config.fb_count = 1;

  // camera init
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) 
  {
    Serial.printf("Camera init failed with error 0x%x", err);
    return;
  }  
  sensor_t * s = esp_camera_sensor_get();
  s->set_brightness(s, 2);     // -2 to 2
  s->set_contrast(s, 1);       // -2 to 2
  s->set_saturation(s, 1);     // -2 to 2
  s->set_hmirror(s, 1);        // 0 = disable , 1 = enable
  s->set_vflip(s, 1); 

  if (psramFound())
  {
    heap_caps_malloc_extmem_enable(10000);  
    Serial.printf("PSRAM initialized. malloc to take memory from psram above this size");    
  }  
}

void sendCameraPicture()
{
  if (cameraClientId == 0)
  {
    return;
  }
  unsigned long  startTime1 = millis();
  //capture a frame
  camera_fb_t * fb = esp_camera_fb_get();
  if (!fb) 
  {
      Serial.println("Frame buffer could not be acquired");
      return;
  }

  unsigned long  startTime2 = millis();
  wsCamera.binary(cameraClientId, fb->buf, fb->len);
  esp_camera_fb_return(fb);
    
  //Wait for message to be delivered
  while (true)
  {
    AsyncWebSocketClient * clientPointer = wsCamera.client(cameraClientId);
    if (!clientPointer || !(clientPointer->queueIsFull()))
    {
      break;
    }
    delay(1);
  }
  
  unsigned long  startTime3 = millis();  
  //Serial.printf("Time taken Total: %d|%d|%d\n",startTime3 - startTime1, startTime2 - startTime1, startTime3-startTime2 );
}

void setUpPinModes()
{
  //Set up PWM
   
    pinMode(IN1, OUTPUT);
    pinMode(IN2, OUTPUT);
    pinMode(IN3, OUTPUT);
    pinMode(IN4, OUTPUT);
    pinMode(SLEEP,OUTPUT);
    pinMode(LIGHT_PIN, OUTPUT);   
  ledcAttach(IN1,PWMFreq,PWMResolution);
  ledcAttach(IN2,PWMFreq,PWMResolution);
  ledcAttach(IN3,PWMFreq,PWMResolution);
  ledcAttach(IN4,PWMFreq,PWMResolution);
  ledcAttach(LIGHT_PIN,PWMFreq,PWMResolution);
  moveCar(STOP);
}


void setup(void) 
{
  //WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0); //disable brownout detector

  setUpPinModes();
  Serial.begin(115200);

  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);

  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound(handleNotFound);
      
  wsCamera.onEvent(onCameraWebSocketEvent);
  server.addHandler(&wsCamera);

  wsCarInput.onEvent(onCarInputWebSocketEvent);
  server.addHandler(&wsCarInput);

  server.begin();
  Serial.println("HTTP server started");

  setupCamera();
}


void loop() 
{
  wsCamera.cleanupClients(); 
  wsCarInput.cleanupClients(); 
  sendCameraPicture(); 
  //Serial.printf("SPIRam Total heap %d, SPIRam Free Heap %d\n", ESP.getPsramSize(), ESP.getFreePsram());
}
