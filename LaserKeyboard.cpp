#include "LaserKeyboard.h"



LaserKeyboard::LaserKeyboard(Adafruit_MCP23017 * mcp, LiquidCrystal_I2C * lcdDisplay, IPAddress* remoteIPAddress)
{

  lcd_ = lcdDisplay;
  buildMidiToNoteMap();


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

  //TODO : put this into WifiManager config
  //IPAddress* remoteIPAddress = new IPAddress(192,168,1,51);
 // IPAddress* remoteIPAddress = new IPAddress(192,168,1,21); //beelink
  // IPAddress* remoteIPAddress = new IPAddress(192,168,1,64);
  myOSCManager_ = new OSCManager(remoteIPAddress, 8000, 8888);
  mcp_ = mcp;
}

//todo : destructor


void LaserKeyboard::readAllPresetsFromROM()
{

  EEPROM.begin(512);
    int nbPresetFromROM =  (char)EEPROM.read(NB_PRESET_ADDRESS);

    Serial.print("Found nb presets in ROM :");
    Serial.println(nbPresetFromROM);
    if(nbPresetFromROM < 1 || nbPresetFromROM > NB_PRESETS)
    {
        Serial.println("Initializing LaserHarp preset bank with default preset");
        nbPresetFromROM = 1;
        notePresets_.push_back(defaultNotes);
    }
    else
    {

      for(int i = 0; i < nbPresetFromROM; i++)
      {
   
        int addr  =  i * BANK_SIZE;     
        Serial.print("Reading Preset Bank ");
        Serial.print(i); 
        Serial.print("at address : ");
        Serial.println(addr);
                       
        std::array<int, NB_BEAM> tempPreset;
        for(int j=0; j < NB_BEAM; j++)
        { 
          char tempC = (char)EEPROM.read(addr + j);
          tempPreset[j] = tempC;
          Serial.print("Read from ROM : ");
          Serial.println((int)tempC);
        }
        notePresets_.push_back(tempPreset);
        Serial.println("Added new preset from ROM");
      }
    }
  EEPROM.end();
  }
  
void LaserKeyboard::storePreset(int index, std::array<int, NB_BEAM> * thisPreset)
{
  int addr  = index * BANK_SIZE;
  EEPROM.begin(512);
    for(int i=0; i < NB_BEAM; i++)
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

void LaserKeyboard::buildMidiToNoteMap()
{
  for(int noteNb = 0; noteNb < 12; noteNb++)
  {
     for (int octaveNb = 0; octaveNb < 8; octaveNb ++)
     {
         midiToNoteMap[21 + octaveNb*12 + noteNb] = octaveNotes[noteNb] + octaveNb;
     }
  }
}
String LaserKeyboard::getNoteFromMidiNb(int midiValue)
{
    if(midiValue >= 21 and midiValue <= 127)
      return midiToNoteMap[midiValue];

    return "O";
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

/*
 * if (beamIndex < 4)
  {
    row = 0;
    column = beamIndex*4;
  }
  else
  {
    row = 1;
    column = 2 + beamIndex*4 - 16;
  }
 */
void LaserKeyboard::getCurserPositionForBeam(int beamIndex, int & column, int & row)
{
  switch (beamIndex)
  {
      case 0:
        column = 0;
        row    = 0;
      break;
      case 2:
        column = 4;
        row    = 0;
      break;
      case 4:
        column = 8;
        row    = 0;
      break;
      case 6:
        column = 12;
        row    = 0; 
      break;
      case 1:
        column = 2;
        row    = 1; 
      break;
      case 3:
        column = 6;
        row    = 1; 
      break;
      case 5:
        column = 10;
        row    = 1; 
      break;
        
  }
    
}

void LaserKeyboard::process_beam_event(int beamId, bool value)
{
                 
        Serial.print("LaserKeyboard::process_beam_event : ");
        Serial.print(beamId);
        Serial.print(" : ");
        Serial.println(value);
        Serial.print("Current Preset : ");
        Serial.println(currentPreset_);
        int currentNote = notePresets_[currentPreset_][beamId];
        int thisRow, thisColumn;
        if(value)
        {
            myOSCManager_->sendNote(currentNote, 120, 0);
            if(lcd_)
            {
               getCurserPositionForBeam(beamId, thisColumn, thisRow);
               lcd_->setCursor(thisColumn,thisRow);
               lcd_->print((char)LCD_CUSTOM_NOTE);
               lcd_->print(getNoteFromMidiNb(currentNote));
               lcd_->print((char)LCD_CUSTOM_NOTE);
            }
        }
        else
        {
            myOSCManager_->sendNote(currentNote, 0, 0);
            if(lcd_)
            {
               getCurserPositionForBeam(beamId, thisColumn, thisRow);
               lcd_->setCursor(thisColumn,thisRow);
               lcd_->print("    ");

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
      if(countSinceLastTriggerMap[i] < TRIGGER_DEBOUNCE_VALUE)
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
