#include <EEPROM.h>
#include <LiquidCrystal.h>
LiquidCrystal lcd(8,9,4,5,6,7);

// define some values used by the panel and buttons
int lcd_key     = 0;
int adc_key_in  = 0;
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5
bool btnHELD = false;


const int Coil = 2;
const int Starter = 3;
const int GenStatus = 11; //if(GenStatus == 0) ---> generator is ON
const int RedLight = 12;
//const int Backlight = 13;
bool ActualGenStatus;
int GenStartIterations = 0;//# of times the system tries to start the generator
int inactivityCounter = 0;


int RawVolts = 0;
int BatteryMilliVolts = 0;
int mapValue = 31000;
int SecondTimer = 0;
int Starter_Timer = 0;
int Stop_Timer = 0;
int genStartTime = 25; //We want to change this value
const int TimeDelay = 100;

//Timers
int timer200ms = 0;
int timer2sec = 0;
int timerDelay = 0; //Counter for the delay to reach the DelayTime value

//Keeps track of the menu
int Menu = 5;
char* AutomationStatus = "OFF";
char* ScreenStatus = "ON";
#define ChangeMinVolt 1
#define ChangeMaxVolt 2
#define ChangeDelay 3
#define ChangeStartDelay 4

//Values to change
int MinimumVoltage = 11500;
int MaximumVoltage = 13500;
int DelayTime = 1 * 10; //The delay for the coils to start again ( seconds * 10)
//EEPROM to store the values into the memory
int addr = 0;

//Keeps track of the status of the signals
bool CoilStatus = true;
bool StarterStatus = true; 
int buttonPressed = 1;
int prevButtonPressed = 1;

//For setting the GenStartDelay while device is running
int DelayHolder = 0;


void setup() {
  pinMode(Coil,OUTPUT);
  pinMode(Starter,OUTPUT);
  pinMode(GenStatus,INPUT_PULLUP);
  pinMode(RedLight,OUTPUT);
  //pinMode(Backlight,OUTPUT);
  Serial.begin(9600);
  lcd.begin(16,2);

  //Both Coil and Starter must be OFF at first
  digitalWrite(Coil, HIGH);
  digitalWrite(Starter, HIGH);

  ReadFromMemory(); //Uses EEPROM read to get values from memory
}

void loop() {
  checkForInactivity();
  
  if(Menu <= 4)
  {
    while(Menu <= 4)
    {
      DisplayParams();
      delay(TimeDelay);
    }
  } 
  else 
  {
    readVolts(); //Continuously reads the voltage
    delay(TimeDelay);
  }
}


void readVolts() //Function for reading the voltage
{
  lcd.setCursor(0,0);
  lcd_key = read_LCD_buttons();
  RawVolts = analogRead(A1); //Reads the voltage from the generator
  BatteryMilliVolts = map(RawVolts,0,1023,0,mapValue); //Value is assigned to the variable

  adjustButtonSensitivity();
  if(btnHELD == true)
  {
    btnHELD = false;
    return;
  }

  if(lcd_key == btnUP || lcd_key == btnDOWN)
  {
    if(AutomationStatus == "ON")
      AutomationStatus = "OFF";
    else if(AutomationStatus == "OFF")
      AutomationStatus = "ON";
  }

  if(lcd_key == btnLEFT || lcd_key == btnRIGHT)
  {
    Menu = 1;
    DisplayParams();
  }
  
  ActualGenStatus = !(digitalRead(GenStatus));
  Starter_Timer++;
  SecondTimer++;
  if (SecondTimer == 10)
   {
    
    lcd.print("Battery = ");
    lcd.print(BatteryMilliVolts / 1000);
    lcd.print(".");
    lcd.print((BatteryMilliVolts % 1000) / 100);
    lcd.print("V");
    SecondTimer = 0;
   }

  if(!(CoilStatus == false and StarterStatus == false))
  {
    if(lcd_key == btnUP)
   {
    DelayHolder += 10;
   } else if(lcd_key == btnDOWN)
   {
    DelayHolder -= 10;
   }
  }
  //additional feature to change the start time while the device is running
  if(CoilStatus == false and StarterStatus == false)
  {
    if(genStartTime != DelayHolder)
    {
      genStartTime = DelayHolder;
    }
  }
  lcd.setCursor (0, 1);
  lcd.print("Automation: ");
  lcd.print(AutomationStatus);
  lcd.print(" ");

  if(AutomationStatus == "OFF")
  {
    GenStartIterations = 0;
    return;
  }
  
  if(BatteryMilliVolts < MinimumVoltage && ActualGenStatus == 0) {
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

      digitalWrite(Starter,LOW); //turn on starter
      StarterStatus = false;
      //delay 2 seconds using timer2sec
      if(timer2sec < genStartTime) //Delay works
      {
        timer2sec++;//this delays 2 seconds for starter
        return; //Using Return since its a function and can be changed to "break" when its in the loop
      }
            
      digitalWrite(Starter,HIGH);//turn off starter
      StarterStatus = true;

      //digitalWrite(RedLight, HIGH);//Testing purposes
      
      
      //delay 15 seconds using timer30sec
      if(timerDelay < DelayTime)
      {
        timerDelay++;
        return;
      }
      ActualGenStatus = !(digitalRead(GenStatus));//read generator status (on or off)
      if(ActualGenStatus == 1){
        GenStartIterations = 0;
        return;
      }
      
      GenStartIterations++;
      
      timer200ms = 0; //Resets the value of 1st timer 
      timer2sec = 0;
      timerDelay = 0;
      
      
    }//bracket for while loop

    if(ActualGenStatus == 0 && GenStartIterations == 3){
    //KILL THE PROGRAM, END IT!!!//turn red light on!
    digitalWrite(Coil, HIGH); //Coil is off
    CoilStatus = true;
    digitalWrite(RedLight, HIGH); //Turns on Red LED
      while(1){ 
       
          StarterFailure();
        }; //Stops the program
    }
    
    
    GenStartIterations = 0;
    Starter_Timer = 0;
  }  else if(BatteryMilliVolts > MaximumVoltage && ActualGenStatus == 1) {
     GenStartIterations = 0;
     digitalWrite(Coil, HIGH);
     CoilStatus = true;
     digitalWrite(Starter, HIGH); 
     StarterStatus = true;
    // Stop Generator 
  }
    ActualGenStatus = !(digitalRead(GenStatus));
    if(ActualGenStatus == 1){
       digitalWrite(Starter, HIGH);
       StarterStatus = true;
       GenStartIterations = 0;
    }
}

//Displays a message to let the user know that the system failed
void StarterFailure()
{
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Starter Failure!");
  lcd.setCursor(0,1);
  lcd.print("Reset System...");
  delay(1000);
}

void DisplayParams()
{
  lcd.clear();
  lcd.setCursor(0,0);

  lcd_key = read_LCD_buttons();

  adjustButtonSensitivity();
  if(btnHELD == true)
  {
    btnHELD = false;
    return;
  }

  if(lcd_key == btnSELECT)
  {
    Menu = 5;
  }
  
  if(Menu == ChangeMinVolt)
  {
    lcd.print("Min Volt: ");
    lcd.setCursor(0,1);
    lcd.print(MinimumVoltage/1000);
    lcd.print(".");
    lcd.print((MinimumVoltage%1000) / 100);
    lcd.print("V");
    if(lcd_key == btnUP) //Increases the Minimum Voltage by 100
    {
      MinimumVoltage = MinimumVoltage + 100;
      if(MinimumVoltage > 12000) //Ceiling
      {MinimumVoltage = 12000;}      
    } else if (lcd_key == btnDOWN) //Decreases the Minimum Voltage by 100
    {
      MinimumVoltage = MinimumVoltage - 100;
      if(MinimumVoltage < 11000) //Floor
      {MinimumVoltage = 11000;}
    } else if (lcd_key == btnRIGHT) //Goes to the next value to change
    {
      Menu++; //Menu is now equals to 2
    }
  } else if (Menu == ChangeMaxVolt)
  {
    lcd.print("Max Volt: ");
    lcd.setCursor(0,1);
    lcd.print(MaximumVoltage/1000);
    lcd.print(".");
    lcd.print((MaximumVoltage%1000)/ 100 );
    lcd.print("V");
    if(lcd_key == btnUP) //Increases the Maximum Voltage by 100
    {
      MaximumVoltage = MaximumVoltage + 100;
      if(MaximumVoltage > 14000) //Ceiling
      {MaximumVoltage = 14000;}
    } else if (lcd_key == btnDOWN) //Decreases the Minimum Voltage by 100
    {
      MaximumVoltage = MaximumVoltage - 100;
      if(MaximumVoltage < 13000) //Floor
      {MaximumVoltage = 13000;}
    } else if (lcd_key == btnRIGHT) //Goes to ChangeDelay
    {
      Menu++; //Menu is now equals to 3
    } else if (lcd_key == btnLEFT) //Goes back to prev value to change
    {
      Menu--; //Menu is now equals to 2
    }
  } else if(Menu == ChangeDelay)
  {
  //The actual delay time should be 20 - 60 seconds
  //We are changing it for testing purposes
    
    lcd.print("Delay Time: ");
    lcd.setCursor(0,1);
    lcd.print(DelayTime / 10); //Displays it in seconds
    lcd.print("s");
    if(lcd_key == btnUP)
    {
      DelayTime = DelayTime + 10;
    } else if(lcd_key == btnDOWN)
    {
      DelayTime = DelayTime - 10;
    } else if(lcd_key == btnRIGHT)
    {
      Menu++; //Menu is now equal to 4
      lcd.clear();
    } else if(lcd_key == btnLEFT)
    {
      Menu--; //Menu is now equal to 3
    }
  } else if(Menu == ChangeStartDelay)
  {
  //The actual genStartTime should be 2 - 3 seconds in increments of 0.1 seconds
  //We are changing it for testing purposes
    lcd.print("Gen Start Time: ");
    lcd.setCursor(0,1);
    lcd.print(genStartTime / 10);
    lcd.print(".");
    lcd.print(genStartTime % 10);
    lcd.print("s");
    if(lcd_key == btnUP)
    {
      genStartTime = genStartTime + 1;
      if(genStartTime > 30) //Ceiling 
      {genStartTime = 30;}
    } else if(lcd_key == btnDOWN)
    {
      genStartTime = genStartTime - 1;
      if(genStartTime < 20)//Floor
      {genStartTime = 20;}
    } else if(lcd_key == btnRIGHT)
    {
      Menu++; //Menu is now equal to 5 means that it will go to readVoltage()
      WriteToMemory();
      DelayHolder = genStartTime;
      lcd.clear();
    } else if(lcd_key == btnLEFT)
    {
      Menu--; //Menu is now equal to 3
    }
  }
}

//Writes the values into EEPROM
void WriteToMemory()
{
  int addr = 0;
  EEPROM.put(addr, (float)MinimumVoltage / 1000);
  
  addr = 5;
  EEPROM.put(addr, (float)MaximumVoltage / 1000);

  addr = 10;
  EEPROM.put(addr, (float)DelayTime / 10);

  addr = 15;
  EEPROM.put(addr, (float)genStartTime / 10);
}


//Reads the values from memory EEPROM Read and assigns the values to corresponding variables\
//Very straigtforward code as of right now
void ReadFromMemory()
{
  float fminv = 11.9f;
  float fmaxv = 13.4f;
  float fdelt = 2.00f;
  float fgensd = 5.00f;

  int address = 0;
  
  //address 0 has Minimum Voltage
  address = 0;
  EEPROM.get(address, fminv);
  MinimumVoltage = (int)(fminv * 1000);
  
  address = 5;
  EEPROM.get(address, fmaxv);
  MaximumVoltage = (int)(fmaxv * 1000);

  address = 10;
  EEPROM.get(address, fdelt);
  DelayTime = (int)(fdelt * 10);

  address = 15;
  EEPROM.get(address, fgensd);
  genStartTime = (int)(fgensd * 10);
}

// read the buttons
int read_LCD_buttons()
{
 adc_key_in = analogRead(0);      // read the value from the sensor 
 if (adc_key_in > 1000) return btnNONE;
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  

 return btnNONE;  // when all others fail, return this...
}


bool adjustButtonSensitivity()
{
  //btnNONE - indicates if no buttons are pressed
  if(lcd_key == btnNONE)
  {
    prevButtonPressed = buttonPressed;
    buttonPressed = 1;
  } 
  else 
  {
    prevButtonPressed = buttonPressed;
    buttonPressed = 0;
  }
  //dont let the holding the button change values rapidly
  if(prevButtonPressed == 0 && buttonPressed == 0)
  {
   btnHELD = true;
  }
}


void checkForInactivity()
{
  lcd_key = read_LCD_buttons();
  if(lcd_key == btnNONE && inactivityCounter < 5000 && ScreenStatus == "ON") //waiting for inactivity
  {
    inactivityCounter++;
    
    if(inactivityCounter >= 5000) //inactivity threshold reached
    {
      lcd.noDisplay(); //turn screen off
      ScreenStatus = "OFF";
      inactivityCounter = 0;
    }
    else
    {
      lcd.display(); //turn screen on
      ScreenStatus = "ON";
    }
  }
  else if(lcd_key != btnNONE)
  {
    lcd.display(); //turn screen on
    ScreenStatus = "ON";
  }
}
