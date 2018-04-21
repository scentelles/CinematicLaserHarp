#include "BeamFixture.h"
#include <vector>
#define NB_BEAM 7

class LaserHarpFixture
{
  public:
  std::vector<BeamFixture> beamVector;
  
  public:
  LaserHarpFixture();
  void init_beam_position();
  void setBeamPosition(int beamId, int position);
};
