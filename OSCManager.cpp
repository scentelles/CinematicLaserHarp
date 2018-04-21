
#include "OSCManager.h"

OSCManager::OSCManager()
{
  outIP_ = new IPAddress(192,168,1,51);        // remote IP of your computer
  outPort_ = 8000;          // remote port to receive OSC
  localPort_ = 8888;        // local port to listen for OSC packets (actually not used for sending)
}
//todo : destructor

void OSCManager::setup()
{
    Serial.println("Starting UDP");
    Udp.begin(localPort_);
    Serial.print("Local port: ");
    Serial.println(Udp.localPort());  
}
  void OSCManager::sendOSCMessage(String msg, int parameter){
    OSCMessage OSCmsg(msg.c_str());
    if (parameter >= 0)
          OSCmsg.add(parameter);
      Udp.beginPacket(*outIP_, outPort_);
      OSCmsg.send(Udp);
      Udp.endPacket();
      OSCmsg.empty(); 
  }
  
  void OSCManager::sendNote(unsigned int note, unsigned int velocity, int duration)
  {
      String prefix  =  "/vkb_midi/0/note/";
    String noteMsg = prefix + String(note);
    sendOSCMessage(noteMsg, velocity);
    if(duration >= 0)
    {
        delay(duration);
        sendOSCMessage(noteMsg, 0);
    }
  }
  
  
  void OSCManager::sendCC(unsigned int nb, unsigned int value)
  {
      String prefix = "/vkb_midi/0/cc/";
    String ccMsg  = prefix + String(nb);
    sendOSCMessage(ccMsg, value);
  }
  
  
  void OSCManager::sendCommand(String command)
  {
    String commandMsg  = "/" + command;
    sendOSCMessage(commandMsg, -1);
    
  }

