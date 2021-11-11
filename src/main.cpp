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

// Define pins for sensors
#define WATER_LEVEL_HIGH_PIN 12
#define WATER_LEVEL_LOW_PIN 13
#define NUTRIENT_RELAY_PIN 7 //ch3
#define SOLENOID_RELAY_PIN 8 //ch2
#define LED_RELAY_PIN 9 // ch3

// Define boundary for EC
#define EC_LOW_BOUNDARY 2.4
#define EC_HIGH_BOUNDARY 2.8

// Define error num
#define SOLENOID_VALVE_ERROR_NUM 240

// Create instance from the sensor classes
LiquidCrystal_I2C lcd(0x27,20,4); //
PhotoResistor photoResistor(A4);
Co2Sensor co2Sensor; //Tx: 2, Rx: 3
TempHumditySensor tempHumditySensor; //6 
WaterTemperatureSensor waterTemperatureSensor; // D6
PHSensor pHsensor(A2); 
ECSensor eCSensor; // Tx: 5, Rx: 4
WaterLevelSensor waterLevelSensor(WATER_LEVEL_LOW_PIN, WATER_LEVEL_HIGH_PIN);
SolenoidValve solenoidValve(SOLENOID_RELAY_PIN);
Nutrient nutrient(NUTRIENT_RELAY_PIN);

/*
  This part define constant for controlling ec value
*/
SimpleTimer timer;
int timerId;
// 20 min set for calling function 
const int intervalForEcFunction =  1200000;
#define EC_LOW_BOUNDARY 1.8
#define EC_HIGH_BOUNDARY 2.2
#define EC_STANDARD 2.0

// This aim for count 
unsigned int waterCnt;

void setup() {
  Serial.begin(9600);
  solenoidValve.close();
  lcd.init();         
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Setting Arduino");
  tempHumditySensor.checkSensor();
  //TODO: in production mode, it should take a while for hitting co2 sensor up 
  co2Sensor.begin();// Rx for 3, Tx for 2
  eCSensor.begin();
  
  //initialize water count
  waterCnt = 0;
  lcd.setCursor(0,0);
  lcd.print("Setting Done!");
  lcd.clear();
  // set interval for controlling ec
  timerId = timer.setInterval(intervalForEcFunction, controlEc);
}

void controlEc() {
  float ecValue = eCSensor.getEC();
  float difference;
  if(ecValue < EC_LOW_BOUNDARY && waterLevelSensor.getWaterLevel() != WATER_LEVEL_LOW){
    difference = ecValue - EC_STANDARD;
    if(difference> 1.6){
      nutrient.injectDoses(8);
    } else if (difference > 1.0){
      nutrient.injectDoses(4);
    } else {
      nutrient.injectOneDose();
    }
  } else if (ecValue > EC_HIGH_BOUNDARY && waterLevelSensor.getWaterLevel() != WATER_LEVEL_HIGH){
    difference = ecValue - EC_STANDARD;
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
    Serial.println("IS Between Boundary or Something WRONG!!");
  }
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
  timer.run();
  /*
  //   This step aim for measuring circumstance 
  */
  float currentTemp  = tempHumditySensor.getTemperature();
  float currentRelativeHumidity  = tempHumditySensor.getRelativeHumidity();
  float currentLux = (photoResistor.getLux(),2);
  float currentPPM = co2Sensor.getPPM();


  lcd.setCursor(0,0);
  lcd.print("Temperature: " + String(currentTemp) + 'C');
  lcd.setCursor(0,1);
  lcd.print("Humidity:    " + String(currentRelativeHumidity) + '%');
  lcd.setCursor(0,2);
  lcd.print("Lux:         "+ String(currentLux));
  lcd.setCursor(0,3);
  lcd.print("CO2PPM:     "+ String(currentPPM));
  delay(4000);

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
  delay(4000);
 
  /*
  //   This step aim for controlling water
  */
  bool isInjected = controlWater(currentWaterLevel);
  Serial.println(waterCnt);
  if(isInjected){
    Serial.println("Is injected!");
    timer.restartTimer(timerId);
  } 
}