#include <ESP8266WiFi.h>//todo remove

#include "BeamFixture.h"

BeamFixture::BeamFixture(String name, Adafruit_PWMServoDriver* pwm_p, int servoId, int powerNum)
{
    //setup servos
    pwm_ = pwm_p;
    servoNum_ = servoId;
    powerNum_ = powerNum;

    //todo : move this this to base class
    name_ = name;

}


void BeamFixture::setPosition(int value)
{

    //todo : value to pulselen
    Serial.print("Moving servo ");
    Serial.print(servoNum_);
    Serial.print(" to position ");
    Serial.println(value);
    Fixture::position_ = value;
    pwm_->setPWM(servoNum_, 0, 125 + value*1.5);
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
void BeamFixture::setPower(int val)
{
 //  Serial.println("BeamFixture::Setting beam power");
    pwm_->setPWM(powerNum_, 0, 4096/256 * val);
}
