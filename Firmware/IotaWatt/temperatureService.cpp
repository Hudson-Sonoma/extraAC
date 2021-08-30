#include "IotaWatt.h"

String temp_chip_type_string(byte addr_0);
byte temp_chip_type(byte addr_0);
float temp_raw2celsius(byte data[], byte type_s);

/******************************************************************************************************** 
 * HVAC Temperature Service - Read from attached OneWire Temperature Sensors (D6)
 *******************************************************************************************************/

uint32_t temperatureService(struct serviceBlock* _serviceBlock) { 
  static boolean started = false;
  static boolean sent_request = false;
  static uint32_t timeThen = millis();        
  uint32_t timeNow = millis();
  uint32_t a,b,c;
  static byte addr[8];
  static unsigned int call_count = 0;

  byte present = 0;
  byte data[12];
  byte type_s;
  static int lastChannel = 0;
  int nextChannel;
  bool rom_crc_valid;

  struct temp_input {
    OneWire *onewire;
    bool found_addr;
    byte addr[8];
    byte type_s;
    byte present;
    bool data_valid;
    byte data[9];
    float degree_c;
  };

  OneWire  oneWire3;
  OneWire  oneWire4;

  static struct temp_input temp_channels[MAXINPUTS] = { 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 0 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 1 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 2 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 3 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 4 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 5 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 6 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 7 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 8 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 9 
    {NULL,false,{0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},        // 10 
    {&oneWire3,false, {0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},  // 11     // each Nth instance of a onewire will search for the Nth temperature sensor.
    {&oneWire3,false, {0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},  // 12
    {&oneWire4,false, {0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0},  // 13
    {&oneWire4,false, {0,0,0,0,0,0,0,0},0,0,false, {0,0,0,0,0,0,0,0,0},0}   // 14
  };

  //inputChannel[channel]->setVoltage(VRMS);

  bool have_temp_channel = false;
  for (int i=0;i<maxInputs; i++) {
      if ( (inputChannel[i]->_type == channelTypeTemperature) &&  (temp_channels[i].onewire != NULL) ) { have_temp_channel=true;}
  }
  if (!have_temp_channel) {
    return UTCtime() + temperatureServiceInterval;
  }

  oneWire3.begin(pin_CS_ADC0);   // D3 / Grey  / 0-7
  pinMode(pin_CS_ADC0, INPUT);  
  oneWire4.begin(pin_CS_ADC1);   // D4 / White / 8-15
  pinMode(pin_CS_ADC1, INPUT);

  trace(T_temperature, 0);
  if(!started){

    a = millis();
    for (int i=0;i<maxInputs; i++) {
      if ( (inputChannel[i]->_type == channelTypeTemperature) &&  (temp_channels[i].onewire != NULL) ) {
        OneWire *ds = temp_channels[i].onewire;
        if (ds->search(addr)) {
          rom_crc_valid = (OneWire::crc8(addr, 7) == addr[7]);
          if (rom_crc_valid) {
            temp_channels[i].found_addr = true;
            temp_channels[i].type_s = temp_chip_type(addr[0]);
            memcpy(temp_channels[i].addr,addr,8);
          }
        }
      }
    }
    b = millis();

    trace(T_temperature, 1);
    log("temperatureService: started.");
    log("temperatureService: SEARCH: %d ms",b-a);
    if (temp_channels[11].found_addr) { log("temperatureServiceSetup: sensor 11: %s %s",bin2hex(temp_channels[11].addr,8).c_str(),temp_chip_type_string(temp_channels[11].addr[0]).c_str()); }
    if (temp_channels[12].found_addr) { log("temperatureServiceSetup: sensor 12: %s %s",bin2hex(temp_channels[12].addr,8).c_str(),temp_chip_type_string(temp_channels[11].addr[0]).c_str()); }
    if (temp_channels[13].found_addr) { log("temperatureServiceSetup: sensor 13: %s %s",bin2hex(temp_channels[13].addr,8).c_str(),temp_chip_type_string(temp_channels[11].addr[0]).c_str()); }
    if (temp_channels[14].found_addr) { log("temperatureServiceSetup: sensor 14: %s %s",bin2hex(temp_channels[14].addr,8).c_str(),temp_chip_type_string(temp_channels[11].addr[0]).c_str()); }

    started = true;
    if (call_count < 6) call_count += 1;

    pinMode(pin_CS_ADC0, OUTPUT);                // Make sure all the CS pins are HIGH
    digitalWrite(pin_CS_ADC0,HIGH);
    pinMode(pin_CS_ADC1,OUTPUT);
    digitalWrite(pin_CS_ADC1,HIGH);
    return (uint32_t)UTCtime() + 1;
  }

  if (!sent_request) {

    // nextChannel = (lastChannel + 1) % maxInputs;

    // while( !(inputChannel[nextChannel]->isActive() && (inputChannel[nextChannel]->_type == channelTypeTemperature)) ) {
    //     if (nextChannel == lastChannel) { break; }  // stop while loop when we have wrapped around.
    //     nextChannel = ++nextChannel % maxInputs;
    // }

  trace(T_temperature, 3);
    a = millis();
    OneWire *last_ds = NULL;
    for (int i=0;i<maxInputs; i++) {
      if ( (inputChannel[i]->_type == channelTypeTemperature) && 
            temp_channels[i].onewire != NULL && temp_channels[i].found_addr ) {
        OneWire *ds = temp_channels[i].onewire;
        //if(ds != last_ds) { ds->reset(); }
        ds->reset();
        ds->select(temp_channels[i].addr);
        ds->write(0x44, 1);
        last_ds = ds;
      }
    }
    b = millis();
    if (call_count < 6) {log("temperatureService: REQUEST: %d ms",b-a);}

    sent_request = true;

    pinMode(pin_CS_ADC0, OUTPUT);                // Make sure all the CS pins are HIGH
    digitalWrite(pin_CS_ADC0,HIGH);
    pinMode(pin_CS_ADC1,OUTPUT);
    digitalWrite(pin_CS_ADC1,HIGH);

    if (call_count < 6) call_count += 1;
    trace(T_temperature, 5);
    return UTCtime() + 1;      //delay(1000);     // maybe 750ms is enough, maybe not

  } else {

    trace(T_temperature, 4);

    a = millis();
    OneWire *last_ds = NULL;
    for (int i=0;i<maxInputs; i++) {
      if ( (inputChannel[i]->_type == channelTypeTemperature) ) {
        if ( temp_channels[i].onewire != NULL && temp_channels[i].found_addr ) {
          OneWire *ds = temp_channels[i].onewire;
          //if(ds != last_ds) { present = ds->reset(); }
          present = ds->reset();
          temp_channels[i].present = present;
          ds->select(temp_channels[i].addr);
          ds->write(0xBE, 0);
          for ( int j = 0; j < 9; j++) {           // we need 9 bytes
            temp_channels[i].data[j] = ds->read();
          }
          temp_channels[i].data_valid = (OneWire::crc8(temp_channels[i].data, 8) == temp_channels[i].data[8]);
          if (temp_channels[i].data_valid) {
            temp_channels[i].degree_c = temp_raw2celsius(temp_channels[i].data, temp_channels[i].type_s);
            temp_channels[i].degree_c += inputChannel[i]->_calibration;
            inputChannel[i]->setTemperature(temp_channels[i].degree_c);
          } else {
            inputChannel[i]->setTemperature(-59);  // +85 for sensor present but no reading triggered, -60 for no sensor ever, 
                                                // and, here, -59 for sensor was present at init, but now has failed
                                                // or been disconnected.
          }
          last_ds = ds;
        } else {
           inputChannel[i]->setTemperature(-60);
        }
      }
    }
    b = millis();
    if (call_count < 6) {log("temperatureService: RESPONSE: %d ms",b-a);}
    if (call_count < 6) {if (temp_channels[11].found_addr) { log("temperatureService: RESPONSE, sensor 11: present=%d,valid=%d,T=%f C",temp_channels[11].present,temp_channels[11].data_valid,inputChannel[11]->getTemperature()); }}
    if (call_count < 6) {if (temp_channels[12].found_addr) { log("temperatureService: RESPONSE, sensor 12: present=%d,valid=%d,T=%f C",temp_channels[12].present,temp_channels[12].data_valid,inputChannel[12]->getTemperature()); }}
    if (call_count < 6) {if (temp_channels[13].found_addr) { log("temperatureService: RESPONSE, sensor 13: present=%d,valid=%d,T=%f C",temp_channels[13].present,temp_channels[13].data_valid,inputChannel[13]->getTemperature()); }}
    if (call_count < 6) {if (temp_channels[14].found_addr) { log("temperatureService: RESPONSE, sensor 14: present=%d,valid=%d,T=%f C",temp_channels[14].present,temp_channels[14].data_valid,inputChannel[14]->getTemperature()); }}


    lastChannel = nextChannel;
    sent_request = false;

    pinMode(pin_CS_ADC0, OUTPUT);                // Make sure all the CS pins are HIGH
    digitalWrite(pin_CS_ADC0,HIGH);
    pinMode(pin_CS_ADC1,OUTPUT);
    digitalWrite(pin_CS_ADC1,HIGH);

    // interrupts();
    if (call_count < 6) call_count += 1;
    trace(T_temperature, 5);
    return UTCtime() + temperatureServiceInterval;
  }



}

String temp_chip_type_string(byte addr_0) {
  String msg;
  switch (addr_0) {
    case 0x10:
      msg = "DS18S20";  // or old DS1820
      break;
    case 0x28:
      msg = "DS18B20";
      break;
    case 0x22:
       msg = "DS1822";
      break;
    default:
      msg = "Device is not a DS18x20 family device.";
      break;
  } 
  return msg;
}

byte temp_chip_type(byte addr_0) {
  byte type_s = 0;
  switch (addr_0) {
    case 0x10:
      type_s = 1;
      break;
    case 0x28:
      type_s = 0;
      break;
    case 0x22:
      type_s = 0;
      break;
    default:
      break;
  } 
  return type_s;
}

float temp_raw2celsius(byte data[], byte type_s) {
   float celsius;
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
    return(celsius);

}

