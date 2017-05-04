#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>
#include <OneWire.h>

//Constants
#define nodeID 11
#define dataTemperature 0
#define dataLuminosity  1
#define dataMoisture    2
#define typeByte  0
#define typeFloat 1
#define typeInt   2
#define updateFrequencyMs 1000

// Configure the nrf24l01 CE and CS pins
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

union raw_data{
  byte b[16];
  float f[4];
  int16_t i[8];
};
// Data structure definition
struct rf_data_packet {
  byte id;
  byte data_type;
  byte data_format;
  byte data_size;
  raw_data data;
};

//Variables definition
uint32_t sendTimer = 0;
rf_data_packet rf_data_temperature;
rf_data_packet rf_data_luminosity;
rf_data_packet rf_data_moisture;
OneWire  ds(2);
const int analogMoisturePin = A0;
const int analogLuminosityPin = A1;
byte data_buffer_1[20];
byte data_buffer_2[20];
byte data_buffer_3[20];
bool send_fail = false;

//bool getTemperature(float *temperature);

void setup() {
  Serial.begin(115200);
  mesh.setNodeID(nodeID);

  Serial.println(("Connecting to the mesh..."));
  mesh.begin();

  rf_data_temperature.id = nodeID;
  rf_data_temperature.data_type = dataTemperature;
  rf_data_temperature.data_format = typeFloat;
  rf_data_temperature.data_size = 1;

  rf_data_luminosity.id = nodeID;
  rf_data_luminosity.data_type = dataLuminosity;
  rf_data_luminosity.data_format = typeInt;
  rf_data_luminosity.data_size = 1;

  rf_data_moisture.id = nodeID;
  rf_data_moisture.data_type = dataMoisture;
  rf_data_moisture.data_format = typeInt;
  rf_data_moisture.data_size = 1;
}

void loop() {
  mesh.update();

  // Send to the master node every second
  if (millis() - sendTimer >= updateFrequencyMs) {
    sendTimer = millis();

    if (!getTemperature(&rf_data_temperature.data.f[0])) {
      Serial.println(F("Erreur de lecture du capteur"));
      return;
    }
    rf_data_luminosity.data.i[0] = analogRead(analogLuminosityPin);
    rf_data_moisture.data.i[0] = analogRead(analogMoisturePin);

    // Send an 'T' type message containing the current Temperature
    build_packet(data_buffer_1, rf_data_temperature);
    build_packet(data_buffer_2, rf_data_luminosity);
    build_packet(data_buffer_3, rf_data_moisture);
    send_fail |= !mesh.write(&data_buffer_1, 'S', 20);
    //delay(250);
    send_fail |= !mesh.write(&data_buffer_2, 'S', 20);
    //delay(250);
    send_fail |= !mesh.write(&data_buffer_3, 'S', 20);
    if (send_fail) {
      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println("Renewing Address");
        mesh.renewAddress();
      } else {
        Serial.println("Send fail, Test OK");
      }
    } else {
      Serial.print("Send OK Temp: "); Serial.println(rf_data_temperature.data.f[0]);
      Serial.print("Send OK Light: "); Serial.println(rf_data_luminosity.data.i[0]);
      Serial.print("Send OK Moisture: "); Serial.println(rf_data_moisture.data.i[0]);
    }
  }
}

void build_packet(byte* data_buffer, rf_data_packet packet){
  data_buffer[0] = packet.id;
  data_buffer[1] = packet.data_type;
  data_buffer[2] = packet.data_format;
  data_buffer[3] = packet.data_size;
  for(int i=0; i < 16; i++){
    data_buffer[4+i] = packet.data.b[i];
  }
}

bool getTemperature(float *temperature){
  byte i;
  byte present = 0;
  byte data[12];
  byte addr[8] = {0x28, 0xBC, 0xA3, 0x6A, 0x8, 0x0, 0x0, 0xB7};
  //28 FF 8C 42 B3 16 4 3A

 
  if (OneWire::crc8(addr, 7) != addr[7]) {
      Serial.println("CRC is not valid!");
      return false;
  }
 
  if(addr[0] != 0x28){  
      Serial.println("Device is not a DS18x20 family device.");
      return false;
  } 
  ds.reset();
  ds.select(addr);
  ds.write(0x44, 1);        // start conversion, with parasite power on at the end
  delay(1000);     // maybe 750ms is enough, maybe not
  // we might do a ds.depower() here, but the reset will take care of it.
  
  present = ds.reset();
  ds.select(addr);    
  ds.write(0xBE);         // Read Scratchpad
  for ( i = 0; i < 9; i++) {           // we need 9 bytes
    data[i] = ds.read();
  }
  int16_t raw = (data[1] << 8) | data[0];
  byte cfg = (data[4] & 0x60);
  // at lower res, the low bits are undefined, so let's zero them
  if (cfg == 0x00) raw = raw & ~7;  // 9 bit resolution, 93.75 ms
  else if (cfg == 0x20) raw = raw & ~3; // 10 bit res, 187.5 ms
  else if (cfg == 0x40) raw = raw & ~1; // 11 bit res, 375 ms

  *temperature = (float)raw / 16.0;
  return true;
}

