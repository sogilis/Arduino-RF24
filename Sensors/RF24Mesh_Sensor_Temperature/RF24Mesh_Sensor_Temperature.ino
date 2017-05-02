#include "RF24.h"
#include "RF24Network.h"
#include "RF24Mesh.h"
#include <SPI.h>

//Constants
#define nodeID 10
#define updateFrequencyMs 1000

// Configure the nrf24l01 CE and CS pins
RF24 radio(7, 8);
RF24Network network(radio);
RF24Mesh mesh(radio, network);

// Data structure definition
struct rf_data_float {
  char id;
  float data;
};

//Variables definition
uint32_t sendTimer = 0;
rf_data_float rf_data;

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

    if (getTemperature(&rf_data.data, true)) {
      Serial.println(F("Erreur de lecture du capteur"));
      return;
    }

    // Send an 'T' type message containing the current Temperature
    if (!mesh.write(&rf_data, 'F', sizeof(rf_data))) {

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

 

