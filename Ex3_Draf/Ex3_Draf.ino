#include <Adafruit_Sensor.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <ESP8266Wifi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
// // MESH Details
// #define MESH_PREFIX "GROUP3"         // name for your MESH
// #define MESH_PASSWORD "GROUP3password" // password for your MESH
// #define MESH_PORT 5555                 // default port
#define STATION_SSID "UiTiOt-E3.1" 
#define STATION_PASSWORD "UiTiOtAP"
//Number for this node
int nodeNumber = 3;
int quangtro = A0;
const int LED = 12;
//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;
AsyncWebServer server(8080);

double temp=0, hum=0, lux;

const char webpage[] PROGMEM = R"=====( 
 <!DOCTYPE html>
<html>
    <head>
        <title>Hệ thống nhúng</title>
        <meta charset="utf-8">
        <h1>HỆ THỐNG NHÚNG MẠNG KHÔNG DÂY - LAB 5</h1>
        <style>
            head, body {
                background-color: rgb(255,240,245);
            }
            h1 {
                text-align: center;
            }
        .container {
            display: flex;
            margin: 50px;
            justify-content: space-around;
        }
        .node {
            background-color: #FFFFFF;
            flex: 1;
            padding: 20px;
            margin: 20px;
            flex-basis: 50%;
            border: 2px solid #000;
            width: 30%;
        }
        .item {
            display: flex;
            margin-top: 10px;
            margin-bottom: 5px ;
        }
        .value {
            margin-left: 5px;
        }
        </style>
    </head>
    <body>
        <div class="container">
            <ul class="node node1"><h2 style="text-align: center; font-size: 20px;">Node ID: 1</h2>
                <li class="item value">Hum: 
                    <span id="hum">0</span>
                </li>
                <li class="item value">Tem: 
                    <span id="temp">0</span>
                </li>
                <div class="item">
                <input type="checkbox" id="checkbox1" onchange="toggleCheckbox(this, 'Led 0')">
<label for="checkbox1">Led 0</label>
<input type="checkbox" id="checkbox2" onchange="toggleCheckbox(this, 'Led 1')">
<label for="checkbox2">Led 1</label>
                </div>
            </ul>
            <ul class="node node2"><h2 style="text-align: center; font-size: 20px">Node ID: 2</h2>
                <li class="item value">Lux: 
                    <span id="lux">0</span>
                </li>
                <div class="item">
                    <input type="checkbox" id="checkbox3" onchange="toggleCheckbox(this, 'Led 2')">
<label for="checkbox3">Led 0</label>
                </div>
            </ul>
        </div>
      <script>
        function getSensorValues() {
            fetch('/get_values') // Gửi yêu cầu GET đến '/get_values'
                .then(response => response.json()) // Chuyển đổi dữ liệu nhận được sang JSON
                .then(data => {
                    // Cập nhật giá trị cảm biến lên trang web
                    document.getElementById('hum').innerText = data.hum;
                    document.getElementById('temp').innerText = data.temp;
                    document.getElementById('lux').innerText = data.lux;
                })
                .catch(error => console.error('Lỗi:', error));
        }
        
    function toggleCheckbox(element, output) {
        var xhr = new XMLHttpRequest();
        var state = element.checked ? 1 : 0;
        console.log("Sending LED state: " + state);
        xhr.open("GET", "/led_set?state=" + state + "&source=" + output, true);
        xhr.send();
    }

        // Gọi hàm getSensorValues() để cập nhật giá trị ban đầu khi trang được tải
        getSensorValues();

        // Tự động cập nhật giá trị cảm biến sau mỗi khoảng thời gian (ví dụ: mỗi 5 giây)
        setInterval(getSensorValues, 5000); // Đổi số 5000 thành thời gian cần cập nhật (miligiây)
    </script>

    </body>
</html>
)=====";

// // User stub
// void sendMessage() ; // Prototype so PlatformIO doesn't complain
// String getReadings(); // Prototype for sending sensor readings

// //Create tasks: to send messages and get readings;
// Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

// String getReadings () {
//   JSONVar jsonData;
//   jsonData["node"] = nodeNumber;
  
//   JSONVar data;
//   data["message"] = "This is node root";
  
//   jsonData["data"] = data;1``
//   data["lux"] = analogRead(quangtro);
  
//   readings = JSON.stringify(jsonData);
//   return readings;
// }

// void sendMessage () {
//   String msg = getReadings();
//   mesh.sendBroadcast(msg);
// }

// Needed for painless library
// void receivedCallback( uint32_t from, String &msg ) {
//   Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
//   JSONVar jsonData = JSON.parse(msg.c_str());

//   int node = jsonData["node"];
//   JSONVar data = jsonData["data"];
//   if (node == 1) {
//     temp = data["temp"];
//     hum = data["hum"];
    
//     Serial.print("Node: ");
//     Serial.println(node);
//     Serial.print("Temperature: ");
//     Serial.print(temp);
//     Serial.println(" C");
//     Serial.print("Humidity: ");
//     Serial.print(hum);
//     Serial.println(" %");
//   } else if (node == 2) {
//     lux = data["lux"];
//     Serial.print("Node: ");
//     Serial.println(node);
//     Serial.print("Light: ");
//     Serial.print(lux);
//     Serial.println(" lx");
//   } else if (node == 3) {
//     double message = data["message"];
//     Serial.print(message);
//   }
// }

// void newConnectionCallback(uint32_t nodeId) {
//   Serial.printf("New Connection, nodeId = %u\n", nodeId);
// }

// void changedConnectionCallback() {
//   Serial.printf("Changed connections\n");
// }

// void nodeTimeAdjustedCallback(int32_t offset) {
//   Serial.printf("Adjusted time %u. Offset = %d\n", mesh.getNodeTime(),offset);
// }
void led_control(AsyncWebServerRequest *request) {
  Serial.println("Received LED control request");
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

  String response = "LED state changed to " + String(act_state) + " from " + source;
    request->send(200, "text/plain", response);
  } else {
    request->send(400, "text/plain", "Bad Request");
  }
}

void setup() {
  Serial.begin(115200);
  pinMode(A0, INPUT);
  pinMode(LED, OUTPUT);
  // //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  // mesh.setDebugMsgTypes( ERROR | STARTUP ); 

  // mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  // mesh.onReceive(&receivedCallback);
  // mesh.onNewConnection(&newConnectionCallback);
  // mesh.onChangedConnections(&changedConnectionCallback);
  // mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  // userScheduler.addTask(taskSendMessage);
  // taskSendMessage.enable();

  
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
  lux = analogRead(quangtro);

  Serial.print("Light: ");
  Serial.print(lux);
  Serial.print(" lx");
  delay(1000);
    // Thử nghiệm chức năng LED

  // it will run the user scheduler as well
  mesh.update();
}