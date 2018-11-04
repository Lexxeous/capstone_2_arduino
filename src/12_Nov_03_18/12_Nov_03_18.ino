//include necessary libraries
#include <EEPROM.h>
#include <LiquidCrystal.h>

//setup LCD
LiquidCrystal lcd(8,9,4,5,6,7);

//define LCD button values
#define btnRIGHT  0
#define btnUP     1
#define btnDOWN   2
#define btnLEFT   3
#define btnSELECT 4
#define btnNONE   5

//keep track of LCD button values
int lcd_key     = 0;
int adc_key_in  = 0;
int buttonPressed = 1; //1 is btnPressed, 0 is btnNotPressed
int prevButtonPressed = 1; //1 is btnPressed, 0 is btnNotPressed
bool btnHELD = false; //default to false

//declare pin numbers for components
const int Coil = 2;
const int Starter = 3;
const int GenStatus = 11; //if(GenStatus == 0), then generator is ON
const int RedLight = 12;
const int Backlight = 10;

//keep track of voltages and counters
int GenStartIterations = 0; //number of starter iterations
int inactivityCounter = 0;
int RawVolts = 0;
int BatteryMilliVolts = 0; //external battery voltage in mV
int mapValue = 31000;

//keep track of timers and delays
int timer200ms = 0;
int timer2sec = 0;
int timerDelay = 0; //counter for the delay to reach "DelayTime" value
int SecondTimer = 0;
int Starter_Timer = 0;
int Stop_Timer = 0;
const int TimeDelay = 100; //universal delay for main loop

//keep track of the menu
int Menu = 5;
#define ChangeMinVolt 1
#define ChangeMaxVolt 2
#define ChangeDelay 3
#define ChangeStartDelay 4

//default user specified system parameters
int MinimumVoltage = 11500; //external battery minimum threshold
int MaximumVoltage = 13500; //external battery maximum threshold
int DelayTime = 300; //time between starter iterations
int genStartTime = 25; //time to crank generator starter

//keep track of EEPROM addresses
int addr = 0;

//keep track of generator signals
bool CoilStatus = true;
bool StarterStatus = true;
bool ActualGenStatus;

//keep track of screen and automation status
char* AutomationStatus = "OFF";
char* ScreenStatus = "ON";

//--------------------------------------------------------------------------------------

void setup() //system setup
{
  //assign pin modes
  pinMode(Coil,OUTPUT);
  pinMode(Starter,OUTPUT);
  pinMode(GenStatus,INPUT_PULLUP);
  pinMode(RedLight,OUTPUT);
  pinMode(Backlight,OUTPUT);

  //initialize baud rate and LCD
  Serial.begin(9600);
  lcd.begin(16,2);

  //default "Coil" and "Starter" to OFF
  digitalWrite(Coil, HIGH);
  digitalWrite(Starter, HIGH);

  ReadFromMemory(); //get system parameters from EEPROM
}

//--------------------------------------------------------------------------------------

void loop() //main loop
{
  checkForInactivity();

  if(Menu <= 4)
  {
    ReadFromMemory(); //get system parameters from EEPROM
    while(Menu <= 4)
    {
      DisplayParams(); //adjust system parameters
      delay(TimeDelay); //main loop runs every 100ms
    }
    WriteToMemory(); //put system parameters into EEPROM
  } 
  else 
  {
    readVolts(); //read external battery voltage
    delay(TimeDelay); //main loop runs every 100ms
  }
}

//--------------------------------------------------------------------------------------

void readVolts()
{
  lcd.setCursor(0,0);
  lcd_key = read_LCD_buttons();
  RawVolts = analogRead(A1); //read voltage from the generator
  BatteryMilliVolts = map(RawVolts,0,1023,0,mapValue); //read external battery in mV

  //dont allow rapid button changes
  adjustButtonSensitivity();
  if(btnHELD == true)
  {
    btnHELD = false;
    return;
  }

  //set "AutomationStatus"
  if((lcd_key == btnUP || lcd_key == btnDOWN) && ScreenStatus == "ON")
  {
    if(AutomationStatus == "ON")
      AutomationStatus = "OFF";
    else if(AutomationStatus == "OFF")
      AutomationStatus = "ON";
  }

  //adjust system parameters
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
    //display external battery voltage in V
    lcd.print("Battery = ");
    lcd.print(BatteryMilliVolts / 1000);
    lcd.print(".");
    lcd.print((BatteryMilliVolts % 1000) / 100);
    lcd.print("V  ");
    SecondTimer = 0;
  }

  //display "AutomationStatus"
  lcd.setCursor (0, 1);
  lcd.print("Automation: ");
  lcd.print(AutomationStatus);
  lcd.print(" ");

  //check "AutomationStatus"
  if(AutomationStatus == "OFF")
  {
    GenStartIterations = 0;
    return;
  }

  //if the generator needs to be turned on
  if(BatteryMilliVolts < MinimumVoltage && ActualGenStatus == 0)
  {
    //try to start generator
    if(CoilStatus == true)
    {
      digitalWrite(Coil,LOW); //sets coil hot at 12 volts
      CoilStatus = false;
    }

    if(timer200ms < 2) //delay 200ms
    {
      timer200ms++;
      return;
    }

    //while still trying to turn on generator
    while(ActualGenStatus == 0 && GenStartIterations <= 2)
    {
      ActualGenStatus = !(digitalRead(GenStatus)); //assign actual generator status
      digitalWrite(Starter,LOW); //turn on starter
      StarterStatus = false;
      
      if(timer2sec < genStartTime) //delay 2 seconds using timer2sec
      {
        timer2sec++; //delay 2 seconds for starter
        return;
      }
            
      digitalWrite(Starter,HIGH); //turn off starter
      StarterStatus = true;
      
      //delay 15 seconds using timer30sec
      if(timerDelay < DelayTime)
      {
        timerDelay++;
        return;
      }
      
      ActualGenStatus = !(digitalRead(GenStatus)); //read generator status
      if(ActualGenStatus == 1)
      {
        GenStartIterations = 0;
        return;
      }
      
      GenStartIterations++; //go to next starter try
      
      //reset timer values
      timer200ms = 0; 
      timer2sec = 0;
      timerDelay = 0;
    }

    if(ActualGenStatus == 0 && GenStartIterations == 3)
    {
      //failed to start generator
      digitalWrite(Coil, HIGH); //set "Coil" to OFF
      CoilStatus = true;
      digitalWrite(RedLight, HIGH); //turn on Red LED
      StarterFailure(); //stops the main loop
    }
    
    //reset starter iterations and timer
    GenStartIterations = 0;
    Starter_Timer = 0;
  }
  else if(BatteryMilliVolts > MaximumVoltage && ActualGenStatus == 1)
  { 
    //stop generator
    GenStartIterations = 0;
    digitalWrite(Coil, HIGH);
    CoilStatus = true;
    digitalWrite(Starter, HIGH); 
    StarterStatus = true; 
  }
  
  ActualGenStatus = !(digitalRead(GenStatus)); //assign actual generator status
  if(ActualGenStatus == 1) //if generator started
  {
     digitalWrite(Starter, HIGH); //turn off starter
     StarterStatus = true;
     GenStartIterations = 0; //reset starter iterations
  }
}

//--------------------------------------------------------------------------------------

void StarterFailure()
{
  while(1) //loop forever
  {
    //display error message
    digitalWrite(Backlight,HIGH);
    lcd.display();
    ScreenStatus = "ON";
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Starter Failure!");
    lcd.setCursor(0,1);
    lcd.print("Reset System...");
    delay(1000);
  }
}

//--------------------------------------------------------------------------------------

void DisplayParams()
{
  //reset and read LCD buttons
  lcd.clear();
  lcd.setCursor(0,0);
  lcd_key = read_LCD_buttons();

  //dont allow rapid button changes
  adjustButtonSensitivity();
  if(btnHELD == true)
  {
    btnHELD = false;
    return;
  }

  //exit if user presses "btnSELECT"
  if(lcd_key == btnSELECT)
  {
    Menu = 5;
  }
  
  if(Menu == ChangeMinVolt) //first system parameter
  {
    //display "MinimumVoltage" parameter
    lcd.print("Min Volt: ");
    lcd.setCursor(0,1);
    lcd.print(MinimumVoltage/1000);
    lcd.print(".");
    lcd.print((MinimumVoltage%1000)/100);
    lcd.print("V");
    
    if(lcd_key == btnUP) //increase "MinimumVoltage" by 100mV
    {
      MinimumVoltage = MinimumVoltage + 100;
      if(MinimumVoltage > 12000) //ceiling
      {MinimumVoltage = 12000;}      
    }
    else if (lcd_key == btnDOWN) //decrease "MinimumVoltage" by 100mV
    {
      MinimumVoltage = MinimumVoltage - 100;
      if(MinimumVoltage < 11000) //floor
      {MinimumVoltage = 11000;}
    }
    else if (lcd_key == btnRIGHT) //go to next parameter ("MaximumVoltage")
    {
      Menu++; //"Menu" = 2
    }
  }
  else if (Menu == ChangeMaxVolt) //second system parameter
  {
    //display "MaximumVoltage" parameter
    lcd.print("Max Volt: ");
    lcd.setCursor(0,1);
    lcd.print(MaximumVoltage/1000);
    lcd.print(".");
    lcd.print((MaximumVoltage%1000)/100 );
    lcd.print("V");
    
    if(lcd_key == btnUP) //increases the "MaximumVoltage" by 100mV
    {
      MaximumVoltage = MaximumVoltage + 100;
      if(MaximumVoltage > 14000) //ceiling
      {MaximumVoltage = 14000;}
    }
    else if (lcd_key == btnDOWN) //decrease "MinimumVoltage" by 100mV
    {
      MaximumVoltage = MaximumVoltage - 100;
      if(MaximumVoltage < 13000) //floor
      {MaximumVoltage = 13000;}
    }
    else if (lcd_key == btnRIGHT) //go to next parameter ("DelayTime")
    {
      Menu++; //"Menu" = 3
    }
    else if (lcd_key == btnLEFT) //go to previous parameter ("MinimumVoltage")
    {
      Menu--; //"Menu" = 2
    }
  }
  else if(Menu == ChangeDelay)
  { 
    //display "DelayTime" parameter
    lcd.print("Delay Time: ");
    lcd.setCursor(0,1);
    lcd.print(DelayTime / 10); //display "DelayTime" in seconds
    lcd.print("s");
    
    if(lcd_key == btnUP) //increase "DelayTime" by 1 second
    {
      DelayTime = DelayTime + 10;
      if(DelayTime > 600) //ceiling
      {DelayTime = 600;}
    }
    else if(lcd_key == btnDOWN) //decrease "DelayTime" by 1 second
    {
      DelayTime = DelayTime - 10;
      if(DelayTime < 200) //floor
      {DelayTime = 200;}
    }
    else if(lcd_key == btnRIGHT) //go to next parameter ("genStartTime")
    {
      Menu++; //"Menu" = 4
      lcd.clear();
    }
    else if(lcd_key == btnLEFT) //go to previous parameter ("MaximumVoltage")
    {
      Menu--;  //"Menu" = 3
    }
  }
  else if(Menu == ChangeStartDelay)
  {
    //display "genStartTime"
    lcd.print("Gen Start Time: ");
    lcd.setCursor(0,1);
    lcd.print(genStartTime / 10);
    lcd.print(".");
    lcd.print(genStartTime % 10);
    lcd.print("s");
    
    if(lcd_key == btnUP) //increase "genStartTime" by 0.1 seconds
    {
      genStartTime = genStartTime + 1;
      if(genStartTime > 30) //ceiling 
      {genStartTime = 30;}
    }
    else if(lcd_key == btnDOWN) //decrease "genStartTime" by 0.1 seconds
    {
      genStartTime = genStartTime - 1;
      if(genStartTime < 20)//floor
      {genStartTime = 20;}
    }
    else if(lcd_key == btnRIGHT) //go to main menu
    {
      Menu++; //"Menu" = 5
      lcd.clear();
    }
    else if(lcd_key == btnLEFT) //go to previous parameter ("DelayTime")
    {
      Menu--; //"Menu" = 3
    }
  }
}

//--------------------------------------------------------------------------------------

void WriteToMemory() //put system parameters into EEPROM
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

//--------------------------------------------------------------------------------------

void ReadFromMemory() //get system parameters from EEPROM
{
  float fminv = 11.9f;
  float fmaxv = 13.4f;
  float fdelt = 2.00f;
  float fgensd = 5.00f;
  int address = 0;
  
  address = 0; // address 0 has Minimum Voltage
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

//--------------------------------------------------------------------------------------

int read_LCD_buttons() //read LCD buttons
{
 adc_key_in = analogRead(0); //read the value from the sensor 
 if (adc_key_in > 1000) return btnNONE;
 if (adc_key_in < 50)   return btnRIGHT;  
 if (adc_key_in < 250)  return btnUP; 
 if (adc_key_in < 450)  return btnDOWN; 
 if (adc_key_in < 650)  return btnLEFT; 
 if (adc_key_in < 850)  return btnSELECT;  

 return btnNONE; //default to return btnNONE
}

//--------------------------------------------------------------------------------------

bool adjustButtonSensitivity() //dont allow rapid button changes
{
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
  
  if(prevButtonPressed == 0 && buttonPressed == 0)
  {
   btnHELD = true;
  }
}

//--------------------------------------------------------------------------------------

void checkForInactivity() //turn off LCD screen after 10 seconds of inactivity
{
  lcd_key = read_LCD_buttons();
  if(lcd_key == btnNONE && inactivityCounter < 100 && ScreenStatus == "ON")
  {
    //one loop of inactivity
    inactivityCounter++;
    
    if(inactivityCounter >= 100) //inactivity threshold reached
    {
      digitalWrite(Backlight,LOW); //turn screen off
      lcd.noDisplay();
      ScreenStatus = "OFF";
      inactivityCounter = 0;
    }
    else
    {
      digitalWrite(Backlight,HIGH); //turn screen on
      lcd.display();
      ScreenStatus = "ON";
    }
  }
  else if(lcd_key == btnSELECT)
  {
     digitalWrite(Backlight,HIGH); //turn screen on
     lcd.display();
     ScreenStatus = "ON";
  }

  if(lcd_key != btnNONE)
  {
    inactivityCounter = 0; //reset inactivity timer
  }
}

//--------------------------------------------------------------------------------------
