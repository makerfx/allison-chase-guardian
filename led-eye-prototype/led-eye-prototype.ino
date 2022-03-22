#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <Metro.h>

#define EYE_SPEED 100
#define EYE_SPEED_TARGETING 200

Metro eyeMetro = Metro(EYE_SPEED);

//WARNING - ADJUSTING THIS SETTING COULD LEAD TO 
//EXCESS CURRENT DRAW AND POSSIBLE SYSTEM DAMAGE
#define DEFAULT_BRIGHTNESS 24 //WARNING!!!!!!!!!
//DON'T DO IT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



#define EYE_PATTERN_LEN 12
#define NUM_RINGS 8

//9 rings
//uint8_t eyeRingLEDSize[NUM_RINGS] = {60, 48, 40, 32, 24, 16, 12, 8, 1};

//8 rings
uint8_t eyeRingLEDSize[NUM_RINGS] = {48, 40, 32, 24, 16, 12, 8, 1};

boolean eyeRingStatus[EYE_PATTERN_LEN] = {1, 1, 1, 0, 0, 0, 1, 1, 1, 0, 0, 0};

#define EYE_TARGET_ANI_FRAMES 16
boolean eyeTargetAniFrames[EYE_TARGET_ANI_FRAMES][8] = {
                                                    {1,0,0,0,0,0,0,0},
                                                    {0,1,0,0,0,0,0,0},
                                                    {0,0,1,0,0,0,0,0},
                                                    {0,0,0,1,0,0,0,0},
                                                    {0,0,0,0,1,0,0,0},
                                                    {0,0,0,0,0,1,0,0},
                                                    {0,0,0,0,0,0,1,1},
                                                    {0,0,0,0,0,0,1,1},
                                                    {0,0,0,0,0,0,0,0},
                                                    {1,1,1,1,1,1,1,1},
                                                    {1,1,1,1,1,1,1,1},
                                                    {0,0,0,0,0,0,0,0},
                                                    {0,0,0,0,0,0,0,0},
                                                    {0,0,0,0,0,0,0,0},
                                                    {0,0,0,0,0,0,0,0},
                                                    {0,0,0,0,0,0,0,0},
                                                    };


int eyeAniMode = 0;
int eyeTargetAniFrame = 0;

#define EYE_NUM_LEDS 245
CRGB eyeLEDs[EYE_NUM_LEDS];
#define EYE_DATA_PIN 17 //left connector


void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  
  LEDS.addLeds<WS2812SERIAL,  EYE_DATA_PIN,   BRG>(eyeLEDs,   EYE_NUM_LEDS);
   LEDS.setBrightness(DEFAULT_BRIGHTNESS);
}

void loop() {

 if (eyeMetro.check() == 1) { // check if the metopRingOffset has passed its interval 
  
    /*for (int i = 0; i<EYE_PATTERN_LEN; i++) {
      Serial.print(eyeRingStatus[i]);
      Serial.print(" ");
    }
    Serial.println("");
   */
   
   if (eyeAniMode == 0) updateEyeRingStatus();
   else if (eyeAniMode == 1) updateEyeTargeting();
    
   FastLED.show();
   debugOptionsCheck(); 
   
 }  

  
}

void updateEyeTargeting() {

  eyeMetro.interval(EYE_SPEED_TARGETING);
  
  uint8_t lastLED = -1;
  
  for (int r=0; r<NUM_RINGS; r++) {
    Serial.print(r);

    uint8_t firstLED = 0;
    
    firstLED = lastLED + 1;
    lastLED =  lastLED + eyeRingLEDSize[r];

    for (int l=firstLED; l<=lastLED; l++){
      if (eyeTargetAniFrames[eyeTargetAniFrame][r]) 
         eyeLEDs[l] = CRGB::Red;
      else eyeLEDs[l] = CRGB::Black; 
    }

    Serial.print("EyeRing: ");
    Serial.print(r);
    Serial.print(": ");
    Serial.print(firstLED);
    Serial.print(": ");
    Serial.println(lastLED); 
    
    if (eyeTargetAniFrame == EYE_TARGET_ANI_FRAMES) {
      fadeToBlackBy(eyeLEDs, EYE_NUM_LEDS, 80);
      eyeAniMode = 0;
      eyeTargetAniFrame = 0;
      eyeMetro.interval(EYE_SPEED);
    } //end if
  } //end for

  if (eyeAniMode == 1) eyeTargetAniFrame++;
}

void updateEyeRingStatus() {

  eyeMetro.interval(EYE_SPEED);
  //increment the status array
  boolean eyeRingStatusTemp[EYE_PATTERN_LEN];
     
  for (int i = 0; i<EYE_PATTERN_LEN; i++) {
    eyeRingStatusTemp[i] = eyeRingStatus[i];
  }
  
 /* left to right
  for (int i = 0; i<EYE_PATTERN_LEN; i++) {
    if (i==0) {
      eyeRingStatus[i] = eyeRingStatusTemp[EYE_PATTERN_LEN-1];
    }
    else eyeRingStatus[i] = eyeRingStatusTemp[i-1];
  }
  */
  //right to left
  
  for (int i = 0; i<EYE_PATTERN_LEN; i++) {
    if (i==EYE_PATTERN_LEN-1) {
      eyeRingStatus[i] = eyeRingStatusTemp[0];
    }
    else eyeRingStatus[i] = eyeRingStatusTemp[i+1];
  }
  
  
  //update the rings

  uint8_t lastLED = -1;
  
  for (int r = 0; r<NUM_RINGS; r++){
    uint8_t firstLED = 0;
    
    firstLED = lastLED + 1;
    lastLED =  lastLED + eyeRingLEDSize[r];

    for (int l=firstLED; l<=lastLED; l++){
      if (eyeRingStatus[r]) 
         //dim the first and last ring 
         if (r ==0 || r == NUM_RINGS-1) eyeLEDs[l].setHSV(160,255,128); 
         else eyeLEDs[l].setHSV(160,255,255); 
        //eyeLEDs[l] = CRGB::Blue;
      else eyeLEDs[l] = CRGB::Black;
    }

    
    Serial.print("EyeRing: ");
    Serial.print(r);
    Serial.print(": ");
    Serial.print(firstLED);
    Serial.print(": ");
    Serial.println(lastLED);  
     
  }
  
    
}

/*
 * debugOptionsCheck() - this function checks the Serial input and
 *                       changes debug options as well as emulates some 
 *                       button presses
 *                       
 */
void debugOptionsCheck() {
  int incomingByte = 0;
  
  if (Serial.available() > 0) {
      // read the incoming byte:
      incomingByte = Serial.read();
      //int option;
      
      switch (incomingByte) {
          /*
          case '0':                         //turn all options off
            for (int o = 1; o<10; o++) {    //we dont use zero
              debugOptions[o] = 0;
            }
            Serial.println ("All debug options turned OFF");
            break;
            
          case '1' ... '9': 
            option = incomingByte - '0';
            debugOptions[option] = !debugOptions[option]; 
            //Serial.printf("Debug option %d is now %s\n", option, debugOptions[option]?"ON":"OFF");
            break;

          case 'q': mapAction(SOURCE_BUTTON, 0, 0); break;
          case 'w': mapAction(SOURCE_BUTTON, 1, 0); break;
          case 'e': mapAction(SOURCE_BUTTON, 2, 0); break;
          */
          case 't': eyeAniMode = 1; Serial.println("Targeting!"); break;
          }
         
         //rprintDebugOptions();
          
      }
       
}
