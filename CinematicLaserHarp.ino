#include <ESP8266WiFi.h>

#include <LiquidCrystal_I2C.h>

#include <Adafruit_PWMServoDriver.h>

#include "Adafruit_MCP23017.h"


#include "sequencer.h"
#include "LaserKeyboard.h"
#include "LaserHarpFixture.h"

#include <Artnet.h>
Artnet artnet;

char ssid[] = "SFR_34A8";                   // your network SSID (name)
char pass[] = "ab4ingrograstanstorc";       // your network password



LaserKeyboard myLaserKeyboard;
LaserHarpFixture myLaserHarpFixture;
Sequencer  * mySequencer_p;
Adafruit_MCP23017 mcp;
Adafruit_PWMServoDriver pwm = Adafruit_PWMServoDriver(0x40);
#define SERVOMIN  150 // this is the 'minimum' pulse length count (out of 4096)
#define SERVOMAX  600 // this is the 'maximum' pulse length count (out of 4096)


uint8_t servonum = 0;
#define NB_SERVO 7

#define LCD_I2C_ADDR 0x27 // <<—– Mettre votre adresse

LiquidCrystal_I2C lcd(LCD_I2C_ADDR,16,2);  // set the LCD for a 16 chars and 2 line display

void setup() {
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


    //setup IO expander
    mcp.begin();      // use default address 0
    for(int i = 0; i < NB_BEAM; i++){
        mcp.pinMode(i, OUTPUT);
    }
    for(int i = 8; i < 8 + NB_BEAM; i++){
        mcp.pinMode(i, OUTPUT);
    }
    //mcp.digitalWrite(0, HIGH);
    
    //setup servos
    pwm.begin();
    pwm.setPWMFreq(60);  // Analog servos run at ~60 Hz updates


    //setup LCD
    lcd.begin();

    // Turn on the blacklight and print a message.
    lcd.backlight();
    lcd.print("Hello, world!");

    //SetupArtnet
    artnet.begin();


   // mySequencer_p = new Sequencer(myLaserHarpFixture.beamVector);
    //mySequencer.setupLightSequence();
    //mySequencer.startLightSequence();
}



//========================================================
//Menu state machine

//========================================================


//========================================================






void init_all()
{
  myLaserKeyboard.setup();
  

  myLaserHarpFixture.init_beam_position();
  
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

  myLaserKeyboard.loop();
//  mySequencer.lightSequenceLoop();
  //displayLoop();
  //ultrasonicLoop();
  //ArtNet input loop
  //artNetLoop();

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
