#include "BeamFixture.h"
#include <vector>
#define NB_BEAM 7
#define SERVO_BOARD_ADDRESS 0x40

class LaserHarpFixture
{
  public:
  Adafruit_PWMServoDriver pwm_ = Adafruit_PWMServoDriver(SERVO_BOARD_ADDRESS);
  std::vector<BeamFixture*> beamVector;

  
  public:
  LaserHarpFixture();
  void setup();
  void setBeamPosition(int beamId, int position);
};
