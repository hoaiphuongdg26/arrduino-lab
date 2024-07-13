#include <Adafruit_Sensor.h>
#include <Wire.h>
#include <BH1750.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>

// MESH Details
#define MESH_PREFIX "GROUP3"         // name for your MESH
#define MESH_PASSWORD "GROUP3password" // password for your MESH
#define MESH_PORT 5555                 // default port

BH1750 lightMeter;

//Number for this node
int nodeNumber = 2;

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
  data["light"] = lightMeter.readLightLevel();
  
  jsonData["data"] = data;
  
  readings = JSON.stringify(jsonData);
  return readings;
}

void sendMessage () {
  String msg = getReadings();
  mesh.sendBroadcast(msg);
}

//Init BH1750
void initSensor(){
 if (!lightMeter.begin()) {
    Serial.println("Could not find a valid BH1750 sensor, check wiring!");
    while (1);
  }  
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

void setup() {
  Serial.begin(115200);
  
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
  float lux = analogRead(A0);
  Serial.print("Light: ");
  Serial.print(lux);
  Serial.println(" lx");
  delay(1000);
  // it will run the user scheduler as well
  mesh.update();
}