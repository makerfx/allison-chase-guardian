#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <Metro.h>

Metro trMetro = Metro(30); //just gets it started, then topRingSpeed is used.

//WARNING - ADJUSTING THIS SETTING COULD LEAD TO 
//EXCESS CURRENT DRAW AND POSSIBLE SYSTEM DAMAGE
#define DEFAULT_BRIGHTNESS 24 //WARNING!!!!!!!!!
//DON'T DO IT!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


#define NUM_NECK_LEDS_PER_RING 60 //143
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

boolean neckTopPattern[NUM_NECK_LEDS_PER_RING];

CRGB neckLEDs[NUM_NECK_LEDS];
#define NECK_DATA_PIN 17 //left connector
int topRingOffset=0; //top ring offset
int topRingTarget=50; // top ring target
int topRingSpeed=1; //top ring speed

boolean dir = 0;


void setup() {
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
  Serial.begin(115200);
  delay(200);
  
  LEDS.addLeds<WS2812SERIAL,  NECK_DATA_PIN,   BRG>(neckLEDs,  NUM_NECK_LEDS);
  LEDS.setBrightness(DEFAULT_BRIGHTNESS);

  //build the top ring pattern to be copied later
  for (int i=0; i<NECK_TOP_GEARS_NUM; i++) { 
     for (int j=0; j<NECK_TOP_GEAR_WIDTH; j++) {
        if (j==0 || j==3) neckTopPattern[i*NECK_TOP_GEAR_WIDTH + j] = 1;
        else neckTopPattern[i*NECK_TOP_GEAR_WIDTH + j] = 0;
      }
  }

  //print the ring pattern
  for (int i=0; i<NUM_NECK_LEDS_PER_RING ; i++) { 
     Serial.print(neckTopPattern[i]);
  }   
  Serial.println("");  

  

}

void loop() {
  // put your main code here, to run repeatedly:

  if (trMetro.check() == 1) { // check if the metopRingOffset has passed its interval 

    if (topRingOffset == topRingTarget) {
      dir = !dir;
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
    
    if (dir) topRingOffset++;
    else topRingOffset --;
  
    
   
    if (topRingOffset >= NUM_NECK_LEDS_PER_RING ) topRingOffset = 0;
    if (topRingOffset < 0) topRingOffset = NUM_NECK_LEDS_PER_RING-1;
    
    //Serial.println(""); Serial.print(topRingOffset); Serial.print("->");
    
    //copy the pattern onto the LED array starting from the top ring offset
    for (int i=0; i<NUM_NECK_LEDS_PER_RING-1 ; i++) { 
       int lo = NUM_NECK_LEDS_PER_RING - topRingOffset + i;
       //Serial.print(i); Serial.print(":"); Serial.print(lo); Serial.print(":");
       if (lo > NUM_NECK_LEDS_PER_RING - 1) lo= lo-NUM_NECK_LEDS_PER_RING ;
       //Serial.print(lo);Serial.print(" ");
       if (neckTopPattern[lo]) neckLEDs[i] = CRGB::Blue; 
       else neckLEDs[i] = CRGB::Black;
    }  
  
  
    FastLED.show();
    debugOptionsCheck();
  } //end trMetro 
   
  
  
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
          case 'r': dir = !dir; break;
          }
         
         //rprintDebugOptions();
          
      }
       
}
