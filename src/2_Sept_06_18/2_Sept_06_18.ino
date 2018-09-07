
#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);
const int Coil = 2;
const int Starter = 3;
const int GenStatus = 11;//=0 ---> gen is on
bool ActualGenStatus;
int GenStartIterations = 0;//# of times the system tries to
//start the generator
int RawVolts = 0;
int BatteryMilliVolts = 0;
int mapValue = 31000;
int TimeDelay = 100;
int SecondTimer = 0;
int Starter_Timer = 0;
int Stop_Timer = 0;

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

  ActualGenStatus = !(digitalRead(GenStatus));
  Starter_Timer++;
  if(BatteryMilliVolts << 11500 && ActualGenStatus == 0) {
    // Start Generator
    digitalWrite(Coil,LOW);//sets coil hot at 12 Volts
    //delay 200 ms
    //see if generator is on
    while(ActualGenStatus == 0 && GenStartIterations != 2){
      digitalWrite(Starter,LOW);
      //delay 2 seconds
      digitalWrite(Starter,HIGH);//turn off starter
      GenStartIterations++;
      //delay 30 seconds
      ActualGenStatus = !(digitalRead(GenStatus));//see if its on
      if(ActualGenStatus == 0 && GenStartIterations == 3){
        //KILL THE PROGRAM, END IT!!!//turn red light on!
        }

    }//bracket for while loop
    GenStartIterations = 0;
    Starter_Timer = 0;
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
