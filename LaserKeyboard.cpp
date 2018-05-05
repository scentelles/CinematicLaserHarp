#include "LaserKeyboard.h"

LaserKeyboard::LaserKeyboard(Adafruit_MCP23017 * mcp)
{
  //todo : get from EEPROM
  //init beam to note map.
  beamIdToNoteMap[0] = 42;
  beamIdToNoteMap[1] = 43;
  beamIdToNoteMap[2] = 44;
  beamIdToNoteMap[3] = 54;
  beamIdToNoteMap[4] = 64;
  beamIdToNoteMap[5] = 74;
  beamIdToNoteMap[6] = 84;

  beamToPinMap[0] = 8;
  beamToPinMap[1] = 9;
  beamToPinMap[2] = 10;
  beamToPinMap[3] = 11;
  beamToPinMap[4] = 12;
  beamToPinMap[5] = 13;
  beamToPinMap[6] = 14;  

  myOSCManager_ = new OSCManager();
  mcp_ = mcp;
}
//todo : destructor

void LaserKeyboard::setup()
{
  myOSCManager_->setup();
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
        Serial.print("LaserKeyboard::process_beam_event : ");
        Serial.print(beamId);
        Serial.print(" : ");
        Serial.println(value);
        if(value)
        {
            myOSCManager_->sendNote(beamIdToNoteMap[beamId], 120, 0);
        }
        else
        {
            myOSCManager_->sendNote(beamIdToNoteMap[beamId], 0, 0);
        }
}


void LaserKeyboard::loop()
{
  short tmpRegister;
  if(mcp_ != NULL)//read full register from IO expander once
  {
      tmpRegister  = mcp_->readGPIO(1); //read PORT B
    //  Serial.println(tmpRegister, BIN);
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
      if(mcp_ != NULL) //read from IO expander register value
      {
          currentBeamStatus_ = (tmpRegister >> i) & 1;   
        //  Serial.println(currentBeamStatus_);
      }
      else //read directly from pins
      {
          currentBeamStatus_ = digitalRead(beamToPinMap[i]);
      }

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
