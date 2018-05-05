#include "OSCManager.h"
#include "Adafruit_MCP23017.h"
#define NB_BEAM 7

class LaserKeyboard{
  public:
  int beamIdToNoteMap[NB_BEAM];
  int beamToPinMap[NB_BEAM];
  int beamStatusMap[NB_BEAM];
  
  int currentBeamStatus_ = 0;
  OSCManager * myOSCManager_;
  Adafruit_MCP23017 * mcp_;

  public:
  LaserKeyboard(Adafruit_MCP23017 * mcp);
  void setup();
  void loop();
  void setBeamIdToNote(int beamId, int note);
  void process_beam_event(int beamId, bool value);
  
};
