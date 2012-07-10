// Holocene Technologies Data Logger
// Bryan Peele
// Rev 0.1   Date: 2012.06.15
// Based on data logging and LCD control programs
// by Adafruit Industries

//Note: Value 1 Lo Temp - LEFT side - white resistot
//      Value 2 Hi Temp - Right Side - red resistor

int startIndex = 0;

#include <SD.h>
#include <Wire.h>
#include "RTClib.h"
#include <Adafruit_MCP23017.h>
#include <Adafruit_RGBLCDShield.h>

Adafruit_RGBLCDShield lcd = Adafruit_RGBLCDShield();

// DATA READ RATE:
// milliseconds between grabbing data and logging it. 1000 ms is once a second
#define LOG_INTERVAL  1000 // mills between entries (reduce to take more/faster data)

// DATA WRITE RATE:
// milliseconds before writing the logged data permanently to disk
// set it to the LOG_INTERVAL to write each time (safest)
// set it to 10*LOG_INTERVAL to write all data every 10 datareads, you could lose up to 
// the last 10 reads if power is lost but it uses less power and is much faster!
#define SYNC_INTERVAL 10000 // mills between calls to flush() - to write data to the card
uint32_t syncTime = 0; // time of last sync()

#define ECHO_TO_SERIAL   1 // echo data to serial port

// the digital pins that connect to the LEDs
#define redLEDpin 2
#define greenLEDpin 3

// Dummy values to test system
int value1 = 0;
int value2 = 0;
float value1_volt = 0;
float value2_volt = 0;
float resistor1 = 246.3; //White (unlabeled)
float resistor2 = 247.9; //Red
float value1_amp = 0;
float value2_amp = 0;
float flow1_lo = 0;
float flow1_hi = 50;
float flow2_lo = 0;
float flow2_hi = 50;
float value1_flow = 0;
float value2_flow = 0;

int lightCount = 0;
int lightControl = 0;
int lightState = 1;   // 1 is ON, 0 is OFF


RTC_DS1307 RTC; // define the Real Time Clock object

// for the data logging shield, we use digital pin 10 for the SD cs line
const int chipSelect = 10;

// the logging file
File logfile;

void error(char *str)
{
  Serial.print("error: ");
  Serial.println(str);
  
  // red LED indicates error
  digitalWrite(redLEDpin, HIGH);

  while(1);
}



void setup(void)
{
  lcd.begin(16,2);

  uint8_t buttons = lcd.readButtons();
  
  Serial.begin(9600);
  Serial.println();

  lcd.setBacklight(1);
  
  // use debugging LEDs
  pinMode(redLEDpin, OUTPUT);
  pinMode(greenLEDpin, OUTPUT);
 
 
  // Welcome screen
  Serial.println("Holocene Technologies");
  lcd.clear();
  lcd.setCursor(4,0);
  lcd.print("HOLOCENE");
  lcd.setCursor(2,1);
  lcd.print("TECHNOLOGIES");
  delay(2500);

  Serial.println("Data Logger Rev 1.0");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("  Data Logger   ");
  lcd.setCursor(0,1);
  lcd.print("Rev1.0 06-18-12 ");
  delay(2500);

  // initialize the SD card
  Serial.print("Initializing SD card...");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Initializing");
  lcd.setCursor(0,1);
  lcd.print("SD card.");
  delay(500);
  lcd.setCursor(8,1);
  lcd.print(".");
  delay(500);
  lcd.setCursor(9,1);
  lcd.print(".");
  delay(500);
  lcd.setCursor(7,1);
  lcd.print("   ");
  delay(500);
  lcd.setCursor(7,1);
  lcd.print(".");
  delay(500);
  lcd.setCursor(8,1);
  lcd.print(".");
  delay(500);
  lcd.setCursor(9,1);
  lcd.print(".");
  delay(500);
  
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);
  
  // see if the card is present and can be initialized:
  if (!SD.begin(chipSelect)) {
    error("Card failed, or not present");
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Card failed!");
  }
  Serial.println("card initialized.");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Card initialized");
  delay(1000);
  
  // create a new file
  char filename[] = "LOGGER00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[6] = i/10 + '0';
    filename[7] = i%10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      logfile = SD.open(filename, FILE_WRITE); 
      break;  // leave the loop!
    }
    

    
    int time = millis();
    time = millis() - time;
    Serial.print("Took ");Serial.print(time);Serial.println(" ms");

    
  }
  
  if (! logfile) {
    error("couldnt create file");
  }
  
  Serial.print("Logging to: ");
  Serial.println(filename);

  // connect to RTC
  Wire.begin();  
  if (!RTC.begin()) {
    logfile.println("RTC failed");
#if ECHO_TO_SERIAL
    Serial.println("RTC failed");
#endif  //ECHO_TO_SERIAL     dee2
  }
  

  logfile.println("millis,stamp,datetime,Value 1,Value 1 (Volts), Value 1 (mA), Value 1 (GPM), Value 2, Value 2 (Volts), Value 2 (mA), Value 2 (GPM)");    
#if ECHO_TO_SERIAL
  Serial.println("millis,stamp,datetime,Value 1,Value 1 (Volts), Value 1 (mA), Value 1 (GPM), Value 2, Value 2 (Volts), Value 2 (mA), Value 2 (GPM)");
#endif //ECHO_TO_SERIAL
 
}

int count = 0;
  
void loop(void)
{ 
  
  uint8_t buttons = lcd.readButtons();
  
  
  if(startIndex == 0) {  
    Serial.println("Press SELECT to start");
    lcd.clear();
    lcd.setCursor(1,0);
    lcd.print("Start Recording");
    lcd.setCursor(1,1);
    lcd.print("Setup");
    count =1;
  }
  
  int cursorPos = 0;

  while(startIndex == 0) {    
    lcd.setCursor(0,cursorPos);
    lcd.print("*");
    
    buttons = lcd.readButtons();
    if (buttons) {
      if (buttons & BUTTON_SELECT) {
        if (cursorPos == 0) {
           lcd.clear();
           lcd.setCursor(0,0);
           lcd.print("Data started");
           delay(1000);
           
           lcd.clear();
           lcd.setCursor(0,0);
           lcd.print("Val1: ");
           lcd.setCursor(0,1);
           lcd.print("Val2: ");
           lcd.setCursor(12,0);
           lcd.print("GPM");
           lcd.setCursor(12,1);
           lcd.print("GPM");
           delay(100);
           startIndex = 1;
        }
        if (cursorPos == 1) {
          lcd.clear();
          lcd.setCursor(0,0);
          lcd.print("SETUP MENU");
          delay(1000);
          lcd.clear();
          startIndex=2;
        }        
      }
      if (buttons & BUTTON_UP) {
        cursorPos = 0;
        lcd.setCursor(0,0);
        lcd.print(" ");
        lcd.setCursor(0,1);
        lcd.print(" ");
      } 
      if (buttons & BUTTON_DOWN) {
        cursorPos=1;
        lcd.setCursor(0,0);
        lcd.print(" ");
        lcd.setCursor(0,1);
        lcd.print(" ");
      }
    }
  }
 
 while(startIndex==2) {
   lcd.setCursor(0,0);
   lcd.print("< PLACEHOLDER >");
   lcd.setCursor(0,1);
   lcd.print("Left to go back");
   
   buttons = lcd.readButtons();
   if (buttons) {
     if (buttons & BUTTON_LEFT) {
       startIndex=0;
     }
   }
 }
 
  while(startIndex == 1) {

    value1 = analogRead(0);
    value2 = analogRead(3);
    
    value1_volt = 5*((float) value1/1023);
    value2_volt = 5*((float) value2/1023);
    
    value1_amp = (1000*value1_volt)/resistor1;
    value2_amp = (1000*value2_volt)/resistor2;
    
    if (value1_amp >= 4) {
      value1_flow = flow1_lo + (((value1_amp-4)*(flow1_hi-flow1_lo))/(20-4));
    } else {
      value1_flow = 0;
    }
    if (value2_amp >= 4) {
      value2_flow = flow2_lo + (((value2_amp-4)*(flow2_hi-flow2_lo))/(20-4));
    } else {
      value2_flow = 0;
    }

    
    lcd.setCursor(6,0);
    lcd.print("   ");
    lcd.setCursor(6,0);
    lcd.print(value1_flow);

    lcd.setCursor(6,1);
    lcd.print("   ");
    lcd.setCursor(6,1);
    lcd.print(value2_flow); 

    if (lightCount > 15 && lightControl == 0) {
      lightState = 0;
      lcd.setBacklight(lightState);
    }
    
    if (lightControl == 0) {
      lightCount++;
    }
  
    buttons = lcd.readButtons();  
    if (buttons && ((buttons & BUTTON_LEFT)||(buttons & BUTTON_UP) || (buttons&BUTTON_RIGHT) || (buttons & BUTTON_DOWN))) {
      lightState = 1;
      lcd.setBacklight(lightState);
      lightCount = 0;
    }  
    
    if (buttons && (buttons & BUTTON_SELECT)) {
      if (lightControl == 0) {
        lcd.setBacklight(1);
        lightControl = 1;
        lightCount = 0;
        delay(20);
      }
      else if (lightControl == 1) {
        lcd.setBacklight(0);
        lightControl = 0;
        lightCount = 20;
        delay(20);
      }
    } 
 
 
  DateTime now;

  // delay for the amount of time we want between readings
  delay((LOG_INTERVAL -1) - (millis() % LOG_INTERVAL));
  
  digitalWrite(greenLEDpin, HIGH);
  
  // log milliseconds since starting
  uint32_t m = millis();
  logfile.print(m);           // milliseconds since start
  logfile.print(", ");    
#if ECHO_TO_SERIAL
  Serial.print(m);         // milliseconds since start
  Serial.print(", ");  
#endif

  // fetch the time
  now = RTC.now();
  // log time
  logfile.print(now.unixtime()); // seconds since 1/1/1970
  logfile.print(", ");
  logfile.print('"');
  logfile.print(now.year(), DEC);
  logfile.print("/");
  logfile.print(now.month(), DEC);
  logfile.print("/");
  logfile.print(now.day(), DEC);
  logfile.print(" ");
  logfile.print(now.hour(), DEC);
  logfile.print(":");
  logfile.print(now.minute(), DEC);
  logfile.print(":");
  logfile.print(now.second(), DEC);
  logfile.print('"');
#if ECHO_TO_SERIAL
  Serial.print(now.unixtime()); // seconds since 1/1/1970
  Serial.print(", ");
  Serial.print('"');
  Serial.print(now.year(), DEC);
  Serial.print("/");
  Serial.print(now.month(), DEC);
  Serial.print("/");
  Serial.print(now.day(), DEC);
  Serial.print(" ");
  Serial.print(now.hour(), DEC);
  Serial.print(":");
  Serial.print(now.minute(), DEC);
  Serial.print(":");
  Serial.print(now.second(), DEC);
  Serial.print('"');
#endif //ECHO_TO_SERIAL


    
  logfile.print(", ");    
  logfile.print(value1);
  logfile.print(", ");    
  logfile.print(value1_volt);
  logfile.print(", ");    
  logfile.print(value1_amp);
  logfile.print(", ");    
  logfile.print(value1_flow);
  logfile.print(", ");   
  logfile.print(value2);
  logfile.print(", ");    
  logfile.print(value2_volt);
  logfile.print(", ");   
  logfile.print(value2_amp);
  logfile.print(", ");    
  logfile.print(value2_flow);

#if ECHO_TO_SERIAL
  Serial.print(", ");   
  Serial.print(value1);  
  Serial.print(", ");    
  Serial.print(value1_volt);
  Serial.print(", ");    
  Serial.print(value1_amp);
  Serial.print(", ");    
  Serial.print(value1_flow);
  Serial.print(", ");   
  Serial.print(value2);  
  Serial.print(", ");    
  Serial.print(value2_volt);
  Serial.print(", ");    
  Serial.print(value2_amp);
  Serial.print(", ");    
  Serial.print(value2_flow);

#endif //ECHO_TO_SERIAL



  logfile.println();
#if ECHO_TO_SERIAL
  Serial.println();
#endif // ECHO_TO_SERIAL

  digitalWrite(greenLEDpin, LOW);

  // Now we write data to disk! Don't sync too often - requires 2048 bytes of I/O to SD card
  // which uses a bunch of power and takes time
  if ((millis() - syncTime) < SYNC_INTERVAL) return;
  syncTime = millis();
  
  // blink LED to show we are syncing data to the card & updating FAT!
  digitalWrite(redLEDpin, HIGH);
  logfile.flush();
  digitalWrite(redLEDpin, LOW);
  
  }
  
}





