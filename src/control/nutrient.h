#include <Arduino.h>
class Nutrient;

#define ONE_DOSE_MILLISECOND 1000

/*
  Nutrient for control the EC value
  
  Relay : NO, COM
  Supply : 220V
  
  By conducting repetitive measuring , it is found that 1 sec is for 1 ml(or slightly less) 
*/
class Nutrient
{
	// Class Member Variables
	// These are initialized at startup
	uint8_t pin;

	// These maintain the current state

  public:
  Nutrient(uint8_t input) {
	  pin = input;
    pinMode(pin, OUTPUT);
    digitalWrite(pin , HIGH);
  }

  void injectOneDose() {
    digitalWrite(pin , LOW);
    Serial.println("양액 한단위 투입");
    delay(ONE_DOSE_MILLISECOND);
    digitalWrite(pin , HIGH);

  }

  void injectDoses(int doses) {
    digitalWrite(pin , LOW);
    Serial.println("양액 " + String(doses) + "단위 투입");

    // It is from the guideline based on nutrient instruction that 
    // the nutrient liquid must be diluted with water(at least 400 times)
    delay(doses*ONE_DOSE_MILLISECOND);
    digitalWrite(pin , HIGH);
  }

};
