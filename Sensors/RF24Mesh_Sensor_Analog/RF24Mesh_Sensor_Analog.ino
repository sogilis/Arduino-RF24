#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

//Constants
#define nodeID 20
#define updateFrequencyMs 1000

// Configure the nrf24l01 CE and CS pins
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

// Data structure definition
struct rf_data_int {
  char id;
  int data;
};

//Variables definition
uint32_t sendTimer = 0;
rf_data_int rf_data;
const int analogInPin = A0;

void setup() {
  Serial.begin(115200);
  mesh.setNodeID(nodeID);

  Serial.println(("Connecting to the mesh..."));
  mesh.begin();

  rf_data.id = nodeID;
}

void loop() {
  mesh.update();

  // Send to the master node every second
  if (millis() - sendTimer >= updateFrequencyMs) {
    sendTimer = millis();

    //Get the sensor data
    rf_data.data = analogRead(analogInPin);

    // Send an 'T' type message containing the current Temperature
    if (!mesh.write(&rf_data, 'I', sizeof(rf_data))) {

      // If a write fails, check connectivity to the mesh network
      if ( ! mesh.checkConnection() ) {
        //refresh the network address
        Serial.println("Renewing Address");
        mesh.renewAddress();
      } else {
        Serial.println("Send fail, Test OK");
      }
    } else {
      Serial.print("Send OK: "); Serial.println(rf_data.data);
    }
  }
}

 

