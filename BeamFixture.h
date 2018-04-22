#ifndef BEAMFIXTURE_H
#define BEAMFIXTURE_H

#include <Adafruit_PWMServoDriver.h>

#include "Fixture.h"


#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pulse length count (out of 4096)

class BeamFixture: public Fixture{
  public:
  bool isOn = false;
  int positionOffset;
  Adafruit_PWMServoDriver * pwm_;
  int servonum_;
  
  public:
  BeamFixture(String name, Adafruit_PWMServoDriver* pwm_p, int servoId);

  void setPositionOffset(int offset);
  void setPosition(int position);
  void setInitPosition();
  void setOn(bool val);
};

#endif
