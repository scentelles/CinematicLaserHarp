#ifndef FIXTURE_H
#define FIXTURE_H

class Fixture
{
  public:
  int position_ = 0;
  int dimmerVal_ = 0;
  int strobeFreq_ = 0;
  int strobeCount_ = 0;
  bool strobeForce = false;
  String name_;
    
  Fixture();
  Fixture(String name);
  virtual void setPosition(int position);
  virtual void setInitPosition();
  virtual void setPower(int val); //overload in derived class to implement access to dimming/power
  virtual void setStrobeFreq(int val);
  virtual void strobe();  
};

#endif
