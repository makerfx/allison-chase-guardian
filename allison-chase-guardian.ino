#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <Metro.h>

bool debugOptions[10] = {0, 0, 0, 1, 0, 0, 0, 0, 0, 0};   //change default here, helpful for startup debugging

                                                        
const char *debugOptionsText[10] =  {"", "Input","Audio", "Action", "Eye Animation",
                                "Neck Animation","Body Animation"};
                                
#define DEBUG_INPUT               1  //input functions 
#define DEBUG_AUDIO               2  //audio functions 
#define DEBUG_ACTION              3  //action functions 
#define DEBUG_ANIMATION_EYE       4  //Eye Animation functions    //THIS MAY SLOW ANIMATION
#define DEBUG_ANIMATION_NECK      5  //Neck Animation functions   //THIS MAY SLOW ANIMATION                            
#define DEBUG_ANIMATION_BODY      6  //Body Animation functions   //THIS MAY SLOW ANIMATION 
                                     


#define EYE_SPEED 100
#define EYE_SPEED_TARGETING 200


//WARNING - ADJUSTING THIS SETTING COULD LEAD TO 
//EXCESS CURRENT DRAW AND POSSIBLE SYSTEM DAMAGE
#define DEFAULT_BRIGHTNESS 24 //WARNING!!!!!!!!!
//DON'T DO IT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define NUM_NECK_LEDS_PER_RING 144 //it is likely 143, but the pattern math is designed for 144
#define NUM_NECK_RINGS 4
#define NUM_NECK_LEDS NUM_NECK_LEDS_PER_RING * NUM_NECK_RINGS
#define NECK_TOP_GEARS_NUM 6
#define NECK_TOP_GEAR_WIDTH NUM_NECK_LEDS_PER_RING / NECK_TOP_GEARS_NUM
#define TOP_RING_SPEED_MIN 20 //note that ring speed is inverted
#define TOP_RING_SPEED_MAX 60
#define TOP_RING_SPEED_MAX_DECEL 100
#define TOP_RING_DECEL_DISTANCE 7 // LEDs from target where decel initiates
#define TOP_RING_DECEL_FACTOR 10 //higher number yields MORE decel
#define TOP_RING_SPEED_RANDOM_CHECK 18      //on a 1 to 20 random number, anything >= will randomize speed
#define TOP_RING_SPEED_RANDOM_CHECK_TURN 12 //on a 1 to 20 random number, anything >= will randomize speed

#define BOTTOM_RING_SPEED_MIN 20 //note that ring speed is inverted
#define BOTTOM_RING_SPEED_MAX 60
#define BOTTOM_RING_SPEED_MAX_DECEL 100
#define BOTTOM_RING_DECEL_DISTANCE 7 // LEDs from target where decel initiates
#define BOTTOM_RING_DECEL_FACTOR 10 //higher number yields MORE decel
#define BOTTOM_RING_SPEED_RANDOM_CHECK 18      //on a 1 to 20 random number, anything >= will randomize speed
#define BOTTOM_RING_SPEED_RANDOM_CHECK_TURN 12 //on a 1 to 20 random number, anything >= will randomize speed
#define BOTTOM_RING_GEAR_SIZE 36
#define BOTTOM_RING_MAX_CYCLES 3


Metro eyeMetro = Metro(EYE_SPEED);
Metro trMetro = Metro(TOP_RING_SPEED_MAX);      //prime the metros
Metro brMetro = Metro(BOTTOM_RING_SPEED_MAX);

boolean neckTopPattern[NUM_NECK_LEDS_PER_RING];

CRGB neckLEDs[NUM_NECK_LEDS];
#define NECK_DATA_PIN 29 
//17 = left; 29 = third from left 24 = right;

int topRingOffset=0; //top ring offset
int topRingTarget=50; // top ring target
int topRingSpeed=1; //top ring speed
boolean topRingDir = 0;

int bottomRingOffset=0; //top ring offset
int bottomRingTarget=10; // top ring target
int bottomRingTargetCycles=2; //how many zero crossings before landing on target
int bottomRingSpeed=1; //top ring speed
boolean bottomRingDir = 0;

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



                                                     


bool neckFacet[3][BOTTOM_RING_GEAR_SIZE] = { {0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0},    
                                                {0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0}, 
                                                {0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0} };   

#define EYE_NUM_LEDS 245
CRGB eyeLEDs[EYE_NUM_LEDS];
#define EYE_DATA_PIN 17 //left connector


/*
 * OLED System Includes & Globals
 * 
 */

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 32 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



void setup() {
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
  Serial.begin(115200);
  delay(200);

  //display init
  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }
  Serial.println("OLED begun");
  
  // Clear the buffer
  display.clearDisplay();

    // text display tests
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0,0);
  display.println("Allison Chase's");
  display.println("Decayed Guardian");
  display.println("Cosplay");
  display.println("...Initializing...");
  
  display.setCursor(0,0);
  display.display(); // actually display all of the above

  Serial.println("Setup Started.");

  
  LEDS.addLeds<WS2812SERIAL,  NECK_DATA_PIN,   BRG>(neckLEDs,  NUM_NECK_LEDS);
  LEDS.addLeds<WS2812SERIAL,  EYE_DATA_PIN,   BRG>(eyeLEDs,   EYE_NUM_LEDS);
  
  LEDS.setBrightness(DEFAULT_BRIGHTNESS);

  //build the top ring pattern to be copied later
  for (int i=0; i<NECK_TOP_GEARS_NUM; i++) { 
     for (int j=0; j<NECK_TOP_GEAR_WIDTH; j++) {
        if (j==0 || j==3) neckTopPattern[i*NECK_TOP_GEAR_WIDTH + j] = 1;
        else neckTopPattern[i*NECK_TOP_GEAR_WIDTH + j] = 0;
      }
  }

  if (debugOptions[DEBUG_ANIMATION_NECK]) {
    //print the ring pattern
    for (int i=0; i<NUM_NECK_LEDS_PER_RING ; i++) { 
       Serial.print(neckTopPattern[i]);
    }   
    Serial.println("");  
  }

  delay (2000);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Running...");
  display.setCursor(0,0);
  display.display(); // actually display all of the above
  printDebugOptions();

}

void loop() {
  // put your main code here, to run repeatedly:

  if (brMetro.check() == 1) { // check if the bottom ring metro has passed its interval 

    if (bottomRingOffset == 0) bottomRingTargetCycles--;
    if (bottomRingTargetCycles < 0) bottomRingTargetCycles = 0;
  
    if ((bottomRingOffset == bottomRingTarget) && (bottomRingTargetCycles ==0)) {
      bottomRingDir = !bottomRingDir;
      bottomRingTarget = random(0, BOTTOM_RING_GEAR_SIZE);
      bottomRingTargetCycles = random(0, BOTTOM_RING_MAX_CYCLES);
      
      if (random(1,20) >= BOTTOM_RING_SPEED_RANDOM_CHECK_TURN) {
          bottomRingSpeed = random(BOTTOM_RING_SPEED_MIN,BOTTOM_RING_SPEED_MAX);
      }
      brMetro.interval(bottomRingSpeed);
      //Serial.print("reverse - new target = "); Serial.println(bottomRingTarget);
    }
  
    //decel math
    int dist = abs(bottomRingOffset-bottomRingTarget);
    
    if (dist < BOTTOM_RING_DECEL_DISTANCE) {
      int temp_bottomRingSpeed = bottomRingSpeed * (BOTTOM_RING_DECEL_FACTOR/dist);
      if (temp_bottomRingSpeed > BOTTOM_RING_SPEED_MAX_DECEL) {
        temp_bottomRingSpeed = BOTTOM_RING_SPEED_MAX_DECEL;
        brMetro.interval(temp_bottomRingSpeed);
      }
      else {
        //random speed changes
        if (random(1,20) >= BOTTOM_RING_SPEED_RANDOM_CHECK) {
          bottomRingSpeed = random(BOTTOM_RING_SPEED_MIN,BOTTOM_RING_SPEED_MAX);
        }
        brMetro.interval(bottomRingSpeed);
      }
    }
    
    if (bottomRingDir) bottomRingOffset++;
    else bottomRingOffset --;
  
    if (bottomRingOffset >= BOTTOM_RING_GEAR_SIZE ) bottomRingOffset = 0;
    if (bottomRingOffset < 0) bottomRingOffset = BOTTOM_RING_GEAR_SIZE-1;
    
    if (debugOptions[DEBUG_ANIMATION_NECK]) {
      Serial.println(""); //Serial.print(bottomRingOffset);Serial.println("->");
    }

    bool tempLEDs[3][NUM_NECK_LEDS_PER_RING];

    //init the array with zeros.... NEEDED?
    for (int r = 0; r<3; r++) {
      for (int l = 0; l<NUM_NECK_LEDS_PER_RING; l++) {
        tempLEDs[r][l]=0; //clear 
      }
    }
   
    
    for (int r = 0; r<NUM_NECK_RINGS-1; r++) {                                                          //for each ring
      int curLED = 0;
      for (int g = 0; g<NUM_NECK_LEDS_PER_RING / (BOTTOM_RING_GEAR_SIZE) ; g++) {                // for each gear

        if (g%2==0) { //even number
                                                                    
            for (int l=0; l<BOTTOM_RING_GEAR_SIZE; l++) {
              int lo =  BOTTOM_RING_GEAR_SIZE - bottomRingOffset + l;
              if (lo > BOTTOM_RING_GEAR_SIZE -1) lo= lo- BOTTOM_RING_GEAR_SIZE;
              //Serial.print(lo);Serial.print(" ");
              tempLEDs[r][curLED] = neckFacet[r][lo];         //map facet pattern 
              curLED++;  
            } //end for l
            //Serial.println("");

            
       
       }
       else { //odd number
           int gearEnd = curLED - 1 ; //last LED of prior gear       
   
           for (int l=0; l<BOTTOM_RING_GEAR_SIZE; l++) {                                         
            tempLEDs[r][curLED] = tempLEDs[r][gearEnd - l];
            curLED++;
            } //end for l
        }

        
        //Serial.print(" ");
        } //end for l
    } //end for r 
   
  
    if (debugOptions[DEBUG_ANIMATION_NECK]) {
      for (int r = 0; r<NUM_NECK_RINGS-1; r++) { //for each ring
  
        Serial.print(r); Serial.print(": ");
        for (int l = 0; l<NUM_NECK_LEDS_PER_RING; l++) {
          if (tempLEDs[r][l] == 1) Serial.print("#");
          else Serial.print(" ");
          tempLEDs[r][l]=0;                                               //clear is this needed?
        }
        Serial.println("");
      }
    }  
    
  }

  if (trMetro.check() == 1) { // check if the top ring metro has passed its interval 

    if (topRingOffset == topRingTarget) {
      topRingDir = !topRingDir;
      topRingTarget = random(0, NUM_NECK_LEDS_PER_RING);
      if (random(1,20) >= TOP_RING_SPEED_RANDOM_CHECK_TURN) {
          topRingSpeed = random(TOP_RING_SPEED_MIN,TOP_RING_SPEED_MAX);
      }
      trMetro.interval(topRingSpeed);
      //Serial.print("reverse - new target = "); Serial.println(topRingTarget);
    }
  
    //decel math
    int dist = abs(topRingOffset-topRingTarget);
    
    if (dist < TOP_RING_DECEL_DISTANCE) {
      int temp_topRingSpeed = topRingSpeed * (TOP_RING_DECEL_FACTOR/dist);
      if (temp_topRingSpeed > TOP_RING_SPEED_MAX_DECEL) {
        temp_topRingSpeed = TOP_RING_SPEED_MAX_DECEL;
        trMetro.interval(temp_topRingSpeed);
      }
      else {
        //random speed changes
        if (random(1,20) >= TOP_RING_SPEED_RANDOM_CHECK) {
          topRingSpeed = random(TOP_RING_SPEED_MIN,TOP_RING_SPEED_MAX);
        }
        trMetro.interval(topRingSpeed);
      }
    }
    
    if (topRingDir) topRingOffset++;
    else topRingOffset --;
  
    
   
    if (topRingOffset >= NUM_NECK_LEDS_PER_RING ) topRingOffset = 0;
    if (topRingOffset < 0) topRingOffset = NUM_NECK_LEDS_PER_RING-1;
    
    //Serial.println(""); Serial.print(topRingOffset); Serial.print("->");
    
    //copy the pattern onto the LED array starting from the top ring offset
    for (int i=0; i<NUM_NECK_LEDS_PER_RING-1 ; i++) { 
       int lo = NUM_NECK_LEDS_PER_RING - topRingOffset + i;
       if (lo > NUM_NECK_LEDS_PER_RING - 1) lo= lo-NUM_NECK_LEDS_PER_RING ;
       if (neckTopPattern[lo]) neckLEDs[i] = CRGB::Blue; 
       else neckLEDs[i] = CRGB::Black;
    }  
  
  
    FastLED.show();
    debugOptionsCheck();
  } //end trMetro 
   
  if (eyeMetro.check() == 1) { // check if the metopRingOffset has passed its interval 
  
    /*for (int i = 0; i<EYE_PATTERN_LEN; i++) {
      Serial.print(eyeRingStatus[i]);
      Serial.print(" ");
    }e
    Serial.println("");
   */
   
   if (eyeAniMode == 0) updateEyeRingStatus();
   else if (eyeAniMode == 1) updateEyeTargeting();
    
   FastLED.show();
   debugOptionsCheck(); 
   
  }  //end eyeMetro 
  
  
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
      int option;
      
      switch (incomingByte) {
          
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
          
          
          case 'q': break;
          case 'w': break;
          case 'e': break;
          case 'r': eyeAniMode = 1; break;
          
          }
         
         printDebugOptions();
          
      }
       
}

/*
 * printDebugOptions() - this function outputs a debug option list with
 *                       with current status, and provides some input 
 *                       instructions
 *  
 */
void printDebugOptions() {
  Serial.println("\nDebug Options Status");
  Serial.println("Use serial input keys 1 through 9 to change debug options");
  Serial.println("Use serial input keys 0 to turn off all debug options at once");
  Serial.println("Use serial input keys QWER to emulate buttons 0 through 3");
  
  for (int o=1; o<10; o++) {    //we don't use zero
    {
      if (debugOptionsText[o]) //don't print undefined options
        Serial.printf("   Option %d = %s %s\n", o, debugOptions[o]?"ON: ":"OFF:", debugOptionsText[o]);
    }
  }
  Serial.println("\n");       //a little extra padding in the output
}
