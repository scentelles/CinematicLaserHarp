#include "OSCManager.h"

#define NB_BEAM 7

class LaserKeyboard{
  public:
  int beamIdToNoteMap[NB_BEAM];
  int beamToPinMap[NB_BEAM];
  int beamStatusMap[NB_BEAM];
  
  int currentBeamStatus_ = 0;
  OSCManager * myOSCManager_;
  

  public:
  LaserKeyboard();
  void setup();
  void loop();
  void setBeamIdToNote(int beamId, int note);
  void process_beam_event(int beamId, bool value);
  
};
