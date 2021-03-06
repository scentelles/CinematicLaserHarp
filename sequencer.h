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
  int lightSequenceNbSteps_ = 5;
  short innerStepBeamToPositionMap[NB_BEAM][MAX_INNER_STEP];
  short sequenceBeamPosTarget[NB_BEAM][MAX_STEP];
  short sequenceBeamLightTarget[NB_BEAM][MAX_STEP];
  std::vector<Fixture*> *  fixtureVector_;
  
  public:
  Sequencer();
  Sequencer(std::vector<Fixture*> *  fixtureVector_p);
  void addStepDefinition();
  void lightSequenceLoop();
  void setFixturePower(int fixtureId, int command);
  void moveFixtureToPosition(int fixtureId, int pos);
  void setLightSequenceNewStepTargets(int stepId);
  void startLightSequence(bool loop);
  void stopLightSequence();
  void setupLightSequence();
  
  
};
