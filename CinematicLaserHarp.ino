#include <ESP8266WiFi.h>

#include <LiquidCrystal_I2C.h>

#include "Adafruit_MCP23017.h"

#include "sequencer.h"
#include "LaserKeyboard.h"
#include "LaserHarpFixture.h"

#include <Artnet.h>
Artnet artnet;


//char ssid[] = "slyzic-hotspot";                   // your network SSID (name)
//char pass[] = "totototo";       // your network password

char ssid[] = "SFR_34A8";                   // your network SSID (name)
char pass[] = "ab4ingrograstanstorc";       // your network password

const int sclPin = D1;
const int sdaPin = D2;

int c = 0;


int buttonPressRequest = false;
bool longPressOngoing = false;

int buttonPressRequestType = 0;
#define SHORT_PRESS 1
#define LONG_PRESS 2

int  previousButtonState = 0;  
int  newButtonState = 0; 
int  buttonSameStateCount = 0;

IPAddress  localAddress(0,0,0,0);

#define HMI_IDLE 0
#define HMI_SEQUENCE_IDLE 1
#define HMI_SEQUENCE_ONGOING 2
#define HMI_DMX_IDLE 3
#define HMI_DMX_ONGOING 4
#define HMI_SETTINGS_IDLE 5
#define HMI_SETTINGS_WIFI 6
#define HMI_INFO 7
#define HMI_LASERHARP_IDLE 8
#define HMI_LASERHARP_ONGOING 9
#define HMI_LASERHARP_EDIT 10
#define HMI_LASERHARP_SAVING 11
#define HMI_LASERHARP_SAVING_NEW 12
#define HMI_SETTINGS_CALIBRATION_IDLE 13
#define HMI_SETTINGS_CALIBRATION_EDIT 14
#define HMI_SETTINGS_CALIBRATION_SAVING 15

int currentLHBeamEditing = 0;
int editValue = 0;
int HMI_State = HMI_IDLE;

#define NB_MAX_CONNECTION_RETRY 16


LaserKeyboard * myLaserKeyboard_p;
LaserHarpFixture myLaserHarpFixture;
Sequencer  * mySequencer_p;
Adafruit_MCP23017 mcp;


//#define LCD_I2C_ADDR 0x3F // <<—– Mettre votre adresse
#define LCD_I2C_ADDR 0x27 // <<—– Mettre votre adresse

LiquidCrystal_I2C lcd(LCD_I2C_ADDR,16,2);  // set the LCD for a 16 chars and 2 line display
uint8_t bell[8] = {0x4,0xe,0xe,0xe,0x1f,0x0,0x4};
uint8_t note[8] = {0x2,0x3,0x2,0xe,0x1e,0xc,0x0};
uint8_t clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};
uint8_t heart[8] = {0x0,0xa,0x1f,0x1f,0xe,0x4,0x0};
uint8_t duck[8] = {0x0,0xc,0x1d,0xf,0xf,0x6,0x0};
uint8_t check[8] = {0x0,0x1,0x3,0x16,0x1c,0x8,0x0};
uint8_t cross[8] = {0x0,0x1b,0xe,0x4,0xe,0x1b,0x0};
uint8_t retarrow[8] = { 0x1,0x1,0x5,0x9,0x1f,0x8,0x4};
#define LCD_CUSTOM_NOTE 0

#define BUTTON_NONE 0  /*!< Pas de bouton appuyé */
#define   BUTTON_UP 1    /*!< Bouton UP (haut) */
#define   BUTTON_DOWN 2  /*!< Bouton DOWN (bas) */
#define   BUTTON_LEFT 3  /*!< Bouton LEFT (gauche) */
#define   BUTTON_RIGHT 4 /*!< Bouton RIGHT (droite) */
#define   BUTTON_SELECT 5 /*!< Bouton SELECT */

byte getPressedButton() {

  /* Lit l'état des boutons */
  int value = analogRead(A0);
 //Serial.println(value);
  /* Calcul l'état des boutons */
  if (value < 50)
    return BUTTON_RIGHT;
  else if (value < 380){
    Serial.print("Analog value");
    Serial.println(value);
    return BUTTON_UP;}
  else if (value < 500){
    Serial.print("Analog value");
    Serial.println(value);
    return BUTTON_DOWN;}
  else if (value < 700)
    return BUTTON_LEFT;
  else if (value < 950)
    return BUTTON_SELECT;
  else
    return BUTTON_NONE;
}
void setup() {
    Serial.begin(115200);

   
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

    lcd.createChar(LCD_CUSTOM_NOTE, note); // Sends the custom char to lcd
    delay(100); // wait a little bit to avoid having corrupted display custom char
    lcd.begin();
    // Turn on the blacklight and print a message.
    lcd.backlight();
    lcd.setCursor(3, 0);
    lcd.print("Cinematic");
    lcd.setCursor(2, 1);
    lcd.print((char)LCD_CUSTOM_NOTE);
    lcd.print("Laser Harp");
    lcd.print((char)LCD_CUSTOM_NOTE);
    
    delay(2000);


    // Connect to WiFi network
    Serial.println();
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);
    WiFi.begin(ssid, pass);

    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Connecting...");


    int nbConnectionTry = 0;
    while ((WiFi.status() != WL_CONNECTED) and (nbConnectionTry < NB_MAX_CONNECTION_RETRY)){
        delay(500);
        Serial.print(".");
        lcd.setCursor(nbConnectionTry,1);
        lcd.print(".");
        
        nbConnectionTry++;
       
    }
    if(WiFi.status() == WL_CONNECTED)
    {
      Serial.println("");
      Serial.println("WiFi connected");
      Serial.println("IP address: ");
      localAddress = WiFi.localIP();
      Serial.println(localAddress);
      
    }
    

    

    //initialize fixture positions and state
    myLaserHarpFixture.setup();



    mySequencer_p = new Sequencer((std::vector<Fixture*>*)&(myLaserHarpFixture.beamVector));
    mySequencer_p->setupLightSequence();

    //beam keyboard setup
    myLaserKeyboard_p = new LaserKeyboard(&mcp, &lcd);     

    stateMachineStateAction(HMI_IDLE);

   
}



//========================================================
//Menu state machine

//========================================================
String buttonToString(int buttonVal)
{
  switch(buttonVal)
  {
     case BUTTON_UP:
       return "BUTTON_UP";
     case BUTTON_DOWN:
       return "BUTTON_DOWN";
     case BUTTON_LEFT:
       return "BUTTON_LEFT";
     case BUTTON_RIGHT:
       return "BUTTON_RIGHT";
     case BUTTON_SELECT:
       return "BUTTON_SELECT";   
  }
}

void stateMachineStateAction(int state)
{
   HMI_State = state;
   int newOffset;
   int tempIndex;
   switch(state)
   {
       case HMI_IDLE:
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("HOME"); 
         lcd.setCursor(0, 1);    
         lcd.print(localAddress);  
         myLaserHarpFixture.resetPosition(); 
       break;
       
       case HMI_SEQUENCE_IDLE:
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("SEQUENCE"); 
         lcd.setCursor(0, 1);    
         lcd.print("SELECT TO START");   
         break;
       
       case HMI_SEQUENCE_ONGOING:
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("SEQUENCE STARTED");  
         lcd.setCursor(0, 1);
         lcd.blink();
         
         mySequencer_p->startLightSequence(true); 
         break;

        case HMI_LASERHARP_IDLE:
           lcd.clear();
           lcd.noBlink();
           lcd.setCursor(0, 0);
           lcd.print("SELECT LASERHARP");  
           lcd.setCursor(0, 1);
           //currentLHNotes = currentLHNotes[currentLHNotes];
           lcd.print(myLaserKeyboard_p->getCurrentPreset());              
           myLaserKeyboard_p->loadPreset(myLaserKeyboard_p->getCurrentPreset());  
                    
           break;
        case HMI_LASERHARP_ONGOING: 
              
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("LASERHARP:PS ");  
          lcd.print(myLaserKeyboard_p->getCurrentPreset());
          myLaserHarpFixture.setLaserHarpInitPosition();
          myLaserHarpFixture.powerAllBeams(true);
          lcd.blink();

          break;

        case HMI_LASERHARP_EDIT: 
              
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("EDITING :PS ");  
          lcd.print(myLaserKeyboard_p->getCurrentPreset());
          lcd.setCursor(0, 1);   
          lcd.print("BEAM ");
          lcd.print(currentLHBeamEditing);
          lcd.print(" : ");
          myLaserKeyboard_p->notePresets_[myLaserKeyboard_p->getCurrentPreset()][currentLHBeamEditing] += editValue;
          lcd.print(myLaserKeyboard_p->getNoteFromMidiNb(myLaserKeyboard_p->notePresets_[myLaserKeyboard_p->getCurrentPreset()][currentLHBeamEditing])); 
          editValue = 0;    
          lcd.blink();

          break;        

        case HMI_LASERHARP_SAVING: 
              
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("SAVED :PS ");  
          tempIndex = myLaserKeyboard_p->getCurrentPreset();
          lcd.print(tempIndex);
          myLaserKeyboard_p->storePreset(tempIndex,  &(myLaserKeyboard_p->notePresets_[tempIndex]));
          lcd.blink();
          delay(3000);
          stateMachineStateAction(HMI_LASERHARP_IDLE);
          break;    

        case HMI_LASERHARP_SAVING_NEW: 
              
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("NEW PRESET :PS ");  
          myLaserKeyboard_p->addPresetFromCurrent(); //TODO use temporary preset instead
          myLaserKeyboard_p->loadPreset(myLaserKeyboard_p->notePresets_.size()-1);
          lcd.print(myLaserKeyboard_p->getCurrentPreset());

          lcd.blink();
          delay(2000);
          stateMachineStateAction(HMI_LASERHARP_IDLE);
          break; 

        case HMI_SETTINGS_CALIBRATION_IDLE: 
          currentLHBeamEditing = 0;
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BEAM CALIBRATION ");  
             
          break;

        case HMI_SETTINGS_CALIBRATION_EDIT: 
              
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("BEAM CALIBRATION ");  
          lcd.setCursor(0, 1);   
          lcd.print("BEAM ");
          lcd.print(currentLHBeamEditing);
          lcd.print(" : ");
          newOffset = myLaserHarpFixture.beamVector[currentLHBeamEditing]->getPositionOffset() + editValue;
          myLaserHarpFixture.beamVector[currentLHBeamEditing]->setPositionOffset(newOffset);
          myLaserHarpFixture.beamVector[currentLHBeamEditing]->setPosition(CENTER_POSITION);
          lcd.print(newOffset); 
          editValue = 0;    
          lcd.blink();

          break;

         case HMI_SETTINGS_CALIBRATION_SAVING: 
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("SAVED : CALIBRATION ");  


          myLaserHarpFixture.storeCalibration();
          lcd.blink();
          delay(3000);
          stateMachineStateAction(HMI_SETTINGS_CALIBRATION_IDLE);

         break;
         
        default :
          Serial.print("ERROR: state action unknown : ");
          Serial.println(state);
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("ERROR: state action unknown : ");
          lcd.println(state);    
          break;       
   }
}


int stateMachineTransition(int buttonVal, int pressType)
{
   switch(HMI_State)
   {
       case HMI_IDLE:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  stateMachineStateAction(HMI_LASERHARP_IDLE); 
               break; 
               
               case BUTTON_UP:
                  HMI_State = HMI_SETTINGS_IDLE; 
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("SETTINGS");                                   
               break; 
               case BUTTON_SELECT:
                  HMI_State = HMI_INFO;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("Cinematic Laser Harp");                     
                  lcd.setCursor(0, 1);
                  lcd.print("v1.0"); 
               break; 
           }
       break;
       
       case HMI_LASERHARP_IDLE:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  //stateMachineStateAction(HMI_SEQUENCE_IDLE);   
                  stateMachineStateAction(HMI_SEQUENCE_ONGOING);    
               break; 
               case BUTTON_UP:
                   stateMachineStateAction(HMI_IDLE);                
               break; 
               case BUTTON_SELECT:
                   if(pressType == SHORT_PRESS)
                       stateMachineStateAction(HMI_LASERHARP_ONGOING);  
                   else
                       stateMachineStateAction(HMI_LASERHARP_EDIT);  
                                    
               break; 
               case BUTTON_RIGHT:
                   myLaserKeyboard_p->loadPreset(myLaserKeyboard_p->getCurrentPreset() + 1);
                   stateMachineStateAction(HMI_LASERHARP_IDLE);                   
               break; 
               case BUTTON_LEFT:
                   
                   myLaserKeyboard_p->loadPreset(myLaserKeyboard_p->getCurrentPreset() - 1);                  
                   stateMachineStateAction(HMI_LASERHARP_IDLE); 
               break;                
           }
       break;

       case HMI_LASERHARP_EDIT:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  editValue = -1;
                  stateMachineStateAction(HMI_LASERHARP_EDIT);    
               break; 
               case BUTTON_UP:
                  editValue = 1;
                  stateMachineStateAction(HMI_LASERHARP_EDIT);                
               break; 
               case BUTTON_LEFT:
                  currentLHBeamEditing -= 1;
                  stateMachineStateAction(HMI_LASERHARP_EDIT);                
               break; 
               case BUTTON_RIGHT:
                  currentLHBeamEditing += 1;
                  stateMachineStateAction(HMI_LASERHARP_EDIT);                
               break; 
               case BUTTON_SELECT:
                   if(pressType == SHORT_PRESS)
                       stateMachineStateAction(HMI_LASERHARP_SAVING);  
                   else
                       stateMachineStateAction(HMI_LASERHARP_SAVING_NEW);  
                                    
               break;          
           }
       break;
       
       case HMI_LASERHARP_ONGOING:
           //any button press stops the Laserharp and goes back to Laserharp idle
           //todo : stop OSC
           if(buttonVal == BUTTON_RIGHT)
           {
           myLaserHarpFixture.powerAllBeams(false);
           stateMachineStateAction(HMI_LASERHARP_IDLE);
           }  
       break;
 
       
       
       
       case HMI_SEQUENCE_IDLE:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  HMI_State = HMI_DMX_IDLE;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("START DMX");   
               break; 
               case BUTTON_UP:
                  stateMachineStateAction(HMI_LASERHARP_IDLE);                         
               break; 
               case BUTTON_SELECT:
                  stateMachineStateAction(HMI_SEQUENCE_ONGOING);  
               break; 
           }
       break;

       case HMI_SEQUENCE_ONGOING:
           //any button press stops the sequence and goes back to sequence idle
           //todo : stop sequence
           lcd.noBlink();
            //TODO : implement stop in sequencer loop
           mySequencer_p->stopLightSequence(); 
           
           stateMachineStateAction(HMI_SEQUENCE_IDLE);  
           break;
           
       case HMI_DMX_IDLE:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  HMI_State = HMI_SETTINGS_IDLE;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("SETTINGS");  
               break; 
               case BUTTON_UP:
                  stateMachineStateAction(HMI_SEQUENCE_IDLE);                
               break; 
               case BUTTON_SELECT:
                  HMI_State = HMI_DMX_ONGOING;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("DMX STARTED"); 
                  lcd.setCursor(0, 1);
                  lcd.print("ADDRESS : ");
                  lcd.print(myLaserHarpFixture.getDmxAddress());
                  //SetupArtnet
                  artnet.begin();
                  
                   
               break; 
           }
       break;
       case HMI_DMX_ONGOING:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  HMI_State = HMI_SETTINGS_IDLE;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("SETTINGS");  
               break; 
               case BUTTON_UP:
                  //TODO : stop DMX
                  stateMachineStateAction(HMI_SEQUENCE_IDLE);             
               break; 
               case BUTTON_SELECT:
                  HMI_State = HMI_DMX_IDLE;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("START DMX"); 
               break; 
           }
       break;

             
       case HMI_SETTINGS_IDLE:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                //  stateMachineStateAction(HMI_SETTINGS_CALIBRATION_IDLE);    
                  stateMachineStateAction(HMI_SETTINGS_CALIBRATION_EDIT);   
               break; 
               case BUTTON_UP:
                  HMI_State = HMI_DMX_IDLE;  
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("START SEQUENCE");                
               break; 
               case BUTTON_SELECT:
                  HMI_State = HMI_SETTINGS_WIFI;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("WIFI ACCESS POINT");  
                  lcd.setCursor(0, 1);
                  lcd.print(ssid);                    
               break; 
           }
       break;

       case HMI_SETTINGS_CALIBRATION_IDLE:
           switch(buttonVal)
           {
               case BUTTON_UP:
                  stateMachineStateAction(HMI_SETTINGS_IDLE);    
               break; 
               case BUTTON_DOWN:
                  stateMachineStateAction(HMI_IDLE);    
               break; 
               case BUTTON_SELECT:
                   stateMachineStateAction(HMI_SETTINGS_CALIBRATION_EDIT);  
               break;          
           }
       break;   
       case HMI_SETTINGS_CALIBRATION_EDIT:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  editValue = -1;
                  stateMachineStateAction(HMI_SETTINGS_CALIBRATION_EDIT);    
               break; 
               case BUTTON_UP:
                  editValue = 1;
                  stateMachineStateAction(HMI_SETTINGS_CALIBRATION_EDIT);                
               break; 
               case BUTTON_LEFT:
                  if (currentLHBeamEditing > 0)
                  {
                      currentLHBeamEditing -= 1;
                      stateMachineStateAction(HMI_SETTINGS_CALIBRATION_EDIT);                
                  }
               break; 
               case BUTTON_RIGHT:
                  if (currentLHBeamEditing < NB_BEAM -1)
                  {
                      currentLHBeamEditing += 1;
                      stateMachineStateAction(HMI_SETTINGS_CALIBRATION_EDIT);       
                  }         
               break; 
               case BUTTON_SELECT:
                   if(pressType == LONG_PRESS)
                       stateMachineStateAction(HMI_SETTINGS_CALIBRATION_SAVING);  
                   else
                       stateMachineStateAction(HMI_SETTINGS_CALIBRATION_IDLE);  
                                    
               break;          
           }
       break;   
   
   }
}

void displayLoop()
{
  if(buttonPressRequest)
  {
    stateMachineTransition(buttonPressRequest, buttonPressRequestType);
/*    if(buttonPressRequestType == SHORT_PRESS)
    {   
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("short ");
      lcd.setCursor(0, 1);
      lcd.print(buttonToString(buttonPressRequest));
    }
    if(buttonPressRequestType == LONG_PRESS)
    {
      lcd.clear();
      lcd.setCursor(0, 0);
      lcd.print("long ");
      lcd.setCursor(0, 1);
      lcd.print(buttonToString(buttonPressRequest));
     // mySequencer_p->startLightSequence(true);
   
    } */ 

    buttonPressRequest = 0;
  }  
  
}


//========================================================


void artNetLoop()
{
   if (artnet.read() == ART_DMX)
  {
    
    //TODO : get Address from config
    uint8_t* dmxFrames = artnet.getDmxFrame();
    myLaserHarpFixture.applyDmxCommands(dmxFrames);
    
  }

}

void ultrasonicLoop()
{
  
}


void buttonLoop()
{
  previousButtonState = newButtonState;  
  newButtonState      = getPressedButton();

  if((newButtonState == BUTTON_NONE) && (previousButtonState != BUTTON_NONE))
  {
       if(!longPressOngoing)
       {
         buttonPressRequest = previousButtonState;
         if(buttonSameStateCount < 100)
         {
           Serial.print("Button short pressed: ");
           Serial.println(previousButtonState);
           buttonPressRequestType = SHORT_PRESS;
         }
         buttonSameStateCount = 0;
       }
       else
       {
         longPressOngoing = false;
       }
       
  }
  
  if((newButtonState == previousButtonState) && (newButtonState != BUTTON_NONE))
  {
    buttonSameStateCount +=1;
       if(buttonSameStateCount > 100)
       {
         Serial.print("Button long pressed : ");
         Serial.println(previousButtonState);
         buttonPressRequest = newButtonState;
         longPressOngoing = true;
         buttonPressRequestType = LONG_PRESS;    
         buttonSameStateCount = 0;      
         newButtonState = BUTTON_NONE;
         previousButtonState = BUTTON_NONE;
       }
  }
}


void loop() {
  displayLoop();
  buttonLoop();
  
  if(HMI_State == HMI_LASERHARP_ONGOING)
      myLaserKeyboard_p->loop();

  if(HMI_State == HMI_SEQUENCE_ONGOING)
      mySequencer_p->lightSequenceLoop();

  //ultrasonicLoop();
  //ArtNet input loop
  if(HMI_State == HMI_DMX_ONGOING)
    artNetLoop();

  delay(10);

  
}
