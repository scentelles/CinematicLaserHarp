#include <ESP8266WiFi.h>

#include <LiquidCrystal_I2C.h>
#include "Adafruit_MCP23017.h"

#include "sequencer.h"
#include "LaserKeyboard.h"
#include "LaserHarpFixture.h"

#include <Artnet.h>
Artnet artnet;
#define CLH_ADDRESS 16

char ssid[] = "SFR_34A8";                   // your network SSID (name)
char pass[] = "ab4ingrograstanstorc";       // your network password

const int sclPin = D1;
const int sdaPin = D2;
int leftButtonPressCount = 0;
bool leftButtonPressRequest = false;
bool leftButtonLongPressRequest =  false;
int nbLeftPress = 0;
int nbLeftLongPress = 0;

#define BUTTON_LEFT D5


LaserKeyboard * myLaserKeyboard_p;
LaserHarpFixture myLaserHarpFixture;
Sequencer  * mySequencer_p;
Adafruit_MCP23017 mcp;


#define LCD_I2C_ADDR 0x3F // <<—– Mettre votre adresse

LiquidCrystal_I2C lcd(LCD_I2C_ADDR,16,2);  // set the LCD for a 16 chars and 2 line display

void setup() {
    Serial.begin(115200);

    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

//TODO : add Wifi timeout
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");

    Serial.println("WiFi connected");
    Serial.println("IP address: ");
    Serial.println(WiFi.localIP());

    //Start I2C
    Wire.begin(sdaPin, sclPin);
    
    //setup IO expander
    //MCP base address is hard coded(0x20)
    mcp.begin();      // use default address 0
    for(int i = 0; i < NB_BEAM; i++){
        mcp.pinMode(i, OUTPUT);
    }
    for(int i = 8; i < 8 + NB_BEAM; i++){
        mcp.pinMode(i, INPUT);
        mcp.pullUp(i, HIGH); 
    }


    //setup LCD

    lcd.begin();
    // Turn on the blacklight and print a message.
    lcd.backlight();
    lcd.setCursor(3, 0);
    lcd.print("Cinematic");
    lcd.setCursor(3, 1);
    lcd.print("Laser Harp");
    delay(2000);
    lcd.clear();
    lcd.print("Mode : IDLE");
    lcd.cursor();
    lcd.blink();

    
    //SetupArtnet
    artnet.begin();

    myLaserHarpFixture.setup();

    mySequencer_p = new Sequencer((std::vector<Fixture*>*)&(myLaserHarpFixture.beamVector));

    mySequencer_p->setupLightSequence();

    //beam keyboard setup
    myLaserKeyboard_p = new LaserKeyboard(&mcp);     
    myLaserKeyboard_p->setup();


   
}



//========================================================
//Menu state machine

//========================================================

void displayLoop()
{
  if(leftButtonPressRequest)
  {
    lcd.clear();
    lcd.print("Counter : ");
    lcd.print(nbLeftPress);
    leftButtonPressRequest = false;

    lcd.clear();
    lcd.print("Mode : IDLE");
  }  
  if(leftButtonLongPressRequest)
  {
    leftButtonLongPressRequest = false;

    lcd.clear();
    lcd.print("Sequence started");
    
    mySequencer_p->startLightSequence(true);

    
  }  
}


//========================================================






void artNetLoop()
{
   if (artnet.read() == ART_DMX)
  {
    //TODO : get Address from config
    int newLaserValue= artnet.getDmxFrame()[CLH_ADDRESS - 1 + 0];
    myLaserHarpFixture.beamVector[0]->setPower(newLaserValue);
    int newLaserPosition= artnet.getDmxFrame()[CLH_ADDRESS - 1 + 1];
    myLaserHarpFixture.beamVector[0]->setPosition(newLaserPosition);
    Serial.println();
    Serial.println();
  }

}

void ultrasonicLoop()
{
  
}


void buttonLoop()
{
  int newLeftButtonState = digitalRead(BUTTON_LEFT);
  
  if(newLeftButtonState == 0)
  {
    leftButtonPressCount +=1;
    //Serial.println(leftButtonPressCount);
    if(leftButtonPressCount == 2){
       Serial.println("Left Button pressed");
       nbLeftPress ++;
       leftButtonPressRequest = true;

       
    }
    if(leftButtonPressCount == 100){
       Serial.println("Left Button long pressed");
       nbLeftLongPress ++;
       leftButtonLongPressRequest = true;
    }
  }
  else
  {
    leftButtonPressCount = 0;
  }
}


void loop() {

  myLaserKeyboard_p->loop();
  mySequencer_p->lightSequenceLoop();
  displayLoop();
  buttonLoop();
  //ultrasonicLoop();
  //ArtNet input loop
  artNetLoop();




  delay(10);

  
}
