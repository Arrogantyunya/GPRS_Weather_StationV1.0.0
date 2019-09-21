/*
  AT24C1024.cpp
  AT24C1024 EEPROM Library for Arduino 
  http://www.arduino.cc/playground/Code/I2CEEPROM24C1024
*/

#include "SoftwareI2C.h"

#if ARDUINO >= 100
#include "Arduino.h"
#else
extern "C" {
#include "WConstants.h"
}
#endif

#include "AT24C1024.h"

SoftwareI2C EP;

void delay_ms(unsigned long nn)
{
   while(nn--)
   {
     delayMicroseconds(1000);
   }
}

AT24C1024::AT24C1024(void)
{
   EP.begin(PB7, PB6);
}

void AT24C1024::write(unsigned long dataAddress, uint8_t data)
{
  EP.beginTransmission((uint8_t)((0x500000 | dataAddress) >> 16)); // B1010xxx
  EP.write((uint8_t)((dataAddress & WORD_MASK) >> 8)); // MSB
  EP.write((uint8_t)(dataAddress & 0xFF)); // LSB
  EP.write(data);
  EP.endTransmission();
  delay_ms(5);
}

uint8_t AT24C1024::read(unsigned long dataAddress)
{
  uint8_t data = 0x00;
  EP.beginTransmission((uint8_t)((0x500000 | dataAddress) >> 16)); // B1010xxx
  EP.write((uint8_t)((dataAddress & WORD_MASK) >> 8)); // MSB
  EP.write((uint8_t)(dataAddress & 0xFF)); // LSB
  EP.endTransmission();
  EP.requestFrom((uint8_t)((0x500000 | dataAddress) >> 16),(uint8_t)1);
  if (EP.available()) data = EP.read();
  return data;
}

AT24C1024 EEPROM1024;
