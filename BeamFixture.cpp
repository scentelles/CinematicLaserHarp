#include <ESP8266WiFi.h>//todo remove

#include "BeamFixture.h"

BeamFixture::BeamFixture(String name, Adafruit_PWMServoDriver* pwm_p, int servoId)
{
    //setup servos
    pwm_ = pwm_p;
    servonum_ = servoId;
    //todo : move this this to base class
    name_ = name;

}


void BeamFixture::setPosition(int value)
{
    //TODO : implement servo control
    //todo : value to pulselen
    Serial.print("Moving servo ");
    Serial.print(servonum_);
    Serial.print(" to position ");
    Serial.println(value);
    Fixture::position_ = value;
    pwm_->setPWM(servonum_, 0, value);
}

void BeamFixture::setInitPosition()
{
  
  Fixture::setInitPosition();
  Serial.println("BEAM INIT POSITION CALLEDFROM BEAMFIXTURE=======================================================");
  
  // setPosition with offset
}
void BeamFixture::setPositionOffset(int offset)
{
  
}
void BeamFixture::setOn(bool val)
{
   Serial.println("Setting BeaM ON");
   //todo : control the IO
}
