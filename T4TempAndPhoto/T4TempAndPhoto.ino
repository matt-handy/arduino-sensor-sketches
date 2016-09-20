#include <OneWire.h>
#include <Adafruit_VC0706.h>

#include <SoftwareSerial.h>         

#include <TSL2561.h>
#include <Wire.h>
TSL2561 tsl(TSL2561_ADDR_FLOAT);
OneWire  ds(8);  // on pin 8 (a 4.7K resistor is necessary)

void setup(void) {
 Serial.begin(9600);
 
 tsl.setGain(TSL2561_GAIN_16X);
 tsl.setTiming(TSL2561_INTEGRATIONTIME_13MS);
}

void temp(){
  byte i;
  byte present = 0;
  byte type_s;
  byte data[12];
  byte addr[8];
  float celsius, fahrenheit;
  ds.reset_search();
  if ( !ds.search(addr)) {
    Serial.println("No more addresses.");
    Serial.println();
    ds.reset_search();
    delay(250);
    return;
  }
  
  if (OneWire::crc8(addr, 7) != addr[7]) {
      //Serial.println("CRC is not valid!");
      return;
  }
  
 
  // the first ROM byte indicates which chip
  switch (addr[0]) {
    case 0x10:
      //Serial.println("  Chip = DS18S20");  // or old DS1820
      type_s = 1;
      break;
    case 0x28:
      //Serial.println("  Chip = DS18B20");
      type_s = 0;
      break;
    case 0x22:
      //Serial.println("  Chip = DS1822");
      type_s = 0;
      break;
    default:
      //Serial.println("Device is not a DS18x20 family device.");
      return;
  } 

  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  
  delay(1000);    
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad

  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }

  // Convert the data to actual temperature
  // because the result is a 16 bit signed integer, it should
  // be stored to an "int16_t" type, which is always 16 bits
  // even when compiled on a 32 bit processor.
  int16_t raw = (data[1] << 8) | data[0];
  if (type_s) {
    raw = raw << 3; // 9 bit resolution default
    if (data[7] == 0x10) {
      // "count remain" gives full 12 bit resolution
      raw = (raw & 0xFFF0) + 12 - data[6];
    }
  } else {
    byte cfg = (data[4] & 0x60);
    // at lower res, the low bits are undefined, so let's zero them
    if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
    else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
    else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms
    //// default is 12 bit resolution, 750 ms conversion time
  }
  celsius = (float)raw / 16.0;
  fahrenheit = celsius * 1.8 + 32.0;
  
  Serial.print("<TLM NAME='T4-TEMP' VALUE='");
  Serial.print(fahrenheit);
  Serial.print("'/>");
}

void pMeter() {
  
  {
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  ir = lum >> 16;
  full = lum & 0xFFFF;
  Serial.print("<TLM NAME='B6IR' VALUE='");
  Serial.print(ir, 1);
  Serial.print("'/>");
  Serial.print("<TLM NAME='B6VIS' VALUE='");
  Serial.print((full - ir), 1);
  Serial.print("'/>");
  Serial.print("<TLM NAME='B6LUX' VALUE='");
  Serial.print(tsl.calculateLux(full, ir), 1);
  Serial.print("'/>");
  }
}

void loop(void) {
  int i = 0;
  while(i < 120){
    //30 second increments
    if(i % 20 == 0){
      Serial.print("<SAMPLE>");
      temp(); 
      pMeter();
      Serial.println("</SAMPLE>");
    }
    delay(30000);
    i++;
  }
}

