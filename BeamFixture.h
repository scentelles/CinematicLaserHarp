#ifndef BEAMFIXTURE_H
#define BEAMFIXTURE_H

#include "Fixture.h"
class BeamFixture: public Fixture{
  public:
  bool isOn = false;
  int position_;
  int positionOffset;
  
  public:
  BeamFixture();

  void setPositionOffset(int offset);
  void setPosition(int position);
  void setInitPosition();
};

#endif
