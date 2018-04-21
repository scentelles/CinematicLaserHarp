#include <ESP8266WiFi.h>//todo remove

#include "BeamFixture.h"

BeamFixture::BeamFixture()
{
  
}


void BeamFixture::setPosition(int value)
{
    //TODO : implement servo control
}

void BeamFixture::setInitPosition()
{
  position_ = 0;
  //Serial.println("BEAM INIT POSITION CALLED=======================================================");
  // setPosition with offset
}
void BeamFixture::setPositionOffset(int offset)
{
  
}
