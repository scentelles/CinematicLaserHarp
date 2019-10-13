#include <ESP8266WiFi.h>//todo remove

#include "Fixture.h"

  Fixture::Fixture(){
     
  }

  Fixture::Fixture(String name){
      name_ = name; 
  }


  void Fixture::setPosition(int position){}
  void Fixture::setInitPosition()
  {
      Serial.println("BEAM INIT POSITION CALLEDFROM BASE FIXTURE=======================================================");
      position_ = 0;
  }
  void Fixture::setPower(int val)
  {
    dimmerVal_ = val;  
  }
  void Fixture::setStrobeFreq(int val)
  {
    strobeFreq_ = val;  
  }  
  void Fixture::strobe()
  {

  }  
