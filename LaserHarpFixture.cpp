#include "LaserHarpFixture.h"

LaserHarpFixture::LaserHarpFixture()
{
   //beamVector.push_back(new BeamFixture());

  
}



void LaserHarpFixture::init_beam_position()
{

    //todo : get from EEPROM
  //TODO : calibrate offset to have beams in center position at init
   beamVector[0].setPositionOffset(0);
   beamVector[1].setPositionOffset(0);
   beamVector[2].setPositionOffset(0);
   beamVector[3].setPositionOffset(0);
   beamVector[4].setPositionOffset(0);
   beamVector[5].setPositionOffset(0);
   beamVector[6].setPositionOffset(0);
   
  for(int i = 0; i < NB_BEAM; i++)
  {
      beamVector[i].setInitPosition();
  }


}

void LaserHarpFixture::setBeamPosition(int beamId, int position)
{
    beamVector[beamId].setPosition(position);
}
  

