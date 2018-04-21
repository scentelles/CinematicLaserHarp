
#include "sequencer.h"

#include <ESP8266WiFi.h>

//At setup :
//pass function pointer of the fixture actionner, with target value.

//Sequencer 
// sequenceStep transition time
// sequenceStep duration time
// beamTargetPositionMap
//At beginning of step. calculate steps fo smoothly reaching target position

Sequencer::Sequencer(){}
Sequencer::Sequencer(Fixture * fixture){
    //todo : :remove.just for test of inheritance.
  fixture->setInitPosition();
  }
Sequencer::Sequencer(std::vector<BeamFixture>  fixtureList)
{

}


void Sequencer::setupLightSequence()
{
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][0] = 127;
    sequenceBeamLightTarget[i][0] = 1;
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][1] = -127;
    sequenceBeamLightTarget[i][0] = 0;
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][2] = 0;
    sequenceBeamLightTarget[i][0] = 1;
  }
}

void Sequencer::startLightSequence()
{
  Serial.println("Starting Light sequence");
  if(state_ == S_STARTED)
  {
    Serial.println("WARNING : Light sequence already started. Skipping new start request");
    return;  
  }
  state_ = S_STARTED;
  currentLightSequenceStep_ = 0;
  //init at first step
  setLightSequenceNewStepTargets(0); 
 
}
void Sequencer::stopLightSequence()
{
  state_ = S_IDLE;
  currentLightSequenceStep_ = -1;
  currentInnerStep_ = -2;
}

void Sequencer::setLightSequenceNewStepTargets(int stepId)
{
  Serial.print("Setting up new step : ");
  Serial.println(stepId);
  if(stepId >= lightSequenceNbSteps_)
  {
    Serial.println("Light Sequence : stepId beyond max step. Ending sequence");
    stopLightSequence();
  }
  //set on/off state of the fixture at beginning of the step
  for(int i = 0; i < NB_BEAM; i++)
  {
    setFixtureOn(i, sequenceBeamLightTarget[stepId]) ;
  }

  
  //Set target position for all fixtures
  //sequenceBeamPosTarget[beamId][stepId];
  //innerStepBeamToPositionMap[beamId][innerStep];
  //last one to be LAST_INNER_STEP
  //based on duration/sched_delay => nb innerSteps
  //todo : make progressive move profile. accel/decel 

  //TODO : to be computed based on duration
  nbInnerStep_ = 10;
  for(int beamId = 0; beamId < NB_BEAM; beamId++)
  {
      
      //innerStepBeamToPositionMap[]
  }
  //reset inner step
  currentInnerStep_ = 0;
  
}

void Sequencer::moveFixtureToPosition(int fixtureId, int pos)
{
    //TODO implement servo move
    Serial.print("Moving Fixture ");
    Serial.print(fixtureId);
    Serial.print("to pos : ");
    Serial.println(pos);

}
void Sequencer::setFixtureOn(int fixtureId, bool command)
{
    //TODO implement fixture On/Off
    if(command)
    {
      Serial.print("Turning ON Fixture ");
      Serial.println(fixtureId);
    }
    else
    {
      Serial.print("Turning OFF Fixture ");
      Serial.println(fixtureId);      
    }


}
void Sequencer::lightSequenceLoop()
{
  if(state_ == S_STARTED)
  {
    if(currentLightSequenceStep_ < lightSequenceNbSteps_)
    {
      Serial.print("Current Step : ");
      Serial.println(currentLightSequenceStep_);
      Serial.print("Nb Step : ");
      Serial.println(lightSequenceNbSteps_);     
      Serial.print("Current Inner step : ");
      Serial.println(currentInnerStep_);       
      Serial.print("nbInnerStep : ");
      Serial.println(nbInnerStep_);         
      if(currentInnerStep_ >= nbInnerStep_)
      {
        //Go to next step
        currentLightSequenceStep_ += 1;
        setLightSequenceNewStepTargets(currentLightSequenceStep_);     
      }
      else
      {
        for(int beamId = 0; beamId < NB_BEAM; beamId++)
        {
          moveFixtureToPosition(beamId, innerStepBeamToPositionMap[beamId][currentInnerStep_]);
        }
        currentInnerStep_ += 1;
      }
    }
  }
}

