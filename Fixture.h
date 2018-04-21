#ifndef FIXTURE_H
#define FIXTURE_H

class Fixture
{
  public:
  int position_ = 0;
    
  Fixture();
  virtual void setPosition(int position);
  virtual void setInitPosition();
  
};

#endif
