#include "OSCManager.h"
#include "Adafruit_MCP23017.h"
#include <EEPROM.h>
#include <vector>
#include <array>
#include <LiquidCrystal_I2C.h>

#define NB_BEAM 7
#define BANK_SIZE 32
#define NB_PRESET_ADDRESS 31

//TODO: put definition in common
#define LCD_CUSTOM_NOTE 0

class LaserKeyboard{
  public:

  std::vector<std::array<int, NB_BEAM>> notePresets_;

  
  int currentPreset_ = 0;
  LiquidCrystal_I2C * lcd_;
  
  int beamIdToNoteMap[NB_BEAM];
  int beamToPinMap[NB_BEAM];
  int beamStatusMap[NB_BEAM];
  
  int currentBeamStatus_ = 0;
  int countSinceLastTriggerMap[NB_BEAM]; //init max as we don't want to trigger at init
  OSCManager * myOSCManager_;
  Adafruit_MCP23017 * mcp_;

  public:
  LaserKeyboard(Adafruit_MCP23017 * mcp, LiquidCrystal_I2C * lcdDisplay);
  void setup();
  void loadPreset(int presetIndex);
  void storePreset(int index, std::array<int, NB_BEAM> * thisPreset);
  void readAllPresetsFromROM();
  void addPresetFromCurrent();
  int getCurrentPreset();
  void loop();
  //void setBeamIdToNote(int beamId, int note);
  void process_beam_event(int beamId, bool value);
  void displayBeamStatus(int beamId);
  
};
