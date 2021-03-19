#if defined(ESP8266)
  #include <ESP8266WiFi.h>
#elif defined(ESP32)
  #include <WiFi.h>
#else
  #error "This ain't a ESP8266 or ESP32"
#endif


#include "SinricPro_Generic.h"
#include "SinricProBlinds.h"


#define WIFI_SSID                             "Your_ss_id"    
#define WIFI_PASSWORD                         "Your_password"
#define APP_KEY                               "YOUR APP_KEY AS IN VİDEO"      // Should look like "de0bxxxx-1x3x-4x3x-ax2x-5dabxxxxxxxx"
#define APP_SECRET                            "YOUR APP_SECRET AS IN VİDEO"   // Should look like "5f36xxxx-x3x7-4x3x-xexe-e86724a9xxxx-4c4axxxx-3x3x-x5xe-x9x3-333d65xxxxxx"
#define BLINDS_ID                             "YOUR BLINDS_ID AS IN VİDEO"    // Should look like "5dc1564130xxxxxxxxxxxxxx"
#define BAUD_RATE                             115200                // Change baudrate to your need


int temp;

#define addr  0x80

//Set the settings to 1 able to learn full motion delay
#define settings 0
// Set the debug parameter to 1 if you are dealing with debug
#define debug 1
unsigned long necessary_time_for_whole_motion =105000;
unsigned long necessary_time_for_percent_motion=necessary_time_for_whole_motion/100;

//Initialize the first position of the blind
volatile int required_blind_pos;
volatile int current_blind_pos ;

float difference=0;

/*The usage of pins for dc motor
You can use the other pwm pins of esp*/ 
uint8_t motor_neg =  D4;
uint8_t motor_pos =  D3;

bool powerState = false;
bool reboot;

// define the functions of the smart blind project
void wifibegin();
void blind_action();
void open_blind();
void close_blind();
void gosettings();
bool onPowerState(const String &deviceId, bool &state) ;
bool onSetPosition(const String &deviceId, int &position);


bool onPowerState(const String &deviceId, bool &state ) {
  #if debug
   Serial.printf("Device %s is powered", deviceId.c_str());
  #endif
  powerState = state;

  return true; // request handled properly
}

bool onSetPosition(const String &deviceId, int &position) {
    //Take the desired percentage from sinric pro
    required_blind_pos = position;
    #if debug
    Serial.printf("Device %s set position to %d\r\n", deviceId.c_str(), required_blind_pos);
    #endif
    delay(1500);
    blind_action();
    return true; // request handled properly
}

void wifibegin(){
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED){
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();

}
/*setup and connect to sinric pro with your APP_KEY AND APP_SECRET*/
void setupSinricPro() {
  
  SinricProBlinds &myBlinds = SinricPro[BLINDS_ID];
  myBlinds.onPowerState(onPowerState);
  myBlinds.onSetPosition(onSetPosition);

  SinricPro.onConnected([](){ Serial.printf("Connected to SinricPro\r\n"); }); 
  SinricPro.onDisconnected([](){ Serial.printf("Disconnected from SinricPro\r\n"); });
  SinricPro.begin(APP_KEY, APP_SECRET);
}
void setup() {
  Serial.begin(BAUD_RATE);
  while (!Serial);
  delay(1000);
  Serial.print("current_blind_pos");Serial.println(current_blind_pos);
  delay(100);

  pinMode(motor_neg,OUTPUT);pinMode(motor_pos,OUTPUT);
  Serial.println("\nStarting Blinds on " + String(ARDUINO_BOARD));
  Serial.println("Version : " + String(SINRICPRO_VERSION_STR));
  if(settings){
    gosettings();
  }
  wifibegin();
  setupSinricPro();
}


void loop() {
  SinricPro.handle();
}
/*Note that Blind_pos taken from sinric pro app is noting but the shadow percentage */

void blind_action(){
  if(current_blind_pos>required_blind_pos){
    open_blind();  
  }
  else if(current_blind_pos<required_blind_pos){
    close_blind();
  }
  else{
    return;
  }
}
void open_blind(){
  Serial.println("curtain is opened");
  difference=abs(current_blind_pos-required_blind_pos);
  #if debug
  Serial.print("difference: ");Serial.println(difference);
  Serial.print("delay:");Serial.println(necessary_time_for_percent_motion*difference);
  #endif
  digitalWrite(motor_neg,LOW);
  digitalWrite(motor_pos,HIGH);
  /*Wait for motor motion */
  delay( necessary_time_for_percent_motion*difference);
  digitalWrite(motor_neg,LOW);
  digitalWrite(motor_pos,LOW);
  current_blind_pos=required_blind_pos;
}  
void close_blind(){
  Serial.println("curtain is closed");
  difference=abs(current_blind_pos-required_blind_pos);
  #if debug
  Serial.println("difference");Serial.print(difference);
  Serial.println("delay");Serial.print(necessary_time_for_percent_motion*difference);
  #endif
  digitalWrite(motor_neg,HIGH);
  digitalWrite(motor_pos,LOW);
   /*Wait for motor motion */
  delay(necessary_time_for_percent_motion*difference);
  digitalWrite(motor_neg,LOW);
  digitalWrite(motor_pos,LOW);
  Serial.println("curtain is closed");
  current_blind_pos=required_blind_pos;  
}  
String ok;
void gosettings(){
  unsigned long time_for_whole_motion = millis();
  digitalWrite(motor_neg,LOW);
  digitalWrite(motor_pos,HIGH);
  for(;;){
    if (Serial.available() > 0) {  
       ok = Serial.readStringUntil('\n');
       if(ok == "ok"){
          necessary_time_for_whole_motion = millis() - time_for_whole_motion;
          necessary_time_for_percent_motion =  necessary_time_for_whole_motion/100;
          Serial.print("necessary time for full motion");Serial.println(necessary_time_for_whole_motion);
          Serial.print("necessary time for one percent motion");Serial.println(necessary_time_for_whole_motion);
          current_blind_pos = 0;// You can use directly if the settings is done by opening blinds.
          break;   
       }    
    }
  }
  digitalWrite(motor_neg,LOW);
  digitalWrite(motor_pos,LOW);
  
}
