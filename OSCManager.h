#include <WiFiUdp.h>
#include <OSCMessage.h>

class OSCManager
{
  public:
    WiFiUDP Udp; 
    // A UDP instance to let us send and receive packets over UDP
    IPAddress * outIP;
    int outPort;          // remote port to receive OSC
    int localPort;        // local port to listen for OSC packets (actually not used for sending)
  
  public:
    OSCManager();
    void sendOSCMessage(String msg, int parameter);
    void sendNote(unsigned int note, unsigned int velocity, int duration);
    void sendCC(unsigned int nb, unsigned int value);
    void sendCommand(String command);
};
