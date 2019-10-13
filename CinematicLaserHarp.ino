#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>

#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ArduinoJson.h>          //https://github.com/bblanchon/ArduinoJson

#include <LiquidCrystal_I2C.h>

#include "Adafruit_MCP23017.h"

#include "sequencer.h"
#include "LaserKeyboard.h"
#include "LaserHarpFixture.h"

#include <Artnet.h>
Artnet artnet;

WiFiManager wifiManager;

//I2C configuration
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

#define HMI_IDLE 0
#define HMI_RESET_WIFI_SETTINGS 17
#define HMI_SEQUENCE_IDLE 1
#define HMI_SEQUENCE_ONGOING 2
#define HMI_DMX_IDLE 3
#define HMI_DMX_ONGOING 4
#define HMI_SETTINGS_IDLE 5
#define HMI_SETTINGS_EDIT 6
#define HMI_SETTINGS_SAVING 16
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
int currentSettingEditing = 0;
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
//uint8_t clock[8] = {0x0,0xe,0x15,0x17,0x11,0xe,0x0};
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

//===============================================================
//WifiManager definitions
//===============================================================
int ip[4];
char osc_remote_IP[40];
IPAddress * remoteIPAddressConfig;

//flag for saving data
bool shouldSaveConfig = false;

//WIFI Manager callbacks
//callback notifying us of the need to save config
void saveConfigCallback () {
  Serial.println("Should save config");
  shouldSaveConfig = true;
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Saving config");
  delay(1000);

}

void configModeCallback (WiFiManager *myWiFiManager) {
  
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("No Wifi found");
  lcd.setCursor(0,1);
  lcd.print("Starting AP");
  delay(2000);
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(myWiFiManager->getConfigPortalSSID());
  lcd.setCursor(0,1);
  lcd.print(WiFi.softAPIP());
  
}

void setup() {
    //Serial.begin(115200);
       
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
    lcd.createChar(LCD_CUSTOM_NOTE, note); // Sends the custom char to lcd
    delay(500); // wait a little bit to avoid having corrupted display custom char

    lcd.clear();
    // Turn on the blacklight and print init message.
    lcd.backlight();
    lcd.setCursor(3, 0);
    lcd.print("Cinematic");
    lcd.setCursor(2, 1);
    lcd.print((char)LCD_CUSTOM_NOTE);
    lcd.print("Laser Harp");
    lcd.print((char)LCD_CUSTOM_NOTE);


    // this will be called for each packet received
    artnet.setArtDmxCallback(onDmxFrame);
    
    delay(2000);

//============================================================
//WIFI CONFIGURATION 
//============================================================  
  //read configuration from FS json
  Serial.println("mounting FS...");

  if (SPIFFS.begin()) {
    Serial.println("mounted file system");
    if (SPIFFS.exists("/config.json")) {
      //file exists, reading and loading
      Serial.println("reading config file");
      // SPIFFS.remove("/config.json");
      exit;
      File configFile = SPIFFS.open("/config.json", "r");
      if (configFile) {
        Serial.println("opened config file");
        size_t size = configFile.size();
        // Allocate a buffer to store contents of the file.
        std::unique_ptr<char[]> buf(new char[size]);

        configFile.readBytes(buf.get(), size);
        DynamicJsonBuffer jsonBuffer;
        JsonObject& json = jsonBuffer.parseObject(buf.get());
        json.printTo(Serial);
        if (json.success()) {
          Serial.println("\nparsed json");
          strcpy(osc_remote_IP, json["osc_remote_IP"]);
        } 
        else 
        {
          Serial.println("failed to load json config");
        }
        configFile.close();
      }
    }
  } 
  else 
  {
    Serial.println("failed to mount FS");
  }
  //end read


  wifiManager.setBreakAfterConfig(true);
  //reset saved settings
  //wifiManager.resetSettings();
  
  WiFiManagerParameter OSC_remoteaddress("OSC_IP", "OSC remote address", osc_remote_IP, 40);
  wifiManager.addParameter(&OSC_remoteaddress);
  
  //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
  wifiManager.setAPCallback(configModeCallback);

  //set config save notify callback
  wifiManager.setSaveConfigCallback(saveConfigCallback);
  // wifiManager.setSaveParamsCallback(saveParamsCallback);


  //fetches ssid and tries to connect
  //if it does not connect it starts an access point
  //and goes into a blocking loop awaiting configuration
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONNECTING...");  
  lcd.blink();
  if (!wifiManager.autoConnect("LASERHARP_CONFIG")) {
    //Saving config in all cases
    Serial.println("saving config");
    DynamicJsonBuffer jsonBuffer;
    JsonObject& json = jsonBuffer.createObject();
    Serial.print("New remote IP : ");
    Serial.println(OSC_remoteaddress.getValue());
    json["osc_remote_IP"] = OSC_remoteaddress.getValue();
    File configFile = SPIFFS.open("/config.json", "w");
    if (!configFile) {
      Serial.println("failed to open config file for writing");
    }

    json.printTo(Serial);
    json.printTo(configFile);
    configFile.close();
    //end save
    
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("RESTARTING...");
    delay(2000);
    ESP.restart();

  }

  //if you get here you have connected to the WiFi
  Serial.println("CONNECTED!");
  Serial.println("local ip");
  Serial.println(WiFi.localIP());
    
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("CONNECTED !");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.SSID());
  delay(1500);
    
  //read updated parameters
  strcpy(osc_remote_IP, OSC_remoteaddress.getValue());
 
  Serial.println("parsing IP address from config");
  sscanf(osc_remote_IP, "%d.%d.%d.%d", &ip[0], &ip[1], &ip[2], &ip[3]);
  remoteIPAddressConfig = new IPAddress(ip[0], ip[1], ip[2], ip[3]);


//==============================================================
//                    INIT 
//==============================================================

    myLaserHarpFixture.setup();

    mySequencer_p = new Sequencer((std::vector<Fixture*>*)&(myLaserHarpFixture.beamVector));
    mySequencer_p->setupLightSequence();

    myLaserKeyboard_p = new LaserKeyboard(&mcp, &lcd, remoteIPAddressConfig);     

    stateMachineStateAction(HMI_IDLE);
   
}



//========================================================
//Menu state machine
//========================================================

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
  // Serial.print("HMI STATE ACTION :");
  // Serial.println(state);
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
         lcd.print( WiFi.localIP());  
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
         lcd.print(myLaserKeyboard_p->getCurrentPreset());              
       
         myLaserKeyboard_p->loadPreset(myLaserKeyboard_p->getCurrentPreset());  
       break;
          
       case HMI_LASERHARP_ONGOING: 
         lcd.clear();
       
         myLaserHarpFixture.setLaserHarpInitPosition();
         myLaserHarpFixture.powerAllBeams(true);
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

       case HMI_DMX_ONGOING:
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("DMX STARTED"); 
          lcd.setCursor(0, 1);
          lcd.print("ADDRESS : ");
          lcd.print(myLaserHarpFixture.getDmxAddress());
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
         lcd.print("SAVING CALIBRATION");  
         lcd.blink();
         delay(3000);

         myLaserHarpFixture.storeCalibration();
         stateMachineStateAction(HMI_SETTINGS_CALIBRATION_IDLE);
       break;
         
       default :
         Serial.print("ERROR: state action unknown : ");
         Serial.println(state);
         lcd.clear();
         lcd.setCursor(0, 0);
         lcd.print("ERROR : FSM");
         lcd.setCursor(0, 1);
         lcd.println(state);    
       break;       
   }
}


int stateMachineTransition(int buttonVal, int pressType)
{
 //  Serial.print("HMI TRANSITION :");
 //  Serial.println(buttonVal);
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
                  if(pressType == SHORT_PRESS)
                  {
                    HMI_State = HMI_INFO;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("Cinematic Laser Harp");                     
                    lcd.setCursor(0, 1);
                    lcd.print("v1.0"); 
                  }
                  else
                  {
                    HMI_State = HMI_RESET_WIFI_SETTINGS;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("LONG PRESS FOR");                     
                    lcd.setCursor(0, 1);
                    lcd.print("  WIFI RESET"); 
                  }

                  
               break; 
           }
       break;


       case HMI_INFO:
          //For all buttons, go back to Idle
          stateMachineStateAction(HMI_IDLE);          
       break;
       
       case HMI_RESET_WIFI_SETTINGS:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
               case BUTTON_UP:
                  stateMachineStateAction(HMI_LASERHARP_IDLE); 
               break; 
               
               
               case BUTTON_SELECT:
                  if(pressType == LONG_PRESS)
                  {
                    HMI_State = HMI_INFO;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("WIFI RST STARTED");                     
                    wifiManager.resetSettings();
                    delay(2000);
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("WIFI RESET DONE");                       
                  }
                  else
                  {
                    HMI_State = HMI_RESET_WIFI_SETTINGS;
                    lcd.clear();
                    lcd.setCursor(0, 0);
                    lcd.print("LONG PRESS FOR");                     
                    lcd.setCursor(0, 1);
                    lcd.print("  WIFI RESET"); 
                    lcd.setCursor(0, 1);
                    lcd.print("PLEASE REBOOT"); 
                  }
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
                  artnet.begin();
                  stateMachineStateAction(HMI_DMX_ONGOING);
               break; 
           }
       break;
       
       case HMI_DMX_ONGOING:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
               case BUTTON_UP:
                  HMI_State = HMI_DMX_IDLE;
                  artnet.stop();
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("START DMX");  
               break; 

               case BUTTON_RIGHT:
                  myLaserHarpFixture.setDmxAddress(myLaserHarpFixture.getDmxAddress() + 1);
                  stateMachineStateAction(HMI_DMX_ONGOING);
               break; 

               case BUTTON_LEFT:
                  myLaserHarpFixture.setDmxAddress(myLaserHarpFixture.getDmxAddress() - 1);
                  stateMachineStateAction(HMI_DMX_ONGOING);
               break; 
               
               case BUTTON_SELECT:
                  // if(pressType == LONG_PRESS)
                  // {
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("STORING....");  
                      myLaserHarpFixture.storeDmxAddress();
                      
                      lcd.clear();
                      lcd.setCursor(0, 0);
                      lcd.print("NEW DMX ADDRESS");  
                      lcd.setCursor(0, 1);
                      lcd.print("    STORED"); 
                      delay(2000);
                      stateMachineStateAction(HMI_DMX_ONGOING);                                              
                  // }
               break; 
           }
       break;
             
       case HMI_SETTINGS_IDLE: //TODO: add settings (debouncing value, DMX address, artnet address?)
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
                  HMI_State = HMI_SETTINGS_EDIT;
                  lcd.clear();
                  lcd.setCursor(0, 0);
                  lcd.print("EDIT DMX SETTINGS");         
               break; 
           }
       break;

       case HMI_SETTINGS_EDIT:
           switch(buttonVal)
           {
               case BUTTON_DOWN:
                  editValue = -1;
                  stateMachineStateAction(HMI_SETTINGS_EDIT);    
               break; 
               
               case BUTTON_UP:
                  editValue = 1;
                  stateMachineStateAction(HMI_SETTINGS_EDIT);                
               break; 
               
               case BUTTON_LEFT:
                  currentSettingEditing -= 1;
                  stateMachineStateAction(HMI_SETTINGS_EDIT);                
               break; 
               
               case BUTTON_RIGHT:
                  currentSettingEditing += 1;
                  stateMachineStateAction(HMI_SETTINGS_EDIT);                
               break; 
               
               case BUTTON_SELECT:
                  stateMachineStateAction(HMI_SETTINGS_SAVING);  
  
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


//========================================================
//                   PROCESSING LOOPS
//========================================================
void displayLoop()
{
  if(buttonPressRequest)
  {
    stateMachineTransition(buttonPressRequest, buttonPressRequestType);
    buttonPressRequest = 0;
  }  
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data, IPAddress remoteIP)
{
  
     myLaserHarpFixture.applyDmxCommands(data);

}


void artNetLoop()
{

    artnet.read();
    myLaserHarpFixture.strobe();

}

void ultrasonicLoop()
{
  
}

void buttonLoop()
{
  previousButtonState = newButtonState;  
  newButtonState      = getPressedButton();

  //Short press detection
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

  //Long press detection
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

//========================================================
//                   MAIN LOOP
//========================================================
void loop() {
  displayLoop();
  buttonLoop();
  
  if(HMI_State == HMI_LASERHARP_ONGOING)
      myLaserKeyboard_p->loop();

  if(HMI_State == HMI_SEQUENCE_ONGOING)
      mySequencer_p->lightSequenceLoop();

  if(HMI_State == HMI_DMX_ONGOING)
    artNetLoop();

/*
  if(Serial.available() > 0) // did something come in?
  {
    char tempC = Serial.read();
    Serial.print("Received keypress : ");
    Serial.println(tempC);
    if(tempC == '4')
      buttonPressRequest = BUTTON_LEFT;
    if(tempC == '6')
      buttonPressRequest = BUTTON_RIGHT;
    if(tempC == '8')
      buttonPressRequest = BUTTON_UP;
    if(tempC == '2')
      buttonPressRequest = BUTTON_DOWN;
    if(tempC == '5')
      buttonPressRequest = BUTTON_SELECT;

    buttonPressRequestType = SHORT_PRESS;
      
  }*/



  //ultrasonicLoop();
  
  delay(10);
  
}
