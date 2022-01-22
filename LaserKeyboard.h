#include "OSCManager.h"
#include "Adafruit_MCP23017.h"
#include <EEPROM.h>
#include <vector>
#include <array>
#include <LiquidCrystal_I2C.h>

#define NB_BEAM 7
#define BANK_SIZE 32
#define NB_PRESETS 31
#define NB_PRESET_ADDRESS NB_PRESETS

//TODO: put debounce value in EEPROM SETTINGS
#define TRIGGER_DEBOUNCE_VALUE 10

//TODO: put definition in common
#define LCD_CUSTOM_NOTE 0

//TODO : fix notes octave bug : should start @C 
const String octaveNotes[12] = {"A", "A#", "B", "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#"};
const std::array<int, NB_BEAM> defaultNotes = {36, 48, 60, 62, 64, 65, 67}; //default to C scale

class LaserKeyboard{
  public:

  std::vector<std::array<int, NB_BEAM>> notePresets_;

  
  int currentPreset_ = 0;
  LiquidCrystal_I2C * lcd_;
  String midiToNoteMap[127];
  
  int beamIdToNoteMap[NB_BEAM];
  int beamToPinMap[NB_BEAM];
  int beamStatusMap[NB_BEAM];
  
  int currentBeamStatus_ = 0;
  int countSinceLastTriggerMap[NB_BEAM]; //init max as we don't want to trigger at init
  OSCManager * myOSCManager_;
  Adafruit_MCP23017 * mcp_;

  public:
  LaserKeyboard(Adafruit_MCP23017 * mcp, LiquidCrystal_I2C * lcdDisplay, IPAddress* remoteIPAddress);
  void initLaserKeyboardStatus();
  void setup();
  void buildMidiToNoteMap();
  String getNoteFromMidiNb(int midiValue);
  //TODO : implement a reset preset method
  void loadPreset(int presetIndex);
  void storePreset(int index, std::array<int, NB_BEAM> * thisPreset);
  void readAllPresetsFromROM();
  void addPresetFromCurrent();
  int getCurrentPreset();
  void loop();
  //void setBeamIdToNote(int beamId, int note);
  void process_beam_event(int beamId, bool value);
  void displayBeamStatus(int beamId);

  private:
  void getCurserPositionForBeam(int beamIndex, int & column, int &row);
  
};
