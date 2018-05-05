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
    statusOn_ = val;  
  }
  

