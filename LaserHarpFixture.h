#include "BeamFixture.h"
#include "Adafruit_MCP23017.h"
#include <vector>
#define NB_BEAM 7

//todo : get address from config in file system
#define DMX_ADDRESS 1
#define SERVO_BOARD_ADDRESS 0x40
#define TEST_PIN D3

class LaserHarpFixture
{
  public:
  Adafruit_PWMServoDriver pwm_ = Adafruit_PWMServoDriver(SERVO_BOARD_ADDRESS);
  std::vector<BeamFixture*> beamVector;

  
  public:
  LaserHarpFixture();
  void setup();
  void resetPosition();
  void setBeamPosition(int beamId, int position);
  void applyDmxCommands(uint8_t* dmxFrame);
  int getDmxAddress();

};
