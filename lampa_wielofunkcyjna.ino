#include <ThreeWire.h>  // time
#include <RtcDS1302.h>  //time
#include "pitches.h"  // alarm
#include "DHT.h"    //temp and h
#include <LiquidCrystal.h>  // LCD
#include <Adafruit_NeoPixel.h>    // Light -> standardowa formula we wszystkich projektach



#define countof(a) (sizeof(a) / sizeof(a[0])) // time

#define DHTPIN A5 //pin for DHT
#define DHTTYPE DHT11 //type of gauge

#ifdef __AVR__
#include <avr/power.h> // Required for 16 MHz Adafruit Trinket
#endif
#define LED_PIN    6   //podĹ‚Ä…czenie do 6 pinu 
#define LED_COUNT 8    //lamp id made of 8 leds

ThreeWire myWire(SDA, SCL, A0); // IO, SCLK, CE; pins for RTC
RtcDS1302<ThreeWire> Rtc(myWire);

//LCD
const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
LiquidCrystal lcd(rs, en, d4, d5, d6, d7);

// alarm clock
const int buttonPin = 9;  //button for turn off alarm
int buttonState = 0;
int a,b;          //parameters for alarm

//Input & Button Logic
const int numOfInputs = 5;
const int inputPins[numOfInputs] = {A1,A2,A3,A4};
int inputState[numOfInputs];
int lastInputState[numOfInputs] = { LOW,LOW,LOW,LOW };
bool inputFlags[numOfInputs] = { LOW,LOW,LOW,LOW };
long lastDebounceTime[numOfInputs] = { 0,0,0,0 };
long debounceDelay = 50;

//LCD Menu Logic
const int numOfScreens = 6; //max 10
int currentScreen = 0;
int parameters[numOfScreens];

// String screens[numOfScreens][2] = { {"Budzik h"," "}, {"Budzik min", " "}, {"Alarm ON-OFF"," "}, {"Aktualny czas"," "}, {"Kolor", " "}, {"Temp.      H."," "} };
String screens[numOfScreens] = {"Budzik h", "Budzik min", "Alarm ON-OFF", "Aktualny czas", "Kolor", "Temp.      H."};

int p = 0;  // Alias for parameters[] used in function parameterChange()
char datestring[10];  //for showing time on LCD screen

//Extreme parameters values
int minimum = 0;
int hhmax = 24;
int mmmax = 60;
int musicmax = 3;
int alarmon = 1;    

//temp
float t;  //variable for temperature 
int h;    //variable for humidity
DHT dht(DHTPIN, DHTTYPE);

//light:
int i;      //parameter for increasing the light intensity 
int swiatlo;  //for switch type of light
bool flag = true; //use to activate the alarm
unsigned long budmilis = 0;
Adafruit_NeoPixel strip(LED_COUNT, LED_PIN, NEO_RGB + NEO_KHZ800);

// notes in the melody:
int melody[] = {NOTE_C4, NOTE_G3, NOTE_G3, NOTE_A3, NOTE_G3, 0, NOTE_B3, NOTE_C4};
int noteDurations[] = {4, 8, 8, 4, 4, 4, 4, 4};


void setup()
{
  for (int i = 0; i < numOfInputs; i++)
  {
    pinMode(inputPins[i], INPUT_PULLUP);  // Buttons for screens and parameters
  }
  pinMode(buttonPin, INPUT);  // Button for alarm clock

  strip.begin(); // Initialize NeoPixel strip object (REQUIRED)
  strip.show(); // Initialize all pixels to 'off'

  Serial.begin(9600);
  lcd.begin(16, 2); //lcd on
  dht.begin();  //gauge on
  Rtc.Begin();    //time on
  RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__); //compile time
}


void loop()
{
  RtcDateTime now = Rtc.GetDateTime();  //what time is now


  snprintf_P(datestring,  //data for showing on screen
    countof(datestring),
    PSTR("%02u:%02u"),
    now.Hour(),
    now.Minute());

  if(parameters[1]<10)    //lamp turns on 10 minutes before alarm when minutes of alarm are smaller than 10 (0-9)
  {
      a=now.Hour()+1;
      if(a=24)a=0;      //when someone set alarm between 0:00-0:09
      b=now.Minute()-50;
  }
  else            //lamp turns on 10 minutes before alarm when minutes of alarm are bigger than 9 (9-59)
  {
    a=now.Hour();
      b=now.Minute()+10;
  }

  if (screens[currentScreen] == screens[3])     //showing time on LCD screen
  {
    lcd.setCursor(0, 1); lcd.print(datestring);
  }

  if ((parameters[0] == a) && (parameters[1] == b) && (parameters[2] == 1)) //alarm on; when parameters[2] == 0 alarm doesn't work
  {
    if (flag) 
    {
      for (i = 1; i < 256; i++) 
      {
        colorWipe(strip.Color(i, i, i), 294); //increase intensity of light
      }
      i = 0;                    //intensity 100%
      do                      //moment of turning on alarm; endless loop
      {
        buttonState = digitalRead(buttonPin);
        if (buttonState == HIGH) { i++; }   //when button press, alarm off
        else { Music(); }           //alarm on
      } while (i < 1);

      flag = false;

    }  
    colorWipe(strip.Color(0, 0, 0), 0);       //turn off lamp
    flag=true;                    //the ability to reset the alarm clock
  }                         //alarm ends


  h = dht.readHumidity();               //read temperature
  t = dht.readTemperature();              //read humidity

  setInputFlags();                  //all function below
  resolveInputFlags();
  Lamp();
  Kolor();
}


void Lamp()         //ON-50% intensity-OFF
{
  switch (parameters[3])
  {
  case 1:
    colorWipe(strip.Color(255, 255, swiatlo), 50);    //100% intensity
    break;
  case 2:
    colorWipe(strip.Color(127, 127, swiatlo / 2), 50);  //50% intensity
    break;
  case 3:
    colorWipe(strip.Color(0, 0, 0), 50);        //0% intensity
    break;
  default:
    break;
  }
}


void Kolor()        //three types of light in RGB
{
  switch (parameters[4])  //color depends on wchih parametr we choose
  {
  case 1:
    swiatlo = 255;    //color 255, 255, 255 - white
    break;
  case 2:
    swiatlo = 20;   //color 255, 255, 20
    break;
  case 3:
    swiatlo = 0;    //color 255, 255, 0 - yellow
    break;
  default:
    break;
  }
}


void colorWipe(uint32_t color, int wait) 
{
  unsigned long czas, zczas;
  czas = millis();
  zczas = millis() - wait;
  int i = 0;
  
  while (i < strip.numPixels())           //turn on the LEDs one by one in time
  {
    if (millis() - zczas > wait)        //like delay
    {
      strip.setPixelColor(i, color);          //set pixel's color 
      strip.show();             //show color
      zczas = czas;
      i++;                                     
    }                         
  }
}


void setInputFlags()
{
  for (int i = 0; i < numOfInputs; i++)
  {
    int reading = digitalRead(inputPins[i]);
    if (reading != lastInputState[i])
    {
      lastDebounceTime[i] = millis();
    }
    if ((millis() - lastDebounceTime[i]) > debounceDelay)
    {
      if (reading != inputState[i])
      {
        inputState[i] = reading;
        if (inputState[i] == HIGH)
        {
          inputFlags[i] = HIGH;
        }
      }
    }
    lastInputState[i] = reading;
  }
}


void resolveInputFlags() 
{
  for (int i = 0; i < numOfInputs; i++)
  {
    if (inputFlags[i] == HIGH)
    {
      inputAction(i);   // Reading buttons
      inputFlags[i] = LOW;
      printScreen();    // Viewing proper screen and parameter
    }
  }
}


void inputAction(int input)
{
    switch(input){
  // Button "left"
  case 0:
  
    if (currentScreen == 0)  // Screen loop
    {
      currentScreen = numOfScreens - 1;
    }
    else
    {
      currentScreen--;
    }
  break;
  
  // Button "right"
  case 1:
  
    if (currentScreen == numOfScreens - 1)  // Screen loop
    {
      currentScreen = 0;
    }
    else
    {
      currentScreen++;
    }
  break;
  
  // Button "up"
  case 2:
    parameterChange(0);
  break;  
  
  //Button "down"
  case 3:
    parameterChange(1);
  break;  
  
  default:
  break;
  }
}


void parameterChange(int key)
{
  p = parameters[currentScreen]; //alias
  //Button "up"
  if (key == 0)
  {
    if (currentScreen == 0 && p == hhmax - 1) parameters[currentScreen] = 0; // hour loop
    else if (currentScreen == 1 && p == mmmax - 1) parameters[currentScreen] = 0; // minute loop
    else if ((currentScreen == 4 || currentScreen == 3) && p == musicmax) parameters[currentScreen] = 1; // light loop
    else if (currentScreen == 2 && p == alarmon) parameters[currentScreen] = 0; // alarm loop
    else parameters[currentScreen]++;
  }
  //Button "down"
  else if (key == 1)
  {
    if (currentScreen == 0  && p == 0) parameters[currentScreen] = hhmax - 1; // hour loop
    else if (currentScreen == 1 && p == 0) parameters[currentScreen] = mmmax - 1; // minute loop
    else if ((currentScreen == 4 || currentScreen == 3) && p == 1) parameters[currentScreen] = musicmax; // light loop
    else if (currentScreen == 2 && p == 1)parameters[currentScreen] = alarmon; //alarm loop
    else parameters[currentScreen]--;
  }
}


void printScreen()      //what information is shown
{
  switch(currentScreen){
  
  case 0: // Screen with modified hour of alarm clock
  
    lcd.clear();
    lcd.print(screens[currentScreen]); // Viewing screen name
    lcd.setCursor(0, 1);
    lcd.print(parameters[currentScreen]); // Viewing hour
    lcd.print(":");
    lcd.print(parameters[currentScreen + 1]); // Viewing minute 
    break;
  
  case 1: // Screen with modified minute of alarm clock
  
    lcd.clear();
    lcd.print(screens[currentScreen]); // Viewing screen name
    lcd.setCursor(0, 1);
    lcd.print(parameters[currentScreen - 1]); // Viewing hour
    lcd.print(":");
    lcd.print(parameters[currentScreen]); // Viewing minute
    break;
  
  case 5: // Screen with temperature and humilildity
  
    lcd.clear();
    lcd.print(screens[currentScreen]); // Viewing screen name
    lcd.setCursor(0, 1);
    lcd.print(t);     // Viewing temperature
    lcd.print(" st.C ");
    lcd.print(h);     // Viewing humilildity
    lcd.print(" %");
    break;
  
  default: // Viewing other screens
  
    lcd.clear();
    lcd.print(screens[currentScreen]);
    lcd.setCursor(0, 1);
    lcd.print(parameters[currentScreen]);
    break;
  }
}


void Music()    //alarm; from example Digital: toneMelody
{
  for (int thisNote = 0; thisNote < 8; thisNote++) 
  {

    int noteDuration = 1000 / noteDurations[thisNote];
    tone(8, melody[thisNote], noteDuration);

    int pauseBetweenNotes = noteDuration * 1.30;
    delay(pauseBetweenNotes);
    // stop the tone playing:
    noTone(8);
  }
} 
