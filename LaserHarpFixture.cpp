#include "LaserHarpFixture.h"

LaserHarpFixture::LaserHarpFixture()
{
    pwm_.begin();
    pwm_.setPWMFreq(60);  // Analog servos run at ~60 Hz updates

 
    beamVector.push_back(new BeamFixture("BEAM_0", &pwm_, 0, 8));
    beamVector.push_back(new BeamFixture("BEAM_1", &pwm_, 1, 9));
    beamVector.push_back(new BeamFixture("BEAM_2", &pwm_, 2, 10));
    beamVector.push_back(new BeamFixture("BEAM_3", &pwm_, 3, 11));
    beamVector.push_back(new BeamFixture("BEAM_4", &pwm_, 4, 12));
    beamVector.push_back(new BeamFixture("BEAM_5", &pwm_, 5, 13));
    beamVector.push_back(new BeamFixture("BEAM_6", &pwm_, 6, 14));
    fogFixture = new BeamFixture("FOG", &pwm_, 7, 15);
}


void LaserHarpFixture::resetPosition()
{
  Serial.println("RESET LASERHARP BEAM POSITION TO CENTER");
  for(int i = 0; i < NB_BEAM; i++)
  {
      beamVector[i]->setInitPosition();
  }  
}

void LaserHarpFixture::setup()
{
   readCalibrationFromROM();
   readSettingsFromROM();
   resetPosition();
}

void LaserHarpFixture::readSettingsFromROM()
{
    EEPROM.begin(512);
    
    dmxAddress = (int8_t)EEPROM.read(FIXTURE_SETTINGS_OFFSET);
    Serial.print("Read Setting from ROM ");
    Serial.println((int)dmxAddress);

    EEPROM.end();
}

void LaserHarpFixture::readCalibrationFromROM()
{
    int addr  = CALIBRATION_OFFSET;
    EEPROM.begin(512);
    
    for(int i = 0; i < NB_BEAM; i++)
    {
        int tempC = (int8_t)EEPROM.read(addr + i);
        beamVector[i]->setPositionOffset(tempC);
        Serial.print("Read Position offset from ROM for Beam ");
        Serial.print(i);
        Serial.print(" : ");
        Serial.println((int)tempC);
    }
    EEPROM.end();
}

void LaserHarpFixture::storeCalibration()
{
    int addr  = CALIBRATION_OFFSET;
    EEPROM.begin(512);
    for(int i =0; i < NB_BEAM; i++)
    { 
      EEPROM.write(addr + i, beamVector[i]->getPositionOffset());
      Serial.print("Writing Calibration for Beam ");
      Serial.print(i);
      Serial.print(" to address ");
      Serial.println(addr + i);
    }
    EEPROM.commit();  
    EEPROM.end();
}

void LaserHarpFixture::storeDmxAddress()
{
    EEPROM.begin(512);

    EEPROM.write(FIXTURE_SETTINGS_OFFSET, (int8_t)dmxAddress);
    Serial.print("Writing DMX Address into EEPROM");
    Serial.print(" to address ");
    Serial.println(FIXTURE_SETTINGS_OFFSET);
    
    EEPROM.commit();  
    EEPROM.end();
}

void LaserHarpFixture::setLaserHarpInitPosition()
{
  Serial.println("INIT POSTION FOR LASERHARP MODE");
  setBeamPosition(0, CENTER_POSITION - 30);
  setBeamPosition(1, CENTER_POSITION - 20);   
  setBeamPosition(2, CENTER_POSITION - 10); 
  setBeamPosition(3, CENTER_POSITION); 
  setBeamPosition(4, CENTER_POSITION + 10); 
  setBeamPosition(5, CENTER_POSITION + 20); 
  setBeamPosition(6, CENTER_POSITION + 30);   
}

void LaserHarpFixture::setBeamPosition(int beamId, int position)
{
    beamVector[beamId]->setPosition(position);
}

void LaserHarpFixture::strobe()
{
    for(int i =0; i < NB_BEAM; i++)
    { 
       beamVector[i]->strobe();
    }
}

void LaserHarpFixture::powerAllBeams (bool on_off)
{
    int powerValue = 0;
    if (on_off)
       powerValue = 127;
    else
       powerValue = 0;
    for(int i =0; i < NB_BEAM; i++)
    { 
      beamVector[i]->setPower(powerValue);
    }
}

void LaserHarpFixture::applyDmxCommands(uint8_t* dmxFrame)
{
   int newLaserValue, newLaserPosition, newStrobeFrequency;
   
   for(int i = 0; i < NB_BEAM; i++)
   {
      newLaserValue = dmxFrame[getDmxAddress() - 1 + i*3];
      beamVector[i]->setPower(newLaserValue);
      newLaserPosition= dmxFrame[getDmxAddress() + i*3];
      beamVector[i]->setPosition(newLaserPosition);
      newStrobeFrequency= dmxFrame[getDmxAddress() + i*3 + 1];
      beamVector[i]->setStrobeFreq(newStrobeFrequency);
   }
   //last address is the fog control
   int fogServoPos = dmxFrame[getDmxAddress() + NB_BEAM*3 - 1]; 
   //if(fogOn > 1)
     fogFixture->setPosition(fogServoPos);
   //else
   //  fogFixture->setPosition(50); 
   
}

int LaserHarpFixture::getDmxAddress()
{
    return dmxAddress;  
}

void LaserHarpFixture::setDmxAddress(short address)
{
    if(address >=0)
    { 
      dmxAddress = address;
    }
}  
