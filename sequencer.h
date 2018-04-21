#include "BeamFixture.h"

#include <vector>

#define NB_BEAM 7

#define S_IDLE    1
#define S_STARTED 2
#define MAX_INNER_STEP 1000
#define MAX_STEP 10



class Sequencer
{
  private:
  int state_ = S_IDLE;

  int nbInnerStep_ = 0;

  int currentLightSequenceStep_ = -1;
  int currentInnerStep_ = -2;
  int lightSequenceNbSteps_ = 3;
  char innerStepBeamToPositionMap[NB_BEAM][MAX_INNER_STEP];
  char sequenceBeamPosTarget[NB_BEAM][MAX_STEP];
  bool sequenceBeamLightTarget[NB_BEAM][MAX_STEP];
  
  public:
  Sequencer();
  Sequencer(std::vector<BeamFixture> fixtureList);
  Sequencer(Fixture *fixture);
  void lightSequenceLoop();
  void setFixtureOn(int fixtureId, bool command);
  void moveFixtureToPosition(int fixtureId, int pos);
  void setLightSequenceNewStepTargets(int stepId);
  void startLightSequence();
  void stopLightSequence();
  void setupLightSequence();
  
  
};
