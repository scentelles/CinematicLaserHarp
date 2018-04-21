#include "LaserKeyboard.h"

LaserKeyboard::LaserKeyboard()
{
  //todo : get from EEPROM
  //init beam to note map.
  beamIdToNoteMap[0] = 42;
  beamIdToNoteMap[1] = 43;
  beamIdToNoteMap[2] = 44;
  beamIdToNoteMap[3] = 45;
  beamIdToNoteMap[4] = 46;
  beamIdToNoteMap[5] = 47;
  beamIdToNoteMap[6] = 48;

  beamToPinMap[0] = D5;
  beamToPinMap[1] = D6;
  beamToPinMap[2] = D7;
  beamToPinMap[3] = D7;
  beamToPinMap[4] = D7;
  beamToPinMap[5] = D7;
  beamToPinMap[6] = D7;  

  myOSCManager_ = new OSCManager();
}
//todo : destructor

void LaserKeyboard::setup()
{

}

void LaserKeyboard::setBeamIdToNote(int beamId, int note)
{

  //store in local
  beamIdToNoteMap[beamId] = note;
  //store in EEPROM
}

void LaserKeyboard::process_beam_event(int beamId, bool value)
{
      //  sendCC(51, newFilter);
        myOSCManager_->sendNote(beamIdToNoteMap[beamId], 120, 100);
  
}


void LaserKeyboard::loop()
{

  for(int i = 0; i < NB_BEAM; i++)
  {
      //todo : replace with I2C IO read
      currentBeamStatus_ = digitalRead(beamToPinMap[i]);
      
      //update and take action only if the status has changed
      if(currentBeamStatus_ != beamStatusMap[i])
      {
          //update the beam status.
          beamStatusMap[i] = currentBeamStatus_;
          //process the corresponding event trigger action
          process_beam_event(i, currentBeamStatus_);
      }
  }
}
