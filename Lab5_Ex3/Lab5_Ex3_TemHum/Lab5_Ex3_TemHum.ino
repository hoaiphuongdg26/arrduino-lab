#include <Adafruit_Sensor.h>
#include <Wire.h>

#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include <DHT.h>

// MESH Details
#define MESH_PREFIX "GROUP3"         // name for your MESH
#define MESH_PASSWORD "GROUP3password" // password for your MESH
#define MESH_PORT 5555                 // default port

// const int DHTPIN = 11; //Đọc dữ liệu từ DHT22 ở chân A3 trên mạch Arduino
// const int DHTTYPE = DHT22; //Khai báo loại cảm biến, có 2 loại là DHT11 và DHT22
// // DHT22 object on the default I2C pins
// DHT dht(DHTPIN, DHTTYPE);

#define DHTPIN 12
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

const int LED0 = 14;
const int LED1 = 13;

//Number for this node
int nodeNumber = 1;

//String to send to other nodes with sensor readings
String readings;

Scheduler userScheduler; // to control your personal task
painlessMesh  mesh;

// User stub
void sendMessage() ; // Prototype so PlatformIO doesn't complain
String getReadings(); // Prototype for sending sensor readings

//Create tasks: to send messages and get readings;
Task taskSendMessage(TASK_SECOND * 5 , TASK_FOREVER, &sendMessage);

String getReadings () {
  JSONVar jsonData;
  jsonData["node"] = nodeNumber;
  
  JSONVar data;
  data["temp"] = dht.readTemperature();
  data["hum"] = dht.readHumidity();
  
  jsonData["data"] = data;
  
  readings = JSON.stringify(jsonData);
  return readings;
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

//Init DHT22
void initSensor(){
  dht.begin();
}

// Needed for painless library
void receivedCallback( uint32_t from, String &msg ) {
  Serial.printf("Received from %u msg=%s\n", from, msg.c_str());
  JSONVar jsonData = JSON.parse(msg.c_str());

  int node = jsonData["node"];
  JSONVar data = jsonData["data"];
  if (node == 1) {
    double temp = data["temp"];
    double hum = data["hum"];
    
    Serial.print("Node: ");
    Serial.println(node);
    Serial.print("Temperature: ");
    Serial.print(temp);
    Serial.println(" C");
    Serial.print("Humidity: ");
    Serial.print(hum);
    Serial.println(" %");
  } else if (node == 2) {
    double lux = data["lux"];
    Serial.print("Node: ");
    Serial.println(node);
    Serial.print("Light: ");
    Serial.print(lux);
    Serial.println(" lx");
  } else if (node == 3) {
    String message = data["message"];
    String source = data["source"];
    Serial.print(message);
    
        if (message == "TurnOnLED") {
            if (source == "checkbox1") {
              digitalWrite(LED0, HIGH);
            } else if (source == "checkbox2") {
              digitalWrite(LED1, HIGH);
            }
        } else if (message == "TurnOffLED") {
          if (source == "checkbox1") {
              digitalWrite(LED0, LOW);
            } else if (source == "checkbox2") {
              digitalWrite(LED1, LOW);
            }
        }
        else {
          Serial.print(message);
        }
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

void setup() {
  Serial.begin(115200);
  dht.begin();
  pinMode(DHTPIN, OUTPUT);
  pinMode(LED0, OUTPUT);
  pinMode(LED1, OUTPUT);
  
  initSensor();

  //mesh.setDebugMsgTypes( ERROR | MESH_STATUS | CONNECTION | SYNC | COMMUNICATION | GENERAL | MSG_TYPES | REMOTE ); // all types on
  mesh.setDebugMsgTypes( ERROR | STARTUP );  // set before init() so that you can see startup messages

  mesh.init( MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT );
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  userScheduler.addTask(taskSendMessage);
  taskSendMessage.enable();
}

void loop() {
  Serial.println(mesh.getNodeId());
  Serial.print("Temperature: ");
  Serial.print(dht.readTemperature());
  Serial.println(" C");
  Serial.print("Humidity: ");
  Serial.print(dht.readHumidity());
  Serial.println(" %");
// it will run the user scheduler as well
  delay(1000);
  mesh.update();
}