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

int BeamFixture::getPositionOffset()
{
  return positionOffset_; 
}

//position range is between 0 and 255 (DMX range)
void BeamFixture::setPosition(int value)
{


    //SERVO start @125PWM
    //range :? using multiplier 1.5
    int pulselen = SERVOMIN + value*1.5 + positionOffset_;

    Serial.print("Moving servo ");
    Serial.print(servoNum_);
    Serial.print(" to position ");
    Serial.print(value);
    Serial.print(" pulselen : ");
    Serial.println(pulselen);
    Fixture::position_ = value;
    if(pulselen >=SERVOMIN && pulselen <= SERVOMAX)
        pwm_->setPWM(servoNum_, 0, pulselen);
    else
        Serial.println("WARNING : SKIPPING OUT OF RANGE SERVO MOVE ORDER");
}

void BeamFixture::setInitPosition()
{
  //Don't call directly base init position, as LaserHarp will have its dedicated init position at center
  //Fixture::setInitPosition();
  Serial.println("BEAM INIT POSITION CALLED FROM BEAMFIXTURE=======================================================");

  setPosition(CENTER_POSITION);
  
}

void BeamFixture::setPositionOffset(int offset)
{
  positionOffset_ = offset;
}

void BeamFixture::setPower(int val)
{
    //Serial.print("Setting beam power ");
    //Serial.print(servoNum_);
    //Serial.print(" to value ");
    //Serial.println(val);
    pwm_->setPWM(powerNum_, 0, 4096/256 * val);
}
