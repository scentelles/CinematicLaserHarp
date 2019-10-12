#include "BeamFixture.h"
#include "Adafruit_MCP23017.h"
#include <vector>
#include <EEPROM.h>

#define NB_BEAM 7


#define SERVO_BOARD_ADDRESS 0x40
#define TEST_PIN D3

#define CALIBRATION_OFFSET 256
#define FIXTURE_SETTINGS_OFFSET 128

class LaserHarpFixture
{
  public:
  Adafruit_PWMServoDriver pwm_ = Adafruit_PWMServoDriver(SERVO_BOARD_ADDRESS);
  std::vector<BeamFixture*> beamVector;
  char dmxAddress = 0;
  
  public:
  LaserHarpFixture();
  void setup();
  void resetPosition();
  void readCalibrationFromROM();
  void readSettingsFromROM();
  void storeCalibration();
  void storeDmxAddress();
  int  getDmxAddress();
  void setDmxAddress(short address);
  
  void setBeamPosition(int beamId, int position);
  void setLaserHarpInitPosition();
  void powerAllBeams (bool on_off);
  void applyDmxCommands(uint8_t* dmxFrame);


};
