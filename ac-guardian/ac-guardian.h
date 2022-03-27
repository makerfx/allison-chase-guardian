#include <WS2812Serial.h>
#define USE_WS2812SERIAL
#include <FastLED.h>
#include <Metro.h>
#include "OSUKeyboard.h"

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

/*
 * Audio System Includes & Globals
 * Reference the Audio Design Tool - https://www.pjrc.com/teensy/gui/index.html
 * 
 */
#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>

// GUItool: begin automatically generated code
AudioPlaySdWav           playSdWav2;     //xy=328.00390625,393.99999809265137
AudioPlaySdWav           playSdWav1;     //xy=330.00390625,334.99999809265137
AudioPlaySdWav           playSdWav3;     //xy=330.00390625,452.99999809265137
AudioMixer4              mixer2;         //xy=536.0039176940918,488.9999990463257
AudioMixer4              mixer1;         //xy=540.0039253234863,313.00000953674316
AudioOutputI2S           i2s1;           //xy=712.0039176940918,389.00001335144043
AudioConnection          patchCord1(playSdWav2, 0, mixer1, 1);
AudioConnection          patchCord2(playSdWav2, 1, mixer2, 1);
AudioConnection          patchCord3(playSdWav1, 0, mixer1, 0);
AudioConnection          patchCord4(playSdWav1, 1, mixer2, 0);
AudioConnection          patchCord5(playSdWav3, 0, mixer1, 2);
AudioConnection          patchCord6(playSdWav3, 1, mixer2, 2);
AudioConnection          patchCord7(mixer2, 0, i2s1, 1);
AudioConnection          patchCord8(mixer1, 0, i2s1, 0);
AudioControlSGTL5000     sgtl5000_1;     //xy=271.00390625,256.99999809265137
// GUItool: end automatically generated code


#define NUM_CHANNELS       3     //SD Card playback is best to not exceed 3 simultaneous WAVs
                                 //There is an SD card test in the Audio examples

#define CHANNEL_MUSIC      0     //Only one item can play per channel
#define CHANNEL_SFX1       1     //Use these to map sound types to a channel
#define CHANNEL_SFX2       2     

#define LEVEL_CHANNEL0    .5    //change these for relative channel levels
#define LEVEL_CHANNEL1    .5
#define LEVEL_CHANNEL2    .5 

#define STARTING_VOLUME       .8 //change this to reduce clipping in the main amp
#define MAX_VOLUME            1.0 
#define MIN_VOLUME            .2 
#define VOLUME_INCREMENT      .1 //how much the volume changes when using buttons

//I use this syntax so that I can leave the declarations above which come from the Audio Design tool
AudioPlaySdWav *channels[NUM_CHANNELS] = { &playSdWav1, &playSdWav2, &playSdWav3 };
String playQueue[NUM_CHANNELS];

#define NUM_ACTIONS 4

#define ACTION_FADE_OUT_EYE        0
#define ACTION_FADE_OUT_NECK       1
#define ACTION_FADE_OUT_BODY       2
#define ACTION_PLAY_LAZER_SOUND    3


unsigned long actionQueue[NUM_ACTIONS];
const char *actionsText[NUM_ACTIONS]={"Fade Out Eye", "Fade Out Neck", "Fade Out Body"}; 

float mainVolume = STARTING_VOLUME;
bool musicLoop = 0;
bool firstLoop = 1;

// Use these with the Teensy 3.5 & 3.6 SD card
#define SDCARD_CS_PIN    BUILTIN_SDCARD
#define SDCARD_MOSI_PIN  11  // not actually used
#define SDCARD_SCK_PIN   13  // not actually used

/*
 * USB Setup including OSU keyboard
 */
USBHost myusb;
USBHub hub1(myusb);

USBHIDParser hid1(myusb);

OSUKeyboard osukey1(myusb);

USBDriver *drivers[] = {&hub1, &hid1};
USBHIDInput *hiddrivers[] = {&osukey1};


bool debugOptions[10] = {0, 1, 1, 1, 1, 1, 1, 0, 0, 0};   //change default here, helpful for startup debugging

                                                        
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

#define BODY_SPEED 100
#define BODY_SPOT_GAP_COLOR CRGB::DeepPink
#define BODY_SPOT_COLOR CRGB::HotPink 
#define BODY_HUE 248 //pink   ;  0 is red
#define BODY_SPOT_LEN_MIN 18
#define BODY_SPOT_LEN_MAX 30
#define BODY_SPOT_GAP_LEN_MIN 20
#define BODY_SPOT_GAP_LEN_MAX 40
#define NUM_BODY_LEDS 300
CRGB bodyLEDs[NUM_BODY_LEDS];
CRGB bodyPattern[NUM_BODY_LEDS];
#define BODY_DATA_PIN 24 //right connector



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

#define BODY_SPEED_MIN 80 //note that  speed is inverted
#define BODY_SPEED_MAX 120
#define BODY_SPEED_MAX_DECEL 100
#define BODY_DECEL_DISTANCE 7 // LEDs from target where decel initiates
#define BODY_DECEL_FACTOR 10 //higher number yields MORE decel
#define BODY_SPEED_RANDOM_CHECK 18      //on a 1 to 20 random number, anything >= will randomize speed
#define BODY_SPEED_RANDOM_CHECK_TURN 12 //on a 1 to 20 random number, anything >= will randomize speed


Metro eyeMetro = Metro(EYE_SPEED);
Metro trMetro = Metro(TOP_RING_SPEED_MAX);      //prime the metros
Metro brMetro = Metro(BOTTOM_RING_SPEED_MAX);
Metro bodyMetro = Metro(BODY_SPEED);
Metro queueMetro = Metro(50);

boolean neckTopPattern[NUM_NECK_LEDS_PER_RING];

CRGB neckLEDs[NUM_NECK_LEDS];
#define NECK_DATA_PIN 29 
//17 = left; 29 = third from left 24 = right;

int topRingOffset=0; //top ring offset
int topRingTarget=50; // top ring target
int topRingSpeed=1; //top ring speed
boolean topRingDir = 0;

int bottomRingOffset=0; 
int bottomRingTarget=10; 
int bottomRingTargetCycles=2; //how many zero crossings before landing on target
int bottomRingSpeed=1; 
boolean bottomRingDir = 0;

int bodyOffset=0;
int bodyTarget=50; 
int bodySpeed=1; 
boolean bodyDir = 0;

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


uint8_t eyeAniMode = 0;
uint8_t neckAniMode = 0;
uint8_t bodyAniMode = 0;

int eyeTargetAniFrame = 0;



                                                     


bool neckFacet[3][BOTTOM_RING_GEAR_SIZE] = { {0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0},    
                                                {0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0,0,1,0,1,0,0}, 
                                                {0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0,0,1,1,1,0,0,0,1,0,1,0,0} };   

#define NUM_EYE_LEDS 245
CRGB eyeLEDs[NUM_EYE_LEDS];
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
