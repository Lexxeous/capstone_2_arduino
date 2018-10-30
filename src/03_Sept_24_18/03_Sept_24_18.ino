
#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);
const int Coil = 2;
const int Starter = 3;
const int GenStatus = 11;//=0 ---> gen is on
const int RedLight = 12; //Red light 
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
int genStartTime = 2;



int timer200ms = 0;
int timer2sec = 0;
int timerDelay = 0; //Counter for the delay to reach the DelayTime value
int DelayTime = 15 * 10; //The delay for the coils to start again

//Keeps track of the status of the signals
bool CoilStatus;
bool StarterStatus; 


void setup() {
  pinMode(2,OUTPUT);
  pinMode(3,OUTPUT);
  pinMode(12,OUTPUT); //Red LED
  pinMode(11,INPUT_PULLUP);
  Serial.begin(9600);
  lcd.begin(16,2);
  lcd.setCursor(0,0);
  lcd.print("Battery= ");
  lcd.setCursor(14,0);
  lcd.print("mV");
  lcd.setCursor(0,1);
  lcd.print("StartTime= ");
  lcd.print(genStartTime);
  lcd.print("s");

  //Both Coil and Starter must be OFF at first
  digitalWrite(Coil, HIGH);
  digitalWrite(Starter, HIGH);

}

void loop() {
 readVolts(); //Continuously reads the voltage
 
 delay(TimeDelay);
}



//Function for reading the voltage
void readVolts()
{
  RawVolts = analogRead(A1); //Reads the voltage from the generator
  BatteryMilliVolts = map(RawVolts,0,1023,0,mapValue); //Value is assigned to the variable
  
  ActualGenStatus = !(digitalRead(GenStatus));
  Starter_Timer++;
  
  if(BatteryMilliVolts < 11500 && ActualGenStatus == 0) {
    // Start Generator

    if(CoilStatus == true)
    {
      digitalWrite(Coil,LOW);//sets coil hot at 12 Volts
      CoilStatus = false;
    }
    //delay 200 ms
    if(timer200ms < 2) //This should delay it by 200ms (Works)
    {
      timer200ms++;
      return; //Using Return since its a function and can be changed to "break" when its in the loop
    }
    
    
    //see if generator is on
    //by checking ActualGenStatus from the beginning
    
    while(ActualGenStatus == 0 && GenStartIterations <= 2){
      ActualGenStatus = !(digitalRead(GenStatus));//Checks the generator if its on

      
      digitalWrite(Starter,LOW);
      StarterStatus = false;
      //delay 2 seconds using timer2sec
      if(timer2sec < 20) //Delay works
      {
        timer2sec++;
        return; //Using Return since its a function and can be changed to "break" when its in the loop
      }
            
      digitalWrite(Starter,HIGH);//turn off starter
      StarterStatus = true;

      digitalWrite(RedLight, HIGH);//Testing purposes
      
      GenStartIterations++; 
      //delay 15 seconds using timer30sec
      if(timerDelay < DelayTime)
      {
        timerDelay++;
        return;
      }

      
      
      
    }//bracket for while loop

    if(ActualGenStatus == 0 && GenStartIterations == 3){
    //KILL THE PROGRAM, END IT!!!//turn red light on!
    digitalWrite(RedLight, HIGH); //Turns on Red LED
    while(1){ }; //Stops the program
    }
    
    
    GenStartIterations = 0;
    Starter_Timer = 0;
  }  else if(BatteryMilliVolts > 14000 && digitalRead(GenStatus) == 0) {
    // Stop Generator 
  }
  
 timer200ms = 0; //Resets the value of 1st timer
 timer2sec = 0;
 timerDelay = 0;
 
 SecondTimer++;
 if (SecondTimer == 10)
   {
     lcd.setCursor(9,0);
     lcd.print(BatteryMilliVolts);

     SecondTimer = 0;
   }
}
