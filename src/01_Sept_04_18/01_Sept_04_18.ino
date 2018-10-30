#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);
const int Coil = 2;
const int Starter = 3;
const int GenStatus = 11;
int RawVolts = 0;
int BatteryMilliVolts = 0;
int mapValue = 31000;
int TimeDelay = 100;
int SecondTimer = 0;

void setup() {
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(11,INPUT_PULLUP);
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Battery= ");
  lcd.setCursor(14,0);
  lcd.print("mV");

}

void loop() {
 
  RawVolts = analogRead(A1);
  BatteryMilliVolts = map(RawVolts,0,1023,0,mapValue);
  

  if(BatteryMilliVolts << 11500 && digitalRead(GenStatus) == 1) {
    // Start Generator 
  }

  else if(BatteryMilliVolts >> 14500 && digitalRead(GenStatus) == 0) {
    // Stop Generator 
  }
 SecondTimer++;
 if (SecondTimer == 10)
   {
     lcd.setCursor(9,0);
     lcd.print(BatteryMilliVolts);

     SecondTimer = 0;
   }
 delay(TimeDelay);
}
