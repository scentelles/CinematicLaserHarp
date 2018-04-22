#ifndef FIXTURE_H
#define FIXTURE_H

class Fixture
{
  public:
  int position_ = 0;
  bool statusOn_ = false;
  String name_;
    
  Fixture();
  Fixture(String name);
  virtual void setPosition(int position);
  virtual void setInitPosition();
  virtual void setOn(bool val); //overload in derived class to implement access to on/off (DMW or HW)
  
};

#endif
