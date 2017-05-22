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
#define orderRelay     0
#define orderMotorTime 1
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
struct rf_order_packet {
  byte id;
  byte order_type;
  byte order_format;
  byte order_size;
  raw_data order;
};

byte data_buffer[20];
byte order_buffer[20];
rf_order_packet rfo;
int nodeAddrIndex = 0;

void setup() {
  Serial.begin(115200);
  Serial.setTimeout(100);

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
    switch(header.type){
      case 'S':
        rf_data_packet rfd;
        network.read(header,&data_buffer,20);
        rfd = debuild_packet(data_buffer);
        print_packet_serial(rfd);
        break;
      default: network.read(header,0,0); Serial.println(header.type);break;
    }
  }

  if(Serial.available()){
    String input_order;
    //String relayOn = String("/relay/20/ON");
    input_order = Serial.readStringUntil('\n');
    Serial.println("order received : " + input_order);
    if (parse_order(input_order, &rfo)) {
      build_packet_order(order_buffer, rfo);
      Serial.println( mesh.write(&order_buffer, 'O', 20, 20) == 1 ? F("Send OK") : F("Send Fail")); //Sending an message 
    } else {
      Serial.println("Parse order failed");
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

void build_packet_order(byte* order_buffer, rf_order_packet packet){
  order_buffer[0] = packet.id;
  order_buffer[1] = packet.order_type;
  order_buffer[2] = packet.order_format;
  order_buffer[3] = packet.order_size;
  for(int i=0; i < 16; i++){
    order_buffer[4+i] = packet.order.b[i];
  }
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

bool parse_order(String sInput, rf_order_packet *rfo)
{
  String current_arg;
  int iPosDelim;
  int iPosStart = 1;
  int nodeId;
  int orderRelayValue;
  float orderMotor;

  if (sInput[0] != '/') {
    return false;
  }

  for (int i = 0; i < 3; i++) {
    if(i==2){
      iPosDelim = sInput.length();
    }else{
      iPosDelim = sInput.indexOf('/', iPosStart);
    }
    if (iPosDelim == -1) {
      return false;
    }
    current_arg = sInput.substring(iPosStart, iPosDelim);
    iPosStart = iPosDelim + 1;
    if (i == 0) {
      nodeId = current_arg.toInt();
      if (nodeId == 0) {
        return false;
      }
      rfo->id = (byte) nodeId;
    } else if (i == 1) {
      if (current_arg == "relay") {
        rfo->order_type = orderRelay;
        rfo->order_format = typeByte;
      } else if (current_arg == "motor") {
        rfo->order_type = orderMotorTime;
      } else {
        return false;
      }
    } else if (i == 2) {
      if (rfo->order_type == orderRelay) {
        orderRelayValue = current_arg.toInt();
        rfo->order.b[0] = (byte) orderRelayValue;
      } else if (rfo->order_type == orderMotorTime) {
        orderMotor = current_arg.toFloat();
        if (orderMotor == 0) {
          return false;
        }
        rfo->order.f[0] = orderMotor;
      }
    }
  }
  return true;
}
