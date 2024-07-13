#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <ESP8266Wifi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define MESH_PREFIX "GROUP3"
#define MESH_PASSWORD "GROUP3password"
#define MESH_PORT 5555
#define STATION_SSID "UiTiOt-E3.1" 
#define STATION_PASSWORD "UiTiOtAP"

int nodeNumber = 3;
const int LED = 15;
//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; 
painlessMesh  mesh;
AsyncWebServer server(8080);

double temp, hum, lux;

const char webpage[] PROGMEM = R"=====( 
<style>
    body {
        background: darkseagreen;
        display: flex;
        min-height: 100vh;
        margin: 0;
        align-items: center;
        justify-content: center;
        font-family: Cambria, sans-serif;
        flex-direction: column;
    }
    h1 {
        margin-bottom: 10px;
    }

    .button {
        display: inline-block;
        margin: 10px auto;
        padding: 10px;
        font-size: 25px;
        border-radius: 8px;
        width: 150px;
    }

    h2 {
        margin-top: 20px;
    }
</style>
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <title>Bài 2</title>
</head>
<body>
<h1>Hệ thống nhúng mạng không dây - LAB 5 - GROUP 3</h1>
<div>
    <button class="button" onclick="send(1)" style="background: green">LED ON</button>
    <button class="button" onclick="send(0)"style="background: brown">LED OFF</button>
</div>
<br>
<div>
    <h2>Nhiệt độ (C): <span id="temp">0</span></h2>
    <h2>Độ ẩm (%): <span id="hum">0</span></h2>
    <h2>Ánh sáng (lx): <span id="lux">0</span></h2>
    <p id = "state"></p> <!-- Added an element to display the response -->
</div>

<script>
  function send(led_sts, source) {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function() {
      if (this.readyState == 4 && this.status == 200) {
          // Xử lý responseText từ cảm biến
          document.getElementById("state").innerHTML = this.responseText;
        }
    };
    xhttp.open("GET", "led_set?state=" + led_sts, true);
    xhttp.send();
  }
  setInterval(funtion(){
    getlux();
    gettemp();
    gethum();
  },1000)
function getlux(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200){
      document.getElementById("lux").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "luxread", true);
  xhttp.send();
}
function gettemp(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200){
      document.getElementById("temp").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "tempread", true);
  xhttp.send();
}
function gethum(){
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200){
      document.getElementById("hum").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "humread", true);
  xhttp.send();
}
</script>
</body>
</html>
)=====";

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

//Create variable JSON
String getReadings () {
  JSONVar jsonData;
  jsonData["node"] = nodeNumber;
  
  JSONVar data;
  data["message"] = "This is node root\n";
  
  jsonData["data"] = data;
  
  readings = JSON.stringify(jsonData);
  return readings;
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar jsonData = JSON.parse(msg.c_str());

  int node = jsonData["node"];
  JSONVar data = jsonData["data"];
  if (node == 1) {
    temp = data["temp"];
    hum = data["hum"];
    
    Serial.print("Node: ");
    Serial.println(node);
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
  } else if (node == 2) {
    lux = data["lux"];
    Serial.print("Node: ");
    Serial.println(node);
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
  } else if (node == 3) {
    double message = data["message"];
    Serial.print(message);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
}
void led_control() {
  String state ="OFF";
    String act_state = request->getParam("state")->value();
    if (act_state == "1") {
      digitalWrite(LED, HIGH);
      state = "ON";
    } else {
      digitalWrite(LED, LOW);
      state = "OFF"
    }
    request->send(200, "text/plain", state);
  }
}
void luxdata(){
  String sensor_value = String (lux);
  server.send(200, "text/plain", sensor_value)
}
void tempdata(){
  String sensor_value = String (temp);
  server.send(200, "text/plain", sensor_value)
}
void humdata(){
  String sensor_value = String (hum);
  server.send(200, "text/plain", sensor_value)
}
void handleRoot(){
  String s = webpage;
  server.send(200,"text/html",s);
}
void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP ); 

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();

 // Connect to Wi-Fi
  WiFi.begin(STATION_SSID, STATION_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  // Print ESP Local IP Address
  Serial.println(WiFi.localIP());

  // Server the HTML file when the root path is requested
  // server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
  //   request->send(200, "text/html", webpage); 
  // }); 

  // Handle the "/led_set" route
  // server.on("/led_set", HTTP_GET, [](AsyncWebServerRequest *request){
  //   led_control(request);
  // });
  server.on("/",handleRoot);
  server.on("led_set", led_control);
  // Handle the "/get_values" route
  server.on("/luxread", luxdata);
  server.on("/tempread", tempdata);
  server.on("/humread", humdata);
  // Start the server
  server.begin();
  Serial.println("Server begin");
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
  server.handleClient();
}