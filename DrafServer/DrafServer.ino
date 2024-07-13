#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <ESP8266Wifi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
// MESH Details
#define MESH_PREFIX "GROUP3"         // name for your MESH
#define MESH_PASSWORD "GROUP3password" // password for your MESH
#define MESH_PORT 5555                 // default port
#define STATION_SSID "UiTiOt-E3.1" 
#define STATION_PASSWORD "UiTiOtAP"
//Number for this node
int nodeNumber = 3;
const int LED = 1;
//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
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
    <button class="button" onclick="send(1, 'button')" style="background: green">LED ON</button>
    <button class="button" onclick="send(0, 'button')"style="background: brown">LED OFF</button>
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
        // Xác định liệu yêu cầu là từ button hay từ cảm biến
        if (source === 'button') {
          // Xử lý responseText từ button
          document.getElementById("state").innerHTML = "Response from button: " + this.responseText;
        } else {
          // Xử lý responseText từ cảm biến
          document.getElementById("state").innerHTML = "Response from sensor: " + this.responseText;
        }
      } else {
        document.getElementById("state").innerHTML = "Error";
      }
    };
    xhttp.open("GET", "led_set?state=" + led_sts + "&source=" + source, true);
    xhttp.send();
  }
  // Gửi yêu cầu để bật đèn LED
  function turnOnLED() {
      socket.send('turnOnLED');
  }
  // Gửi yêu cầu để tắt đèn LED
  function turnOffLED() {
      socket.send('turnOffLED');
  }
  // Gửi yêu cầu để kiểm tra trạng thái của đèn LED
  function getLEDState() {
      socket.send('getLEDState');
  }
  function updateValuesRealtime() {
  // Gửi yêu cầu để lấy giá trị mới từ máy chủ
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      var response = JSON.parse(this.responseText);
      temp = response.temp;
      hum = response.hum;
      lux = response.lux;

      // Cập nhật giá trị trên trang web
      document.getElementById("temp").innerHTML = temp;
      document.getElementById("hum").innerHTML = hum;
      document.getElementById("lux").innerHTML = lux;
    } else {
      console.log("Error updating values");
    }
  };
  xhttp.open("GET", "/get_values", true);
  xhttp.send();
}

// Tự động cập nhật giá trị mỗi 5 giây
setInterval(updateValuesRealtime, 5000);
</script>
</body>
</html>
)=====";

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

String getReadings () {
  JSONVar jsonData;
  jsonData["node"] = nodeNumber;
  
  JSONVar data;
  data["message"] = "This is node root";
  
  jsonData["data"] = data;
  
  readings = JSON.stringify(jsonData);
  return readings;
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

// Needed for painless library
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
void led_control(AsyncWebServerRequest *request) {
  if (request->hasParam("state") && request->hasParam("source")) {
    String act_state = request->getParam("state")->value();
    String source = request->getParam("source")->value();

    if (act_state == "1") {
      // Assuming LED is defined somewhere in your code
      digitalWrite(LED, HIGH);
    } else {
      // Assuming LED is defined somewhere in your code
      digitalWrite(LED, LOW);
    }

    String response = "LED state changed to " + act_state + " from " + source;
    request->send(200, "text/plain", response);
  } else {
    request->send(400, "text/plain", "Bad Request");
  }
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

// Serve the HTML file when the root path is requested
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){ 
    request->send(200, "text/html", webpage); 
  }); 

  // // Handle the "/message" route
  // server.on("/message", HTTP_GET, [](AsyncWebServerRequest *request){
  //   String response = "This is a message from the server";
  //   request->send(200, "text/plain", response);
  // });

  // Handle the "/led_set" route
  server.on("/led_set", HTTP_GET, [](AsyncWebServerRequest *request){
    led_control(request);
  });

  // Handle the "/get_values" route
  server.on("/get_values", HTTP_GET, [](AsyncWebServerRequest *request) {
    // Tạo một đối tượng JSON để chứa giá trị thời gian thực
    JSONVar response;
    response["temp"] = temp;
    response["hum"] = hum;
    response["lux"] = lux;

    // Chuyển đối tượng JSON thành chuỗi
    String jsonResponse = JSON.stringify(response);

    // Trả về chuỗi JSON làm phản hồi cho yêu cầu AJAX
    request->send(200, "application/json", jsonResponse);
  });

  // Start the server
  server.begin();
  Serial.println("Server begin");
}

void loop() {
  // it will run the user scheduler as well
  mesh.update();
}