#include <ESP8266WiFi.h>

#include <LiquidCrystal_I2C.h>

#include <Adafruit_PWMServoDriver.h>

#include "Adafruit_MCP23017.h"

#include "OSCManager.h"
#include "sequencer.h"

#include <Artnet.h>
Artnet artnet;

char ssid[] = "SFR_34A8";                   // your network SSID (name)
char pass[] = "ab4ingrograstanstorc";       // your network password

int currentBeamStatus = 0;

OSCManager myOSCManager;
Sequencer  mySequencer;

Adafruit_MCP23017 mcp;

// called this way, it uses the default address 0x40
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver();
#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pulse length count (out of 4096)
// our servo # counter
uint8_t servonum = 0;
#define NB_SERVO 7

#define LCD_I2C_ADDR 0x27 // <<—– Mettre votre adresse

LiquidCrystal_I2C lcd(LCD_I2C_ADDR,16,2);  // set the LCD for a 16 chars and 2 line display

void setup() {
  // put your setup code here, to run once:
    Serial.begin(115200);

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    Serial.println("Starting UDP");
    myOSCManager.Udp.begin(myOSCManager.localPort);
    Serial.print("Local port: ");
    Serial.println(myOSCManager.Udp.localPort());

//setup IO expander
    mcp.begin();      // use default address 0
    mcp.pinMode(0, OUTPUT);
    //mcp.digitalWrite(0, HIGH);
    
//setup servos
  pwm.begin();
  pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates


//setup LCD
  // initialize the LCD
  lcd.begin();

  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.print("Hello, world!");

//SetupArtnet
artnet.begin();


    mySequencer.setupLightSequence();
    mySequencer.startLightSequence();
}



//========================================================
//Menu state machine

//========================================================


//========================================================


int beamIdToNoteMap[NB_BEAM];
int beamToPinMap[NB_BEAM];
int beamStatusMap[NB_BEAM];
int beamPositionMap[NB_BEAM];
int beamOffsetMap[NB_BEAM];



void setBeamPosition(int beamId, int value)
{
    //TODO : implement servo control
}

void init_beam_position()
{
  for(int i = 0; i < NB_BEAM; i++)
  {
      beamPositionMap[0] = 0;
  }

  //todo : get from EEPROM
  //TODO : calibrate offset to have beams in center position at init
  beamOffsetMap[0] = 0;
  beamOffsetMap[1] = 0;
  beamOffsetMap[2] = 0;
  beamOffsetMap[3] = 0;
  beamOffsetMap[4] = 0;
  beamOffsetMap[5] = 0;
  beamOffsetMap[6] = 0;

  for(int i = 0; i < NB_BEAM; i++)
  {
    setBeamPosition(i, beamOffsetMap[i]);
  }
}

void init_all()
{
  //todo : get from EEPROM
  //init beam to note map.
  beamIdToNoteMap[0] = 42;
  beamIdToNoteMap[1] = 43;
  beamIdToNoteMap[2] = 44;
  beamIdToNoteMap[3] = 45;
  beamIdToNoteMap[4] = 46;
  beamIdToNoteMap[5] = 47;
  beamIdToNoteMap[6] = 48;


  beamToPinMap[0] = D5;
  beamToPinMap[1] = D6;
  beamToPinMap[2] = D7;
  beamToPinMap[3] = D7;
  beamToPinMap[4] = D7;
  beamToPinMap[5] = D7;
  beamToPinMap[6] = D7;

  

  init_beam_position();
  
}

void setBeamIdToNote(int beamId, int note)
{

  //store in local
  beamIdToNoteMap[beamId] = note;
  //store in EEPROM
}

void process_beam_event(int beamId, bool value)
{
      //  sendCC(51, newFilter);
        myOSCManager.sendNote(beamIdToNoteMap[beamId], 120, 100);
  
}


void beamSensorLoop()
{

  for(int i = 0; i < NB_BEAM; i++)
  {
      //todo : replace with I2C IO read
      currentBeamStatus = digitalRead(beamToPinMap[i]);
      
      //update and take action only if the status has changed
      if(currentBeamStatus != beamStatusMap[i])
      {
          //update the beam status.
          beamStatusMap[i] = currentBeamStatus;
          //process the corresponding event trigger action
          process_beam_event(i, currentBeamStatus);
      }
      
  }
}

void buttonsLoop()
{
  //up
  //down
  //enter
  //back (long enter?)
}

void artNetLoop()
{
   if (artnet.read() == ART_DMX)
  {
    // print out our data
    Serial.print("universe number = ");
    Serial.print(artnet.getUniverse());
    Serial.print("\tdata length = ");
    Serial.print(artnet.getLength());
    Serial.print("\tsequence n0. = ");
    Serial.println(artnet.getSequence());
    Serial.print("DMX data: ");
    for (int i = 0 ; i < artnet.getLength() ; i++)
    {
      Serial.print(artnet.getDmxFrame()[i]);
      Serial.print("  ");
    }
    Serial.println();
    Serial.println();
  }
}

void ultrasonicLoop()
{
  
}

void loop() {

  beamSensorLoop();
  mySequencer.lightSequenceLoop();
  //displayLoop();
  //ultrasonicLoop();
  //ArtNet input loop

  Serial.print("Servo num : ");
  Serial.println(servonum);
  for (uint16_t pulselen = SERVOMIN; pulselen < SERVOMAX; pulselen++) {
    pwm.setPWM(servonum, 0, pulselen);
  }
  delay(500);
  for (uint16_t pulselen = SERVOMAX; pulselen > SERVOMIN; pulselen--) {
    pwm.setPWM(servonum, 0, pulselen);
  }
  delay(500);
  servonum ++;
  if (servonum >= NB_SERVO) servonum = 0;

  artNetLoop();
  
}
