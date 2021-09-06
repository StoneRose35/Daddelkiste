

// include the library code:
#include <LiquidCrystal.h>
#include <Wire.h>
#define wdr() __asm__ __volatile__ ("wdr")


// initialize the library by associating any needed LCD interface pin
// with the arduino pin number it is connected to
//const int rs = 12, en = 11, d4 = 5, d5 = 4, d6 = 3, d7 = 2;
const int rs = 12, en = 11, d4 = A2, d5 = A3, d6 = A4, d7 = A5;
const int fan = 9;
const int contrast_gen = 6;
const int atmega_twi_addr = 10;


LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int volumeOld=0;
unsigned int cntr=0; 
int bat_voltage = 0;
int fanSpeed;
char buttonPushLength = 0; // 1: short, 2: long

void sendVolume(int val)
{
      String sentValue;
      sentValue = "VOL(";
      sentValue += val;
      sentValue += ")\n";
      Serial.print(sentValue);
}

void returnErrorCode(int code)
{
        String err_msg = "ERR(";
        err_msg += code;
        err_msg += ")\n";
        Serial.print(err_msg);    
}

void initWatchdog()
{
  //cli();
  wdr();
  WDTCSR |= (1<<WDCE) | (1<<WDE); // has to be done in one instruction, see data sheet
  WDTCSR  = (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (0 << WDIE); // watchdog is set to 1s 
}

void setup() {

  initWatchdog();
  // generate a square wave for the constrast voltage
  analogWrite(contrast_gen,128);

  //slow down PWM Frequency
  TCCR1B&=0b11111000;
  TCCR1B|=0b00000100; 

  // 3.3v reference for the volume pot
  analogReference(EXTERNAL);
  
  delay(100);
  Wire.begin();
  
  // switch off audio amp
  Wire.beginTransmission(atmega_twi_addr);
  Wire.write(0);
  Wire.endTransmission();

  // get the button press length
  Wire.beginTransmission(atmega_twi_addr);
  Wire.write(3);
  Wire.endTransmission();
  delay(2);
  Wire.requestFrom(atmega_twi_addr,1);
  while (Wire.available()) {
        buttonPushLength = Wire.read();
  }

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  // Print a message to the LCD.
  lcd.print("Daddelkiste booting");

  Serial.begin(9600);
  
}

void loop() {
  wdr();
  int volumeValue = analogRead(A0);
  if (volumeValue > volumeOld)
  {
    if (volumeValue - volumeOld > 4)
    {
      sendVolume(volumeValue);
      volumeOld = volumeValue;
    }
  }
  else if ( volumeValue < volumeOld) 
  {
    if (volumeOld - volumeValue > 4)
    {
     sendVolume(volumeValue);
      volumeOld = volumeValue;
    }
  }

  if (Serial.available() > 0)
  {
    String data = Serial.readStringUntil('\n');
    if (data.startsWith("F"))
    {
      // command for setting the fan speed
      fanSpeed = data.substring(1).toInt();
      analogWrite(fan,fanSpeed);
    }
    else if (data.startsWith("D"))
    {
      int linenr = data.substring(1,2).toInt();
      lcd.setCursor(0, linenr);
      lcd.print("                    ");
      lcd.setCursor(0, linenr);
      lcd.print(data.substring(2));
    }
    else if (data.startsWith("A"))
    {
      Wire.beginTransmission(atmega_twi_addr);
      // toggle audio amplifier
      if (data.substring(1).toInt()==0)
      {
          Wire.write(0);
      }
      else
      {
          Wire.write(1);
      }
      int retcode = Wire.endTransmission();  
      if(retcode!=0)
      {
        returnErrorCode(retcode);
      }
    }
    else if (data.startsWith("V"))
    {
      sendVolume(volumeOld);
    }
    else if (data.startsWith("B"))
    {
      String sentValue;
      sentValue = "BUT(";
      sentValue += atoi(buttonPushLength);
      sentValue += ")\n";
      Serial.print(sentValue);
    }
  }

  if (cntr>=500)
  {
      Wire.beginTransmission(atmega_twi_addr);
      Wire.write(2);
      int retcode = Wire.endTransmission();
      if(retcode!=0)
      {
        returnErrorCode(retcode);
      }
      delay(2);
      int bytes_returned = Wire.requestFrom(atmega_twi_addr,2);
      if (bytes_returned != 2)
      {
        returnErrorCode(bytes_returned + 10);
      }
      bat_voltage=0;
      int factor = 256;
      while (Wire.available()) {
        char c = Wire.read();
          bat_voltage = bat_voltage + factor * c;
          factor /= 256;
      }
      String bat_msg = "BAT(";
      bat_msg += bat_voltage;
      bat_msg += ")\n";
      Serial.print(bat_msg);
      cntr=0;
  }
  else {
      cntr += 1;
  }
}
