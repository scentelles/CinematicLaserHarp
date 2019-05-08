#include "LaserKeyboard.h"



LaserKeyboard::LaserKeyboard(Adafruit_MCP23017 * mcp, LiquidCrystal_I2C * lcdDisplay)
{

  lcd_ = lcdDisplay;
  
  //todo : get from EEPROM
  notePresets_.push_back({45, 56, 60, 62, 63, 45, 21});
  readAllPresetsFromROM();
  
  loadPreset(currentPreset_);

  beamToPinMap[0] = 8;
  beamToPinMap[1] = 9;
  beamToPinMap[2] = 10;
  beamToPinMap[3] = 11;
  beamToPinMap[4] = 12;
  beamToPinMap[5] = 13;
  beamToPinMap[6] = 14;  

  countSinceLastTriggerMap[0] = 0xFFFF;//init to max as we don't want to trigger at init
  countSinceLastTriggerMap[1] = 0xFFFF;
  countSinceLastTriggerMap[2] = 0xFFFF;
  countSinceLastTriggerMap[3] = 0xFFFF;
  countSinceLastTriggerMap[4] = 0xFFFF;
  countSinceLastTriggerMap[5] = 0xFFFF;
  countSinceLastTriggerMap[6] = 0xFFFF;
  
  //IPAddress* remoteIPAddress = new IPAddress(192,168,1,51);
  IPAddress* remoteIPAddress = new IPAddress(192,168,1,21); //beelink
  // IPAddress* remoteIPAddress = new IPAddress(192,168,1,64);
  myOSCManager_ = new OSCManager(remoteIPAddress, 8000, 8888);
  mcp_ = mcp;
}

//todo : destructor


void LaserKeyboard::readAllPresetsFromROM()
{

  EEPROM.begin(512);
    int nbPresetFromROM =  (char)EEPROM.read(NB_PRESET_ADDRESS);
    //TODO : find another way...
    if(nbPresetFromROM < 1 || nbPresetFromROM > 32)
        nbPresetFromROM = 1;
    Serial.print("Found nb presets in ROM :");
    Serial.println(nbPresetFromROM);
    //skip first one as wee need at least one preset untouched
    for(int i = 1; i < nbPresetFromROM;i++)
    {
      int addr  =  i * BANK_SIZE;      
      std::array<int, NB_BEAM> tempPreset;
      for(int j=0; j < NB_BEAM-1; j++)
      { 
        char tempC = (char)EEPROM.read(addr + i);
        
        tempPreset[j] = tempC;
      }
      notePresets_.push_back(tempPreset);
      Serial.println("Added new preset from ROM");
    }
  EEPROM.end();
  }
  
void LaserKeyboard::storePreset(int index, std::array<int, NB_BEAM> * thisPreset)
{
  int addr  = index * BANK_SIZE;
  EEPROM.begin(512);
    for(int i=0; i < NB_BEAM-1; i++)
    { 
      EEPROM.write(addr + i, (*thisPreset)[i]);
      Serial.print("Writing ");
      Serial.print((*thisPreset)[i]);
      Serial.print("to address");
      Serial.println(addr + i);
    }
    Serial.print("Storing Preset at index ");
    Serial.println(index);
    EEPROM.write(NB_PRESET_ADDRESS, notePresets_.size());
   // EEPROM.commit();  
    EEPROM.end();
}


int LaserKeyboard::getCurrentPreset()
{
  return currentPreset_;
}

void LaserKeyboard::addPresetFromCurrent()
{
  std::array<int, NB_BEAM> tempPreset = notePresets_[currentPreset_];
  notePresets_.push_back(tempPreset);
  storePreset(notePresets_.size()-1, &tempPreset);
}

void LaserKeyboard::loadPreset(int presetIndex)
{
  //exclude out of bound preset
  if((presetIndex >=  notePresets_.size()) || (presetIndex < 0))
      return;
      
  currentPreset_ = presetIndex;
  for(int i=0; i<NB_BEAM; i++)
  {
      beamIdToNoteMap[i] = notePresets_[presetIndex][i];
  }
   Serial.println("=======================");
    Serial.println(beamIdToNoteMap[0]);
  Serial.println(beamIdToNoteMap[1]);
  Serial.println(beamIdToNoteMap[2]);
  Serial.println(beamIdToNoteMap[3]);
  Serial.println(beamIdToNoteMap[4]);
    Serial.println(beamIdToNoteMap[5]);
      Serial.println(beamIdToNoteMap[6]);
           Serial.println("=======================");
}

void LaserKeyboard::setup()
{

  myOSCManager_->setup();
}



  
/*void LaserKeyboard::setBeamIdToNote(int beamId, int note)
{

  //store in local
 // beamIdToNoteMap[beamId] = note;
  //store in EEPROM
  notePresets_[currentPreset_]
}*/

void LaserKeyboard::displayBeamStatus(int beamId)
{
  
}

void LaserKeyboard::process_beam_event(int beamId, bool value)
{
      //  sendCC(51, newFilter);
        Serial.print("LaserKeyboard::process_beam_event : ");
        Serial.print(beamId);
        Serial.print(" : ");
        Serial.println(value);
        Serial.print("Current Preset : ");
        Serial.println(currentPreset_);
        int currentNote = notePresets_[currentPreset_][beamId];
        
        if(value)
        {
            myOSCManager_->sendNote(currentNote, 120, 0);
            if(lcd_)
            {
               lcd_->setCursor(beamId*2,1);
               lcd_->print((char)LCD_CUSTOM_NOTE);
            }
        }
        else
        {
            myOSCManager_->sendNote(currentNote, 0, 0);
            if(lcd_)
            {
               lcd_->setCursor(beamId*2,1);
               lcd_->print(" ");
            }
        }
}


void LaserKeyboard::loop()
{
  short tmpRegister;
  if(mcp_ != NULL)//read full register from IO expander once
  {
      tmpRegister  = mcp_->readGPIO(1); //read PORT B
   //   Serial.println(tmpRegister, BIN);
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
      if(mcp_ != NULL) //read from IO expander register value
      {
          currentBeamStatus_ = (tmpRegister >> i) & 1;   
          //Serial.println(currentBeamStatus_);
          
      }
      else //read directly from pins
      {
          currentBeamStatus_ = digitalRead(beamToPinMap[i]);
      }

      if(currentBeamStatus_)
      {
         countSinceLastTriggerMap[i] = 0;  
      }
      countSinceLastTriggerMap[i]++;

      //keep the trigger on debounce fog hazard
      if(countSinceLastTriggerMap[i] < 20)
      {
        currentBeamStatus_ = 1;
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
