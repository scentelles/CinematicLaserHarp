#include "LaserHarpFixture.h"

LaserHarpFixture::LaserHarpFixture()
{
    pwm_.begin();
    pwm_.setPWMFreq(60);  // Analog servos run at ~60 Hz updates
    
    beamVector.push_back(new BeamFixture("BEAM_0", &pwm_, 0));
    beamVector.push_back(new BeamFixture("BEAM_1", &pwm_, 1));
    beamVector.push_back(new BeamFixture("BEAM_2", &pwm_, 2));
    beamVector.push_back(new BeamFixture("BEAM_3", &pwm_, 3));
    beamVector.push_back(new BeamFixture("BEAM_4", &pwm_, 4));
    beamVector.push_back(new BeamFixture("BEAM_5", &pwm_, 5));
    beamVector.push_back(new BeamFixture("BEAM_6", &pwm_, 6));
}



void LaserHarpFixture::setup()
{

    //todo : get from EEPROM
  //TODO : calibrate offset to have beams in center position at init
   beamVector[0]->setPositionOffset(0);
   beamVector[1]->setPositionOffset(0);
   beamVector[2]->setPositionOffset(0);
   beamVector[3]->setPositionOffset(0);
   beamVector[4]->setPositionOffset(0);
   beamVector[5]->setPositionOffset(0);
   beamVector[6]->setPositionOffset(0);
   
  for(int i = 0; i < NB_BEAM; i++)
  {
      beamVector[i]->setInitPosition();
  }


}

void LaserHarpFixture::setBeamPosition(int beamId, int position)
{
    beamVector[beamId]->setPosition(position);
}
  

