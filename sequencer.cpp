
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
  
Sequencer::Sequencer(std::vector<Fixture*> *  fixtureVector_p)
{
   fixtureVector_ = fixtureVector_p;

}

void Sequencer::addStepDefinition()
{
  
}

void Sequencer::setupLightSequence()
{
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][0] = 350;
    sequenceBeamLightTarget[i][0] = 1;
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][1] = 0;
    sequenceBeamLightTarget[i][1] = 0;
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][2] = 350;
    sequenceBeamLightTarget[i][2] = 1;
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][3] = 100;
    sequenceBeamLightTarget[i][3] = 1;
  }
  for(int i = 0; i < NB_BEAM; i++)
  {
    sequenceBeamPosTarget[i][4] = 350;
    sequenceBeamLightTarget[i][4] = 1;
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
    setFixtureOn(i, sequenceBeamLightTarget[i][stepId]) ;
  }

  
  //Set target position for all fixtures
  //sequenceBeamPosTarget[beamId][stepId];
  //innerStepBeamToPositionMap[beamId][innerStep];
  //last one to be LAST_INNER_STEP
  //based on duration/sched_delay => nb innerSteps
  //todo : make progressive move profile. accel/decel 

  //TODO : to be computed based on duration
  nbInnerStep_ = 50;
  for(int beamId = 0; beamId < NB_BEAM; beamId++)
  {
      int currentFixturePosition = (*fixtureVector_)[beamId]->position_;
      Serial.print("========== CurrentBeamPosition : ");
      Serial.println(currentFixturePosition);

      
      float increment = (float)((sequenceBeamPosTarget[beamId][currentLightSequenceStep_] - currentFixturePosition)) / nbInnerStep_;
      Serial.print("========== increment : ");
      Serial.println(increment);
            
      for(int j = 0; j < nbInnerStep_; j++)
      {//todo : put final target value to avoid rounding delta.
          int newPos = currentFixturePosition + (increment * (j+1));
          innerStepBeamToPositionMap[beamId][j] = newPos;
          //Serial.print("========== Setting inner step position : ");
          //Serial.print(j);
          //Serial.print(" : ");
          //Serial.println(newPos);
      }
  }
  //reset inner step
  currentInnerStep_ = 0;
  
}

void Sequencer::moveFixtureToPosition(int fixtureId, int pos)
{
    Serial.print("Moving Fixture ");
    Serial.print(fixtureId);
    Serial.print("to pos : ");
    Serial.println(pos);
    (*fixtureVector_)[fixtureId]->setPosition(pos);

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

    
    (*fixtureVector_)[fixtureId]->setOn(command);

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

