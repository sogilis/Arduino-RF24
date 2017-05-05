#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
//Include eeprom.h for AVR (Uno, Nano) etc. except ATTiny
#include <EEPROM.h>

//Constants
#define dataTemperature 0
#define dataLuminosity  1
#define dataMoisture    2
#define typeByte  0
#define typeFloat 1
#define typeInt   2

/***** Configure the chosen CE,CS pins *****/
RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio,network);

uint32_t displayTimer = 0;

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

byte data_buffer[20];

void setup() {
  Serial.begin(115200);

  // Set the nodeID to 0 for the master node
  mesh.setNodeID(0);
  Serial.print("Node ID : ");
  Serial.println(mesh.getNodeID());
  // Connect to the mesh
  mesh.begin();

}


void loop() {    

  // Call mesh.update to keep the network updated
  mesh.update();
  
  // In addition, keep the 'DHCP service' running on the master node so addresses will
  // be assigned to the sensor nodes
  mesh.DHCP();
  
  
  // Check for incoming data from the sensors
  if(network.available()){
    RF24NetworkHeader header;
    network.peek(header);

    float dat=0;
    
    switch(header.type){
      //case 'M': network.read(header,&dat,sizeof(dat)); Serial.println(dat); break;
      case 'S':
        rf_data_packet rfd;
        network.read(header,&data_buffer,20);
        rfd = debuild_packet(data_buffer);
        print_packet_serial(rfd);
        break;
      default: network.read(header,0,0); Serial.println(header.type);break;
    }
  }
  
  if(millis() - displayTimer > 5000){
    displayTimer = millis();
    Serial.println(" ");
    Serial.println(F("********Assigned u Addresses********"));
     for(int i=0; i<mesh.addrListTop; i++){
       Serial.print("NodeID: ");
       Serial.print(mesh.addrList[i].nodeID);
       Serial.print(" RF24Network Address: 0");
       Serial.println(mesh.addrList[i].address,OCT);
     }
    Serial.println(F("**********************************"));
  }
}

rf_data_packet debuild_packet(byte *data_buffer){
  rf_data_packet rfd;
  rfd.id = data_buffer[0];
  rfd.data_type = data_buffer[1];
  rfd.data_format = data_buffer[2];
  rfd.data_size = data_buffer[3];
  for(int i=0; i < 16; i++){
    rfd.data.b[i] = data_buffer[4+i];
  }
  return rfd;
}

void print_packet_serial(rf_data_packet rfd){
  Serial.print('/');
  Serial.print(rfd.id);
  switch(rfd.data_type){
    case dataTemperature:
      Serial.print("/temperature");
    break;
    case dataLuminosity:
      Serial.print("/luminosity");
    break;
    case dataMoisture:
      Serial.print("/moisture");
    break;
  }
  Serial.print('/');

  if(rfd.data_format == typeFloat){
    Serial.println(rfd.data.f[0]);
  } else if(rfd.data_format == typeInt){
    Serial.println(rfd.data.i[0]);
  }
}

