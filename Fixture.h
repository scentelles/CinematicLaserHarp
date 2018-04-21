#ifndef FIXTURE_H
#define FIXTURE_H

class Fixture
{
  public:
  Fixture();
  virtual void setPosition(int position) = 0;
  virtual void setInitPosition() = 0;
  
};

#endif
