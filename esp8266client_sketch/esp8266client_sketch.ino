#include <SoftwareSerial.h>
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <ArduinoJson.h>
#include <WiFiClient.h>

#define D5 (14)
#define D6 (12)

#ifndef STASSID
#define STASSID "yourssidwifi"
#define STAPSK  "yourssidpw"
#endif

const int maxJson = 2048;
const char* serverUrl = "http://yourserverurl:6080/api";
//rest api config
const char* uuidDevice = "290fb27e-242f-11ec-9621-0242ac130002"; //id of your device in rest api
const char* stringPayloadRandom = "abcdefghijklmnopqrstuvwxyz0123456789";
size_t stringLen = strlen(stringPayloadRandom);  
const int randomUidMaxLenght = 30; 

SoftwareSerial ArduSerial; // RX | TX
ESP8266WiFiMulti WiFiMulti;

void setup() 
{
    Serial.begin(9600);
    ArduSerial.begin(9600, SWSERIAL_8N1, D5, D6, false, 256);
    WiFi.mode(WIFI_STA);
    WiFiMulti.addAP(STASSID, STAPSK);
    Serial.println("Connecting...");
    for (int x = 4; x > 0; x--) {
      Serial.printf("[RunUP] WAIT %d...\n", x);
      Serial.flush();
      delay(1000);
    }
}
 
void loop() 
{
  Serial.flush();
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    Serial.println(WiFi.localIP());
    //check for an incoming action
    if(ArduSerial.available() > 0){
      String currentSerialDataAction = getDataFromSerial();
      DynamicJsonDocument responseJson(maxJson);
      DeserializationError errorJson = deserializeJson(responseJson, currentSerialDataAction);
      //send http put
      if (errorJson == DeserializationError::Ok) {
        Serial.print(currentSerialDataAction);
        DynamicJsonDocument actionJson(maxJson);
        //create json for action 
        actionJson["Id"] = 0;
        actionJson["Uid"] = CreateRandomString();
        actionJson["Payload"] = responseJson;
        actionJson["DeviceID"] = uuidDevice;
        //serialize json
        char actionJsonSerial[maxJson];
        int byteSerializatedAction = serializeJson(actionJson, actionJsonSerial);
        //check for error
        if (byteSerializatedAction > 0) {
          //send action
          sendPutRequestAction(actionJsonSerial);
          delay(500);
        }
      }
    }
    //check for an incoming command
    String checkCommandIncoming = sendGetRequestCommand();
    if(checkCommandIncoming.length() > 0){
      Serial.println(checkCommandIncoming);//Print the response payload 
      DynamicJsonDocument responseJson(maxJson);
      DeserializationError errorJson = deserializeJson(responseJson, checkCommandIncoming);
      //check for error++++++++++
      if (!errorJson) {
        const char* payloadCommand = responseJson["Payload"];
        Serial.println(payloadCommand);
        ArduSerial.write(payloadCommand);
        delay(500);
      }
    }
    //update device status
    ArduSerial.write("info/");
    if(ArduSerial.available() > 0){
      String currentSerialDataStats = getDataFromSerial();
      DynamicJsonDocument responseJsonStats(maxJson);
      DeserializationError errorJson = deserializeJson(responseJsonStats, currentSerialDataStats);
      if(errorJson == DeserializationError::Ok){
        //create json object --- json serialize
        DynamicJsonDocument statsJson(maxJson);
        //create json for action 
        statsJson["Payload"] = responseJsonStats;
        statsJson["DeviceID"] = uuidDevice;
        //serialize json
        char statsJsonSerial[maxJson];
        int byteSerializatedStats = serializeJson(statsJson, statsJsonSerial);
        //check for error
        if(byteSerializatedStats > 0){
          //send stats
          sendPutRequestStats(statsJsonSerial);
          delay(500);
        }
      }
    }
  } else {
    Serial.println("WiFi Disconnected");
  }
  //delay
  delay(500);
}

String CreateRandomString(){
  String returnValue = "";
  for(int i=0; i < randomUidMaxLenght; i++){
    returnValue = returnValue + String(stringPayloadRandom[random(0,stringLen)]);
  }
  return returnValue;
}

String getDataFromSerial(){
  String tempData = "";
  // read from arduino serial
  while(ArduSerial.available() > 0){
    // read the incoming bytes:
    tempData += String(ArduSerial.read());
  }
  // return data
  return tempData;
}

bool sendPutRequestStats(char* jsonToSend){
  WiFiClient client;
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
   if (http.begin(client, String(serverUrl) + "/Statistics/CreateNow")) {  // HTTP
    Serial.print("[HTTP] PUT...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "5858d977-fce2-4ccc-bfcb-001431bc95f5");
    int httpCode = http.PUT(jsonToSend);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] PUT... code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.printf("[HTTP] PUT... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    return true;
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
    return false;
  }
}

String sendGetRequestCommand(){
  WiFiClient client;
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
   if (http.begin(client, String(serverUrl) + "/Command/GetLastToExecute/" + String(uuidDevice))) {  // HTTP
    Serial.print("[HTTP] GET...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "5858d977-fce2-4ccc-bfcb-001431bc95f5");
    int httpCode = http.GET();
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] GET... code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println(payload);
      if(httpCode == 200){
        return payload;
      } else {
        return "";
      }
    } else {
      Serial.printf("[HTTP] GET... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
  }
  return "";
}

bool sendPutRequestAction(char* jsonToSend){
  WiFiClient client;
  HTTPClient http;
  Serial.print("[HTTP] begin...\n");
   if (http.begin(client, String(serverUrl) + "/Actions/Create")) {  // HTTP
    Serial.print("[HTTP] PUT...\n");
    // start connection and send HTTP header
    http.addHeader("Content-Type", "application/json");
    http.addHeader("Authorization", "5858d977-fce2-4ccc-bfcb-001431bc95f5");
    int httpCode = http.PUT(jsonToSend);
    // httpCode will be negative on error
    if (httpCode > 0) {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTP] PUT... code: %d\n", httpCode);
      String payload = http.getString();
      Serial.println(payload);
    } else {
      Serial.printf("[HTTP] PUT... failed, error: %s\n", http.errorToString(httpCode).c_str());
    }
    http.end();
    return true;
  } else {
    Serial.printf("[HTTP} Unable to connect\n");
    return false;
  }
}
