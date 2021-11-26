#include <circumstance/temp_humdity_sensor.h>
#include <circumstance/photo_resistor.h>
#include <circumstance/co2_sensor.h>
#include <water/water_temp_sensor.h>
#include <water/ec_sensor.h>
#include <water/ph_sensor.h>
#include <water/water_level_sensor.h>
#include <control/nutrient.h>
#include <control/solenoid_valve.h>
#include <LiquidCrystal_I2C.h>
#include <Wire.h>
#include <Arduino.h>
#include <SimpleTimer.h>
#include <ArduinoHttpClient.h>
#include <WiFiNINA.h>
#include <ArduinoJson.h>
#include <arduino_secrets.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <BH1750.h>
#include <constants.h>
#include <endpoint_generator.h>
// Define pins for sensors
#define WATER_LEVEL_HIGH_PIN 12
#define WATER_LEVEL_LOW_PIN 13
#define NUTRIENT_RELAY_PIN 8 
#define SOLENOID_RELAY_PIN 7 
#define LED_RELAY_PIN 9 // ch3

// Define error num
#define SOLENOID_VALVE_ERROR_NUM 240

// Create instance from the sensor classes
BH1750 lightMeter(0x23);
LiquidCrystal_I2C lcd(0x27,20,4); //
PhotoResistor photoResistor();
Co2Sensor co2Sensor; //Tx: 2, Rx: 3
TempHumditySensor  tempHumditySensor; //6 
WaterTemperatureSensor waterTemperatureSensor; // D6
PHSensor pHsensor(A2); 
ECSensor eCSensor; // Tx: 5, Rx: 4
WaterLevelSensor waterLevelSensor(WATER_LEVEL_LOW_PIN, WATER_LEVEL_HIGH_PIN);
SolenoidValve solenoidValve(SOLENOID_RELAY_PIN);
Nutrient nutrient(NUTRIENT_RELAY_PIN);

/*
  This part define constant for controlling ec value
*/
// 20 min set for calling function 
const long intervalForEcFunction =  1200;

double ec = 2.0;
double ecHighBoundary = ec*0.9;
double ecLowBoundary = ec*1.1;
char* turnOn = "16:22";
char* turnOff = "16:23";
char* _id;
// This aim for count 
unsigned int waterCnt;

void controlEc() {
  Serial.println("controlEC!");
  float ecValue = eCSensor.getEC();
  float difference;
  if(ecValue < ecLowBoundary && waterLevelSensor.getWaterLevel() != WATER_LEVEL_LOW){
    difference = ecValue - ec;
    if(difference> 1.6){
      nutrient.injectDoses(8);
    } else if (difference > 1.0){
      nutrient.injectDoses(4);
    } else {
      nutrient.injectOneDose();
    }
  } else if (ecValue > ecHighBoundary && waterLevelSensor.getWaterLevel() != WATER_LEVEL_HIGH){
    difference = ecValue - ec;
    if(difference> 1.0){
      //TODO: 3.0이상이다..?
      solenoidValve.open();
      delay(3000);
      solenoidValve.close();
    } else {
      solenoidValve.open();
      delay(1000);
      solenoidValve.close();
    }
  } else {
    Serial.println("EC Between Boundary or Something WRONG!!");
  }
}
/*
  This part for http communication
*/
EndpointGenerator endpointGenerator;
WiFiServer server(80); 
WiFiClient wifiClient = server.available();
HttpClient httpClientToCloud = HttpClient(wifiClient, SECRET_CLOUD_ADDRESS, SECRET_HTTP_PORT);
HttpClient httpClientToESP32 = HttpClient(wifiClient, SECRET_CAMERA_IP, SECRET_HTTP_PORT);
IPAddress localIP;
const String CONTENT_JSON = "application/json";
int status = WL_IDLE_STATUS;

void connectToServer() {
  while (status != WL_CONNECTED) {
    Serial.print("Attempting to connect to Network named: ");
    Serial.println(SECRET_SSID);
    status = WiFi.begin(SECRET_SSID, SECRET_PASSWORD);
    delay(10000);
  }
  // WiFi.config(IPAddress(172,30,1,42));
  Serial.print("SSID: ");
  Serial.println(WiFi.SSID());
  localIP = WiFi.localIP();
  IPAddress gateway = WiFi.gatewayIP();
  Serial.print("IP Address: ");
}

void initializeArduinoFromDatabase() {
  StaticJsonDocument<300> jsonDoc;
  String payload;
  Serial.println("making POST request");
  jsonDoc["id"] = SECRET_PLANTER_ID;
  jsonDoc["publicIP"] = SECRET_PUBLIC_IP;
  jsonDoc["port"] = SECRET_PUBLIC_PORT;
  jsonDoc["localIP"] =  String(localIP[0]) + String(".") +\
  String(localIP[1]) + String(".") +\
  String(localIP[2]) + String(".") +\
  String(localIP[3]);
  serializeJson(jsonDoc,payload);
  Serial.println(payload);
  Serial.println(endpointGenerator.getInitPath());
  httpClientToCloud.post(endpointGenerator.getInitPath(),CONTENT_JSON,payload);
  int statusCode = httpClientToCloud.responseStatusCode();
  String body = httpClientToCloud.responseBody();
  Serial.print("Status code: ");
  Serial.println(statusCode);
  Serial.print("Response: ");
  Serial.println(body);

  DeserializationError error = deserializeJson(jsonDoc, body);

  // Test if parsing succeeds.
  if (error) {
    Serial.print(F("deserializeJson() failed: "));
    Serial.println(error.c_str());
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("INIT ERROR");
    lcd.setCursor(0,1);
    lcd.print(statusCode);

    return;
  }
  ec = jsonDoc["setting"]["ec"];
  turnOn = jsonDoc["setting"]["turnOn"];
  turnOff = jsonDoc["setting"]["turnOff"];
  _id = jsonDoc["_id"];
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("INIT Success");
  lcd.setCursor(0,1);
  lcd.print(statusCode);
  delay(500);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Current Setting");
  lcd.setCursor(0,1);
  lcd.print("EC: ");
  lcd.print(ec);
  lcd.setCursor(0,2);
  lcd.print("Turn On: ");
  lcd.print(turnOn);
  lcd.setCursor(0,3);
  lcd.print("Turn Off: ");
  lcd.print(turnOff);
  delay(1000);
}


void uploadDataToCloud() {
  StaticJsonDocument<500> jsonDoc;
  String payload;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Req to Cam");
  httpClientToESP32.get("/getImage");
  lcd.setCursor(0,1);
  lcd.print(httpClientToESP32.responseStatusCode());

  float currentTemp  = tempHumditySensor.getTemperature();
  float currentRelativeHumidity  = tempHumditySensor.getRelativeHumidity();
  float currentLux = lightMeter.readLightLevel();
  float currentPPM = co2Sensor.getPPM();
  float currentWaterTemp = waterTemperatureSensor.getWaterTemperature();
  float currentPHAvg = pHsensor.getPHAvg();
  float currentEC = eCSensor.getEC();
  jsonDoc["planter"] =  _id;
  jsonDoc["planterId"] =  SECRET_PLANTER_ID;
  jsonDoc["date"] =  WiFi.getTime() + TIME_DIFFERENCE_FROM_UTC;
  jsonDoc["image"]["data"] = httpClientToESP32.responseBody();
  jsonDoc["image"]["contentType"] = "image/base64"; 
  jsonDoc["temperature"] =  currentTemp;
  jsonDoc["humidity"] =  currentRelativeHumidity;
  jsonDoc["lux"] =  currentLux;
  jsonDoc["co2ppm"] =  currentPPM;
  jsonDoc["waterTemperature"] =  currentWaterTemp;
  jsonDoc["ph"] =  currentPHAvg;
  jsonDoc["ec"] =  currentEC;

  serializeJson(jsonDoc,payload);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Req to cloud");
  httpClientToCloud.post(endpointGenerator.getSnapshotsPath(),"content-type: application/json",payload);
  lcd.setCursor(0,1);
  lcd.print(httpClientToCloud.responseStatusCode());
  int statusCode = httpClientToCloud.responseStatusCode();
  Alarm.delay(2000);
}


void handleRequestFromServer() { 
    StaticJsonDocument<200> jsonDoc;                           
    Serial.println("new wifiClient");           // print a message out the serial port
    String currentLine = "";                // make a String to hold incoming data from the wifiClient
    while (wifiClient.connected()) {            // loop while the wifiClient's connected
      if (wifiClient.available()) {             // if there's bytes to read from the wifiClient,
        char c = wifiClient.read();             // read a byte, then
        Serial.write(c);                    // print it out the serial monitor
        if (c == '\n') {                    // if the byte is a newline character
          if (currentLine.length() == 0) {
            // HTTP request는 /n으로 끝난다 따라서, 만약 c가 newline인데 currentLine이 존재하지 않는다면,
            // 모든 리퀘스트 문자를 다 받았다고 전제하고 반복문에서 빠져나간다.
            break;
          }
          else {
            // 새로운 /n으로 끝났으면 새로운 줄이라고 인식해서 currentLine을 비워준다
            currentLine = "";
          }
        }
        else if (c != '\r') {
          // carriage return character가 아니라면, 더해준다
          currentLine += c;
        }
        // //DATA UPLOAD
        // if(currentLine.endsWith("GET /capture")){
        //   httpClientToESP32.get("/getImage");
        //   Serial.println(httpClientToESP32.responseBody());
        //   wifiClient.println("HTTP/1.1 200 OK");
        //   wifiClient.println("Content-type:image/base64");
        //   wifiClient.println(); 
        //   wifiClient.print(httpClientToESP32.responseBody());      
        //   wifiClient.println();
        //  break;
          

        // }
        //REMOTE MONITOR
        if (currentLine.endsWith("GET /monitor")) {
      StaticJsonDocument<500> jsonDoc;
      String payload;
      lcd.clear();
      lcd.setCursor(0,0);
      lcd.print("Req to Cam");
  httpClientToESP32.get("/getImage");
  lcd.setCursor(0,1);
  lcd.print(httpClientToESP32.responseStatusCode());

  float currentTemp  = tempHumditySensor.getTemperature();
  float currentRelativeHumidity  = tempHumditySensor.getRelativeHumidity();
  float currentLux = lightMeter.readLightLevel();
  float currentPPM = co2Sensor.getPPM();
  float currentWaterTemp = waterTemperatureSensor.getWaterTemperature();
  float currentPHAvg = pHsensor.getPHAvg();
  float currentEC = eCSensor.getEC();
  jsonDoc["planter"] =  _id;
  jsonDoc["planterId"] =  SECRET_PLANTER_ID;
  jsonDoc["date"] =  WiFi.getTime() + TIME_DIFFERENCE_FROM_UTC;
  jsonDoc["image"]["data"] = httpClientToESP32.responseBody();
  jsonDoc["image"]["contentType"] = "image/base64"; 
  jsonDoc["temperature"] =  currentTemp;
  jsonDoc["humidity"] =  currentRelativeHumidity;
  jsonDoc["lux"] =  currentLux;
  jsonDoc["co2ppm"] =  currentPPM;
  jsonDoc["waterTemperature"] =  currentWaterTemp;
  jsonDoc["ph"] =  currentPHAvg;
  jsonDoc["ec"] =  currentEC;

          serializeJson(jsonDoc, payload);
          Serial.println("Sending response");
          // send a standard http response header
          wifiClient.println("HTTP/1.1 200 OK");
          wifiClient.println("Content-type:application/json");
          wifiClient.println(); 
          wifiClient.print(payload);      
          wifiClient.println();
          break;
        }
        //REMOTE CONTROL
        if (currentLine.endsWith("POST /initcam")) {
          String body = "";
          bool isBody = false;
          int cnt = 0;
          while(wifiClient.connected() && cnt < 10000){
          cnt++;
          char c = wifiClient.read();
          if(c == '{') isBody = true;
          if(isBody && c != '/n' && c != '/r') body += c;
          if(c == '}') break;
          }
          Serial.println(body);
          DeserializationError error = deserializeJson(jsonDoc, body);

          // Test if parsing succeeds.
          if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          wifiClient.println("HTTP/1.1 401 invaild request");
          wifiClient.println();
          break;
          } else {

          // if(jsonDoc.containsKey("turnOff")) turnOff = jsonDoc["turnOff"];
          // Serial.println(jsonDoc["id"]);
          // Serial.println(jsonDoc["floor"]);
          // Serial.println(jsonDoc["localIP"]);

          // String payload;
          // jsonDoc["ec"] = ec;
          // jsonDoc["turnOn"] = turnOn;
          // jsonDoc["turnOff"] =  turnOff;  
          // serializeJson(jsonDoc, payload);
          Serial.println("Sending response");
          wifiClient.println("HTTP/1.1 200 OK");
          wifiClient.println("Content-type:application/json");
          // wifiClient.println(); 
          // wifiClient.print(payload);
          // wifiClient.println();
          break;
          }
        }
           if (currentLine.endsWith("POST /control")) {
          String body = "";
          bool isBody = false;
          int cnt = 0;
          while(wifiClient.connected() && cnt < 10000){
          cnt++;
          char c = wifiClient.read();
          if(c == '{') isBody = true;
          if(isBody && c != '/n' && c != '/r') body += c;
          if(c == '}') break;
          }
          Serial.println(body);
          DeserializationError error = deserializeJson(jsonDoc, body);

          // Test if parsing succeeds.
          if (error) {
          Serial.print(F("deserializeJson() failed: "));
          Serial.println(error.c_str());
          wifiClient.println("HTTP/1.1 401 invaild request");
          wifiClient.println();
          break;
          } else {
          if(jsonDoc.containsKey("ec")) ec = jsonDoc["ec"];
          if(jsonDoc.containsKey("turnOn"))turnOn = jsonDoc["turnOn"];
          if(jsonDoc.containsKey("turnOff")) turnOff = jsonDoc["turnOff"];
          Serial.println(ec);
          Serial.println(turnOn);
          Serial.println(turnOff);

          String payload;
          jsonDoc["ec"] = ec;
          jsonDoc["turnOn"] = turnOn;
          jsonDoc["turnOff"] =  turnOff;  
          serializeJson(jsonDoc, payload);
          Serial.println("Sending response");
          wifiClient.println("HTTP/1.1 200 OK");
          wifiClient.println("Content-type:application/json");
          wifiClient.println(); 
          wifiClient.print(payload);
          wifiClient.println();
          break;
          }
        }
      }
    }
    // close the connection:
    wifiClient.stop();
    Serial.println("client disconnected");
  }


/*
this part set up timer

the alarm cannot be more than 6.
if u want change go to header file of the alarm and change the number
*/
AlarmID_t ecTimerId;
const long intervalForDataUploadingFunction =  1200;

void turnOnLED() {
  Serial.println("turn on LED");
}
void turnOffLED() {
  Serial.println("turn off LED");
}

void setUpTimer() {
  time_t time = WiFi.getTime() + TIME_DIFFERENCE_FROM_UTC;
  setTime(time);
  Serial.println(hour());
  Serial.println(minute());
  // EC 인터벌 등록!
  ecTimerId =Alarm.timerRepeat(intervalForEcFunction, controlEc);
  // Turn On 등록!
  char* ptr = strtok(turnOn, ":");
  int hour = atol(ptr) - 0;
  int min = atol(strtok(NULL, ":")) - 0;
  Alarm.alarmRepeat(hour,min,0,turnOnLED);
  // Turn Off 등록!
  ptr = strtok(turnOff, ":");
  hour = atol(ptr) - 0;
  min = atol(strtok(NULL, ":")) - 0;
  Alarm.alarmRepeat(hour,min,0,turnOffLED);

  //Send data to cloud server
  Alarm.timerRepeat(intervalForDataUploadingFunction, uploadDataToCloud);
}

void setup() {
  Serial.begin(9600);
  Wire.begin();
  lightMeter.begin();
  lcd.init();         
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Setting Sensor");
  Serial.println();
  tempHumditySensor.checkSensor();
  //TODO: in production mode, it should take a while for hitting co2 sensor up 
  co2Sensor.begin();// Rx for 3, Tx for 2
  eCSensor.begin();
  
  //initialize water count
  waterCnt = 0;
  lcd.setCursor(0,0);
  lcd.print("Sensor done");
  lcd.clear();
  //initialize server and arduino
  lcd.setCursor(0,0);
  lcd.print("connecting to WiFi");
  lcd.clear();
  connectToServer();
  lcd.setCursor(0,0);
  lcd.print("Connected!");
  lcd.setCursor(0,1);
  lcd.print("Local Ip" + WiFi.localIP());
  initializeArduinoFromDatabase();

  setUpTimer();
}



bool controlWater(WaterLevel waterInput){
    bool isInjected = false;

    if(waterInput == WATER_LEVEL_LOW && waterCnt < SOLENOID_VALVE_ERROR_NUM)
    {
      Serial.println("if Loop");
      solenoidValve.open();
      isInjected = true;
    while (waterLevelSensor.getWaterLevel() == WATER_LEVEL_LOW && waterCnt < SOLENOID_VALVE_ERROR_NUM)
    {
      Serial.println("물투입");
      delay(500);
      waterCnt++;
    }
    solenoidValve.close();

    if(waterCnt >= SOLENOID_VALVE_ERROR_NUM)
      {
        //TODO: What should be done when water is empty or sensor went wrong
        // SEND MESSAGE AND FIX! AND RESET THE PROGREM WITH BUTTEN!
      }
    else 
      {
        waterCnt = 0;
      }
  }
  return isInjected;
} 

void loop() {
  /*
  //   This step aim for measuring circumstance 
  */

  float currentTemp  = tempHumditySensor.getTemperature();
  float currentRelativeHumidity  = tempHumditySensor.getRelativeHumidity();
  float currentLux = lightMeter.readLightLevel();
  float currentPPM = co2Sensor.getPPM();


  lcd.setCursor(0,0);
  lcd.print("Temperature: " + String(currentTemp) + 'C');
  lcd.setCursor(0,1);
  lcd.print("Humidity:    " + String(currentRelativeHumidity) + '%');
  lcd.setCursor(0,2);
  lcd.print("Lux:         "+ String(currentLux));
  lcd.setCursor(0,3);
  lcd.print("CO2PPM:     "+ String(currentPPM));
  Alarm.delay(4000);

  /*
  //   This step aim for measuring Water 
  */

  float currentWaterTemp = waterTemperatureSensor.getWaterTemperature();
  float currentPHAvg = pHsensor.getPHAvg();
  WaterLevel currentWaterLevel = waterLevelSensor.getWaterLevel();
  float currentEC = eCSensor.getEC();
  Serial.println(currentEC);

  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("WaterTemp:   " + String(currentWaterTemp) + 'C');
  lcd.setCursor(0,1);
  lcd.print("WaterLevel:    "+ String(waterLevelSensor.printWaterLevel(currentWaterLevel)));
  lcd.setCursor(0,2);
  lcd.print("PH:            " + String(currentPHAvg));
  lcd.setCursor(0,3);
  lcd.print("EC:            "+ String(currentEC));
  Alarm.delay(4000);
 
  /*
  //   This step aim for controlling water
  */
  bool isInjected = controlWater(currentWaterLevel);
  Serial.println(waterCnt);
  if(isInjected){
    Serial.println("Is injected!");
    Alarm.disable(ecTimerId);
    Alarm.enable(ecTimerId);
  } 
  wifiClient = server.available();
  if(wifiClient){
    handleRequestFromServer();
  }
}