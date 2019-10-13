#ifndef BEAMFIXTURE_H
#define BEAMFIXTURE_H

#include <Adafruit_PWMServoDriver.h>

#include "Fixture.h"


#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  500 // this is the 'maximum' pulse length count (out of 4096)
#define CENTER_POSITION 126

class BeamFixture: public Fixture{
  public:
  int positionOffset_;
  Adafruit_PWMServoDriver * pwm_;
  int servoNum_;
  int powerNum_;
  
  public:
  BeamFixture(String name, Adafruit_PWMServoDriver* pwm_p, int servoId, int powerNum);

  void setPositionOffset(int offset);
  int getPositionOffset();
  void setPosition(int position);
  void setInitPosition();
  void setPower(int val);
  void strobe();  
};

#endif
