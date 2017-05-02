#include "RF24Network.h"
#include "RF24.h"
#include "RF24Mesh.h"
#include <SPI.h>
//Include eeprom.h for AVR (Uno, Nano) etc. except ATTiny
#include <EEPROM.h>

/***** Configure the chosen CE,CS pins *****/
RF24 radio(7,8);
RF24Network network(radio);
RF24Mesh mesh(radio,network);

uint32_t displayTimer = 0;

struct rf_data_float {
  char id;
  float data;
};

struct rf_data_int {
  char id;
  int data;
};

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
      case 'F':
        rf_data_float rfd;
        network.read(header,&rfd,sizeof(rfd));
        Serial.print("F : ");
        Serial.print(rfd.id,DEC);
        Serial.print("=");
        Serial.println(rfd.data);
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
