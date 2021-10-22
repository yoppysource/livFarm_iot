#include <OneWire.h>
#include <DallasTemperature.h>
#include <MHZ.h>
#include <circumstance/temp_humdity_sensor.h>
#include <circumstance/photo_resistor.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x3F,20,2);
// LiquidCrystal_I2C lcd(0x27,20,4);
// LiquidCrystal_I2C lcd(0x27, 2, 1, 0, 4, 5, 6, 7, 3, POSITIVE);  // Set the LCD I2C address

#define ONE_WIRE_BUS 5

OneWire oneWire(ONE_WIRE_BUS);
 float Celcius=0;
 float Fahrenheit=0;

DallasTemperature sensors(&oneWire);


#define CO2_IN 10
MHZ co2(CO2_IN, MHZ19B);
TempHumditySensor tempHumditySensor;
// Set photoResistor as A4
PhotoResistor photoResistor(A4);

void setup() {
  Serial.begin(9600);
  // Print a message to the LCD.
  // lcd.print("hello, world!");
  lcd.init(); // LCD 초기화 print a message to the LCD
  Serial.println("Init Done");
  
  // lcd.backlight(); // 백라이트 켜기
  // Serial.println("Light on Done");
  // lcd.setCursor(0,0); // 1번째 1 라인
  // lcd.print("Hello World");
  // Serial.println("print done");
  // lcd.setCursor(0,1); // 1번쨰 2라인
  // lcd.print("Enjoy - ADunio");
  // tempHumditySensor.checkSensor();
  // sensors.begin();                              // (Uno example) device to MH-Z19 serial start   
  // pinMode(CO2_IN, INPUT);
  // delay(100);
  // Serial.println("MHZ 19B");

  // enable debug to get addition information
  // co2.setDebug(true);

  // if (co2.isPreHeating()) {
  //   Serial.print("Preheating");
  //   while (co2.isPreHeating()) {
  //     Serial.print(".");
  //     delay(5000);
  //   }
  //   Serial.println();
  // }



}

void loop() {
  // /*
  //   This step aim for measuring circumstance 
  // */
  // // Temperature & humidity
  //   Serial.print("Temp: "); Serial.print(tempHumditySensor.getTemperature()); Serial.print(" C");
  //   Serial.print("\t\t");
  //   Serial.print("Humidity: "); Serial.print(tempHumditySensor.getRelativeHumidity()); Serial.println(" \%");
  // // Lux
  //   Serial.print("Lux : "); Serial.println(photoResistor.getLux());
  // // PPM

  // sensors.requestTemperatures(); 
  // Celcius=sensors.getTempCByIndex(0);
  // Fahrenheit=sensors.toFahrenheit(Celcius);
  // Serial.print(" C  ");
  // Serial.print(Celcius);
  // Serial.print(" F  ");
  // Serial.println(Fahrenheit);

  // int ppm_pwm = co2.readCO2PWM();
  // Serial.print("PPMpwm: ");
  // Serial.println(ppm_pwm);
  // delay(500);

  lcd.setCursor(0,1); // 1번쨰 2라인
  lcd.print("Enjoy - ADunio");

   lcd.backlight(); // 백라이트 켜기
  Serial.println("Light on Done");
  lcd.setCursor(0,0); // 1번째 1 라인
}

// #include <MHZ19.h>

// MHZ19 mhz(&Serial1);

// void setup()
// {
//   Serial.begin(115200);
//   Serial.println(F("Starting..."));

//   Serial1.begin(9600);
// }

// void loop()
// {
//   MHZ19_RESULT response = mhz.retrieveData();
//   if (response == MHZ19_RESULT_OK)
//   {
//     Serial.print(F("CO2: "));
//     Serial.println(mhz.getCO2());
//     Serial.print(F("Temperature: "));
//     Serial.println(mhz.getTemperature());
//     Serial.print(F("Accuracy: "));
//     Serial.println(mhz.getAccuracy());
//   }
//   else
//   {
//     Serial.print(F("Error, code: "));
//     Serial.println(response);
//   }
  
//   delay(15000);
// }