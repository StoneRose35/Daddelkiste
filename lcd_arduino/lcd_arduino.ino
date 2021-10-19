

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
const int resetline = 7;


LiquidCrystal lcd(rs, en, d4, d5, d6, d7);
int volumeOld=0;
unsigned int cntr=0; 
int bat_voltage = 0;
int fanSpeed;
int buttonPushLength = 0; // 1: short, 2: long

void sendVolume(int val)
{
      String sentValue;
      sentValue = "VOL(";
      sentValue += val;
      sentValue += ")\n";
      Serial.print(sentValue);
}

void readBattery()
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
  WDTCSR  = (1<<WDE) | (1<<WDP2) | (1<<WDP1) | (0<<WDIE); // watchdog is set to 1s 
}

void setup() {

  // generate a square wave for the constrast voltage
  analogWrite(contrast_gen,128);

  // predefine reset line to output zero if defined as output
  digitalWrite(resetline,0);

  //slow down PWM Frequency
  TCCR1B&=0b11111000;
  TCCR1B|=0b00000100; 

  // 3.3v reference for the volume pot
  analogReference(EXTERNAL);
  
  delay(50);

  // set up the LCD's number of columns and rows:
  lcd.begin(20, 4);
  // Print a message to the LCD.
  lcd.print("Daddelkiste booting");
  delay(2);
  Serial.begin(9600);

}

void loop() {

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
     wdr();
      volumeOld = volumeValue;
    }
  }

  if (Serial.available() > 0)
  {
    String data = Serial.readStringUntil('\n');
    wdr();
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
      Wire.beginTransmission(atmega_twi_addr);
      Wire.write(3);
      int retcode = Wire.endTransmission();
      if (retcode!=0)
      {
        returnErrorCode(retcode);  
      }
      delay(2);
      int bytesreturned = Wire.requestFrom(atmega_twi_addr,1);
      while (Wire.available()) {
        buttonPushLength = Wire.read();
      }
      if (bytesreturned!= 1)
      {
        returnErrorCode(bytesreturned + 10);
      }
      String sentValue;
      sentValue = "BUT(";
      sentValue += buttonPushLength;
      sentValue += ")\n";
      Serial.print(sentValue);
    }
    else if (data.startsWith("I"))
    {
      lcd.begin(20, 4);  
    }
    else if (data.startsWith("S"))
    {
      readBattery();
      //wdr();
    }
    else if (data.startsWith("Q")) // start i2c communication
    {
      Wire.begin();  
    }
    else if (data.startsWith("J")) // pull down reset for 2 ms
    {
      pinMode(resetline,OUTPUT);
      delay(2);
      pinMode(resetline,INPUT);
      
    }
  }
}
