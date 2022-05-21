#include "ac-guardian.h"

void setup() {
  // put your setup code here, to run once:
  randomSeed(analogRead(0));
  Serial.begin(115200);
  delay(200);

  //usb setup
  //don't forget to add these for EVERY keyboard :)
  
  osukey1.attachRawPress(OnPress);
  osukey1.attachRawRelease(OnRelease);
  osukey2.attachRawPress(OnPress);
  osukey2.attachRawRelease(OnRelease);
  osukey3.attachRawRelease(OnRelease);
  osukey3.attachRawPress(OnPress);
  osukey4.attachRawRelease(OnRelease);
  osukey4.attachRawPress(OnPress);
  osukey5.attachRawRelease(OnRelease);
  osukey5.attachRawPress(OnPress);
  myusb.begin();
  
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
  display.println(__FILENAME__);
  display.print(__DATE__);
  display.print(" ");
  display.println(__TIME__);
  display.println("Initializing...");
  display.display(); // actually display all of the above

  Serial.println("Setup Started.");

    //setup audio system
  AudioMemory(128);
  sgtl5000_1.enable();
  sgtl5000_1.volume(mainVolume);

  //set relative volumes by channel
  mixer1.gain(0, LEVEL_CHANNEL0);
  mixer2.gain(0, LEVEL_CHANNEL0);
  mixer1.gain(1, LEVEL_CHANNEL1);
  mixer2.gain(1, LEVEL_CHANNEL1);
  mixer1.gain(2, LEVEL_CHANNEL2);
  mixer2.gain(2, LEVEL_CHANNEL2);

  //setup SD card
  SPI.setMOSI(SDCARD_MOSI_PIN);
  SPI.setSCK(SDCARD_SCK_PIN);
  if (!(SD.begin(SDCARD_CS_PIN))) {
    while (1) {
      Serial.println("Unable to access the SD card");
      delay(500);
    }
  }
  
  LEDS.addLeds<WS2812SERIAL,  NECK_DATA_PIN,   BRG>(neckLEDs,  NUM_NECK_LEDS);
  LEDS.addLeds<WS2812SERIAL,  EYE_DATA_PIN,    BRG>(eyeLEDs,   NUM_EYE_LEDS);
  LEDS.addLeds<WS2812SERIAL,  BODY_DATA_PIN,   BRG>(bodyLEDs,  NUM_BODY_LEDS);
  
    
  //LEDS.setBrightness(DEFAULT_BRIGHTNESS);
  updateBrightness(); //will set the default
  
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

  //build the body pattern
  generateBodyPattern();


  delay (2000);
  display.clearDisplay();
  display.setCursor(0,0);
  display.println("Running...");
  display.setCursor(0,0);
  display.display(); // actually display all of the above
  printDebugOptions();

/*
  Serial.println(bodySpotColor.red);
  Serial.println(bodySpotColor.green);
  Serial.println(bodySpotColor.blue);

  Serial.println("");
  Serial.println(bodySpotGapColor.red);
  Serial.println(bodySpotGapColor.green);
  Serial.println(bodySpotGapColor.blue);
 */
  
  //modePowerUp();
}

void loop() {

  EVERY_N_MILLISECONDS(10) {                           
    FastLED.show();
    gHue++; 
  }
  
  EVERY_N_MILLISECONDS(100) {                           
    updateOLED();
    //FastLED.show();
    debugOptionsCheck();
  }
   
   if (queueMetro.check() == 1) { //time to process the queues

    for (int q = 0; q < NUM_ACTIONS; q++) {
      unsigned long qTime = actionQueue[q];
      if (qTime && (millis() > qTime)) {
        if (debugOptions[DEBUG_ACTION]) Serial.printf ("Action Called: %s \n", actionsText[q]);
        if (q==ACTION_FADE_OUT_EYE) eyeAniMode = 0;
        else if (q==ACTION_FADE_IN_EYE) eyeAniMode = 1;       
        else if (q==ACTION_FADE_OUT_NECK) neckAniMode = 0;
        else if (q==ACTION_FADE_IN_NECK) neckAniMode = 1;
        else if (q==ACTION_FADE_OUT_BODY) bodyAniMode = 0;
        else if (q==ACTION_FADE_IN_BODY) bodyAniMode = 1;
        else if (q==ACTION_PLAY_LASER_SOUND) playWAV(CHANNEL_SFX1, "ACGLAZR1.WAV"); //todo: something with the body lights?
        
        else if (q==ACTION_SET_MODE_OFF) mode = MODE_OFF;
        else if (q==ACTION_SET_MODE_IDLE) mode = MODE_IDLE;
        
        actionQueue[q]=0;
      }
    }
    
  

    bool block = false;
    //play any queued music
    for (int q = 0; q < NUM_CHANNELS; q++) {
      String fn = playQueue[q];
      if (fn.length() >0) {
        playQueue[q] = "";
        playWAV(q,fn); 
        block = true;
      }
    }

    //loop BGM if needed - todo: have global for filename to loop
    if ( !block && (musicLoop==true) && !(channels[CHANNEL_MUSIC]->isPlaying())) {
      if(debugOptions[DEBUG_AUDIO]) Serial.println("Audio: restarting music loop");
      //playWAV(CHANNEL_MUSIC, "ACGBATL2.WAV");
      playWAV(CHANNEL_MUSIC, musicLoopFilename);
      if (mode == MODE_POWER_UP) mode = MODE_IDLE;
    }
  } //end playQueueMetro check
  
  if (brMetro.check() == 1) { // check if the bottom ring metro has passed its interval 
    if (neckAniMode == 0) fadeToBlackBy(neckLEDs+NUM_NECK_LEDS_PER_RING, NUM_NECK_LEDS_PER_RING*(NUM_NECK_RINGS-1), 60);
    else if (neckAniMode == 1) updateNeckBottomRings();
    
  }

  if (trMetro.check() == 1) { // check if the top ring metro has passed its interval 
     if (neckAniMode == 0) fadeToBlackBy(neckLEDs, NUM_NECK_LEDS_PER_RING, 60); //just the top ring
     else if (neckAniMode == 1) updateNeckTopRing();
   
 
  } //end trMetro 
   
  if (eyeMetro.check() == 1) { // check if the eye metro has passed its interval 
  
   if (eyeAniMode == 0) fadeToBlackBy(eyeLEDs, NUM_EYE_LEDS, 60);
   else if (eyeAniMode == 1) updateEyeRingStatus();
   else if (eyeAniMode == 2) updateEyeTargeting();
   
  }  //end eyeMetro 
  
  if (bodyMetro.check() == 1) updateBodyLEDs();
  
}

/*
 * generateBodyPattern() - this function generates a random pattern that is then animated
 *                        by updateBodyLEDs()
 *                        
 */
void generateBodyPattern() {
  bool pType = 1;
  CRGB pVal;
  uint8_t pLen;
  
  for (uint16_t l=0; l<NUM_BODY_LEDS; l++) {
    if (pType) {
      pLen = random(BODY_SPOT_LEN_MIN, BODY_SPOT_LEN_MAX);
      //pVal = BODY_SPOT_V;
      //pVal = BODY_SPOT_COLOR;
      pVal = bodySpotColor;

      }
    else {
      pLen = random(BODY_SPOT_GAP_LEN_MIN, BODY_SPOT_GAP_LEN_MAX);
      //pVal = BODY_BASE_V;
      //pVal = BODY_SPOT_GAP_COLOR;
      pVal = bodySpotGapColor;
    }

    for (uint8_t pLED = 0; pLED<pLen; pLED++) {
      //Serial.print(l+pLED);
      
      if (l+pLED>=NUM_BODY_LEDS) break;
      bodyPattern[l+pLED] = pVal;
 
      /*
       if (pVal == bodySpotColor) {
        Serial.print("#");
      }
      else if (pVal == bodySpotGapColor) Serial.print(".");
      */
    }
 
    l = l + pLen - 1;
    if (l>=NUM_BODY_LEDS) break;
    pType = !pType;
  }
}

void updateBodyLEDs() {
  if (bodyAniMode == 0) {
    fadeToBlackBy(bodyLEDs, NUM_BODY_LEDS, 60);
    return;
  }
  else if (bodyAniMode == 1) {//normal body operation

    if (bodyOffset == bodyTarget) {
          bodyDir = !bodyDir;
          bodyTarget = random(0, NUM_BODY_LEDS);
          if (random(1,20) >= BODY_SPEED_RANDOM_CHECK_TURN) {
              bodySpeed = random(BODY_SPEED_MIN,BODY_SPEED_MAX);
          }
          bodyMetro.interval(bodySpeed);
          //Serial.print("reverse - new target = "); Serial.println(bodyTarget);
        }
      
        //decel math
        int dist = abs(bodyOffset-bodyTarget);
        
        if (dist < BODY_DECEL_DISTANCE) {
          int temp_bodySpeed = bodySpeed * (BODY_DECEL_FACTOR/dist);
          if (temp_bodySpeed > BODY_SPEED_MAX_DECEL) {
            temp_bodySpeed = BODY_SPEED_MAX_DECEL;
            bodyMetro.interval(temp_bodySpeed);
          }
          else {
            //random speed changes
            if (random(1,20) >= BODY_SPEED_RANDOM_CHECK) {
              bodySpeed = random(BODY_SPEED_MIN,BODY_SPEED_MAX);
            }
            bodyMetro.interval(bodySpeed);
          }
        }
        
        if (bodyDir) bodyOffset++;
        else bodyOffset --;
      
        
       
        if (bodyOffset >= NUM_BODY_LEDS ) bodyOffset = 0;
        if (bodyOffset < 0) bodyOffset = NUM_BODY_LEDS-1;
        
        //Serial.println(""); Serial.print(bodyOffset); Serial.print("->");
        
        //copy the pattern onto the LED array starting from the top ring offset
        for (int i=0; i<NUM_BODY_LEDS ; i++) { 
           int lo = NUM_BODY_LEDS - bodyOffset + i;
           if (lo > NUM_BODY_LEDS - 1) lo= lo-NUM_BODY_LEDS ;
            bodyLEDs[i] = bodyPattern[lo];
        }  

    }
    else if (bodyAniMode == 2) {//rainbow!!
      //rainbowHue=rainbowHue+10;
      fill_rainbow( bodyLEDs, NUM_BODY_LEDS, gHue, 7);
    }
/*
  //copy the pattern to the LEDs
  for (uint16_t l=0; l<NUM_BODY_LEDS; l++) {
    bodyLEDs[l] = CRGB::Red; 
    bodyLEDs[l] = CHSV(BODY_HUE, 255, bodyPattern[l]);
  }
 */  
 
}
void updateNeckTopRing() {
  
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
    //copy the pattern onto the second row of LEDs in reverse (because I had to make things complicated!)
    for (int i=0; i<NUM_NECK_LEDS_PER_RING-1 ; i++) { 
       neckLEDs[i+NUM_NECK_LEDS_PER_RING] = neckLEDs[NUM_NECK_LEDS_PER_RING-i-1];
    }  

    
}
void updateNeckBottomRings() {
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
    
    /*if (debugOptions[DEBUG_ANIMATION_NECK]) {
      Serial.println(""); //Serial.print(bottomRingOffset);Serial.println("->");
    }
  */
    bool tempLEDs[NUM_NECK_RINGS][NUM_NECK_LEDS_PER_RING];

    //init the array with zeros.... NEEDED?
    for (int r = 0; r<NUM_NECK_RINGS; r++) {
      for (int l = 0; l<NUM_NECK_LEDS_PER_RING; l++) {
        tempLEDs[r][l]=0; //clear 
      }
    }
   
    
    for (int r = 0; r<NUM_NECK_RINGS-1; r++) {                                                          //for each ring
      int curLED = 0;
      for (int g = 0; g<(NUM_NECK_LEDS_PER_RING / BOTTOM_RING_GEAR_SIZE)+1 ; g++) {                // for each gear //+1 is hack to fit

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

   /*
  // crazy debug print version
    if (debugOptions[DEBUG_ANIMATION_NECK]) {
      for (int r = 0; r<NUM_NECK_RINGS-2; r++) { //for each ring
  
        Serial.print(r); Serial.print(": ");
        for (int l = 0; l<NUM_NECK_LEDS_PER_RING; l++) {
          if (tempLEDs[r][l] == 1) Serial.print("#");
          else Serial.print(" ");
          //tempLEDs[r][l]=0;                                               //clear is this needed?
        }
        Serial.println("");
      }
    } 
    */


    //copy the bottom neck rings onto the LED array 
    for (int r = 0; r<NUM_NECK_RINGS-2; r++) { //for each ring
      for (int l = 0; l<NUM_NECK_LEDS_PER_RING; l++) {
          //int ln = (r+1)*NUM_NECK_LEDS_PER_RING+l;
          int ln = 0;
          
          if (r==1 || r ==3) {
              ln = (r+2)*NUM_NECK_LEDS_PER_RING+l; //hack!
          }
          else 
              ln = (r+2+1)*NUM_NECK_LEDS_PER_RING-l-1;
              
          //Serial.print(ln);
          if (tempLEDs[r][l] == 1) {
            neckLEDs[ln] = CRGB::Blue; 
            //Serial.println("*");
          }
          else //Serial.println(""); 
            neckLEDs[ln] = CRGB::Black;                                          
        }  
    }
}

void updateEyeTargeting() {

  eyeMetro.interval(EYE_SPEED_TARGETING);
  
  uint8_t lastLED = -1;
  
  for (int r=0; r<NUM_RINGS; r++) {
    //Serial.print(r);

    uint8_t firstLED = 0;
    
    firstLED = lastLED + 1;
    lastLED =  lastLED + eyeRingLEDSize[r];

    for (int l=firstLED; l<=lastLED; l++){
      if (eyeTargetAniFrames[eyeTargetAniFrame][r]) 
         eyeLEDs[l] = CRGB::Red;
      else eyeLEDs[l] = CRGB::Black; 
    }
    
    if (eyeTargetAniFrame == EYE_TARGET_ANI_FRAMES) {
      fadeToBlackBy(eyeLEDs, NUM_EYE_LEDS, 80);
      eyeAniMode = 1;
      eyeTargetAniFrame = 0;
      if (mode == MODE_ATTACK) mode = MODE_IDLE;
      eyeMetro.interval(EYE_SPEED);
    } //end if
  } //end for

  if (eyeAniMode == 2) eyeTargetAniFrame++;
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
            printDebugOptions();
            break;
   
          case 'q': modePowerUp(); break;
          case 'w': modeAttack(); break;
          case 'e': modeDamaged(); break;
          case 'r': modeDestroyed(); break;
          case 't': volumeUp(); break;
          case 'y': volumeDown(); break;
          case 'c': modeTheChain(); break;
          case 'd': modeVader(); break;
          case 'l': modeLowRider(); break;
          case 'h': modeDrWho(); break;
          
          
          case '<': brightnessDown(); break;
          case '>': brightnessUp(); break;
         // case ',': bodySpotColor+= CRGB(0,10,10); printDebugColor("Body Spot",bodySpotColor); generateBodyPattern(); break;
         // case '.': bodySpotColor-= CRGB(0,10,10); printDebugColor("Body Spot",bodySpotColor); generateBodyPattern(); break;
         // case '<': bodySpotGapColor-= CRGB(10,0,0); printDebugColor("Body Gap",bodySpotGapColor); generateBodyPattern(); break;
         // case '>': bodySpotGapColor+= CRGB(10,0,0); printDebugColor("Body Gap",bodySpotGapColor); generateBodyPattern(); break;
          
          
          }
         
     
          
      }
       
}

void printDebugColor(String colorName, CRGB color){
  Serial.print(colorName);
  Serial.print(":");
  Serial.print (color.red);
  Serial.print(",");
  Serial.print (color.green);
  Serial.print(",");
  Serial.print (color.blue);
  Serial.println();  
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

//usb keyboard button press
void OnPress(uint8_t key)
{
  Serial.print("Key Pressed:  "); Serial.println(key, HEX);
}

//usb keyboard button release
void OnRelease(uint8_t key)
{
  if (key==0x1E) modePowerUp();
  else if (key==0x1F && mode==MODE_IDLE) modeAttack();
  else if (key==0x1F && (mode==MODE_OFF || mode == MODE_LOWRIDER)) modeLowRider();
  
  else if (key==0x20 && mode==MODE_IDLE) modeDamaged();
  else if (key==0x20 && (mode==MODE_OFF || mode == MODE_DRWHO)) modeDrWho();
  
  else if (key==0x21 && mode==MODE_IDLE) modeDestroyed();
  else if (key==0x21 && (mode==MODE_OFF || mode == MODE_VADER)) modeVader();
  
  else if (key==0x22) modeTheChain();
  else if (key==0x6) volumeUp();
  else if (key==0x7) volumeDown();
  else if (key==0xE9) volumeUp();
  else if (key==0xEA) volumeDown();
  else if (key==0xCD) toggleMute();
  

  
  if (debugOptions[DEBUG_INPUT]) {
    Serial.print("Key Released: "); 
    Serial.println(key, HEX);
  }
  
}

void modePowerUp() {
  if (mode > 0 && mode < 5) return;
  mode=MODE_POWER_UP;
  clearActions();
  musicLoop=true;
  //firstLoop=true;
  queueWAV(CHANNEL_SFX1, "ACGBOOT.WAV");
  queueWAV(CHANNEL_MUSIC, "ACGBATL1.WAV");
  musicLoopFilename = "ACGBATL2.WAV";

  queueAction(ACTION_FADE_IN_NECK, 50);
  queueAction(ACTION_FADE_IN_BODY, 2000);
  queueAction(ACTION_FADE_IN_EYE,  3000);
  
 
   /*
  eyeAniMode = 1;
  neckAniMode = 1;    //todo queue these to come up over time?
  bodyAniMode = 1;
  */
}

void modeAttack() {
  if (mode!=MODE_IDLE) return;
  mode=MODE_ATTACK;
  clearActions();
  eyeAniMode = 2;
  queueWAV(CHANNEL_SFX1, "ACGCHRG.WAV");
  actionQueue[ACTION_PLAY_LASER_SOUND]=millis() + 1700;
}

void modeDamaged() {
  if (mode!=MODE_IDLE) return;
  mode=MODE_DAMAGED;
  String fn = "ACGHIT";
  fn = fn + random(1, NUM_SFX_HIT) + ".WAV";
  queueWAV(CHANNEL_SFX1, fn); 
  clearActions();
  queueAction(ACTION_SET_MODE_IDLE,  2000); //this may change to an end of animation trigger 
                                            //instead of queued action once we have animation code for this
}

void modeDestroyed() {
  if (mode!=MODE_IDLE) return;
  mode=MODE_DESTROYED;
  clearActions();
  musicLoop = false;
  queueWAV(CHANNEL_MUSIC, "ACGBATL3.WAV");
  queueAction(ACTION_FADE_OUT_EYE,  4000);
  queueAction(ACTION_FADE_OUT_NECK, 2000);
  queueAction(ACTION_FADE_OUT_BODY, 3000);
  queueAction(ACTION_SET_MODE_OFF,  6000);  
}

void modeTheChain() {
 
  if (mode==MODE_THE_CHAIN) {
    //stop playback
    Serial.println("Break The Chain!");
    musicLoop = false;
    channels[CHANNEL_MUSIC]->stop();
    mode=MODE_OFF;
    neckAniMode = 0;
    bodyAniMode = 0;
    eyeAniMode = 0;
  }
  else {
    clearActions();
    musicLoop = true;
    musicLoopFilename = "CHAIN.WAV";
    mode=MODE_THE_CHAIN;
    Serial.println("The Chain!");
    neckAniMode = 1;
    eyeAniMode = 1;
    bodyAniMode = 2;
    queueWAV(CHANNEL_MUSIC, musicLoopFilename);
  }
}

void modeVader() {
   if (mode==MODE_VADER) {
    //stop playback
    Serial.println("Use the Force Luke!");
    musicLoop = false;
    channels[CHANNEL_MUSIC]->stop();
    mode=MODE_OFF;
    neckAniMode = 0;
    bodyAniMode = 0;
    eyeAniMode = 0;
  }
  else {
    clearActions();
  
    musicLoop = true;
    musicLoopFilename = "VADER.WAV";
    mode=MODE_VADER;
    Serial.println("Vader!");
    queueWAV(CHANNEL_MUSIC, musicLoopFilename);
  }
}

void modeDrWho() {
   if (mode==MODE_DRWHO) {
    //stop playback
    Serial.println("Bye Bye Dalek!");
    musicLoop = false;
    channels[CHANNEL_MUSIC]->stop();
    mode=MODE_OFF;
    neckAniMode = 0;
    bodyAniMode = 0;
    eyeAniMode = 0;
  }
  else {
    clearActions();
  
    musicLoop = true;
    musicLoopFilename = "DRWHO.WAV";
    mode=MODE_DRWHO;
    Serial.println("Exterminate!");
    queueWAV(CHANNEL_MUSIC, musicLoopFilename);
  }
}

void modeLowRider() {
   if (mode==MODE_LOWRIDER) {
    //stop playback
    Serial.println("Stop the ride!");
    musicLoop = false;
    channels[CHANNEL_MUSIC]->stop();
    mode=MODE_OFF;
    neckAniMode = 0;
    bodyAniMode = 0;
    eyeAniMode = 0;
  }
  
  else {
    clearActions();
    musicLoop = true;
    musicLoopFilename = "LOWRIDER.WAV";
    mode=MODE_LOWRIDER;
    Serial.println("Low Rider!");
    neckAniMode = 0;
    eyeAniMode = 0; //no leds for lowrider
    bodyAniMode = 0;
    queueWAV(CHANNEL_MUSIC, musicLoopFilename);
  }
}
void queueAction(uint8_t action, long time) {
  if (debugOptions[DEBUG_ACTION]) Serial.printf("Queuing action: %s for %d ms\n", actionsText[action], time);
  actionQueue[action]= millis() + time;
  
}


void toggleMute() {
  Serial.println("Mute!");
  //todo: code here!
  
}

/*
 * Audio Playback 
 * queueWAV() - this is needed because we can't start audio files
 *              during the USB interrupt that generates key messages
 *              We add the file to a channel queue (currently only one deep)
 *              and there is a queue metro that will check for it and play it
 * 
 */

void queueWAV (int channel, String fn) {
  if (channel < NUM_CHANNELS) {
    playQueue[channel] = fn;
  if (debugOptions[DEBUG_AUDIO]) Serial.printf ("queueWAV(%i, %s)\n", channel, fn.c_str());
  
  }
}

/*
 * Audio Playback
 * playWAV() - this plays a specific wav file from SD card
 *             DO NOT CALL THIS FROM WITHIN A USB EVENT HANDLER, 
 *             USE queueWAV instead!
 */

void playWAV (int channel, String fn) {

  if (debugOptions[DEBUG_AUDIO]) Serial.printf("playWAV(%i, %s)\n", channel, fn.c_str());

  channels[channel]->play(fn.c_str());


} //end playWAV

/*
 * volumeUp() - increase system volume
 * 
 */
void volumeUp() 
{
  mainVolume += VOLUME_INCREMENT;
  updateVolume();
}

/*
 * volumeDown() - decrease system volume
 * 
 */
void volumeDown() 
{
  mainVolume -= VOLUME_INCREMENT;
  updateVolume();
}

/*
 * audio
 * updateVolume() -  checks min / max vol levels and updates the audio system volume
 *  
 */
void updateVolume() {

  if (mainVolume < MIN_VOLUME) mainVolume = MIN_VOLUME;
  else if (mainVolume > MAX_VOLUME) mainVolume = MAX_VOLUME;
 
  sgtl5000_1.volume(mainVolume);
    if (debugOptions[DEBUG_AUDIO]) {
    Serial.print("System volume now: ");
    Serial.println(mainVolume);
  }
  
}

/*
 * brightnessUp() - increase global brightness
 * 
 */
void brightnessUp() 
{
  globalBrightness += BRIGHTNESS_INCREMENT;
  updateBrightness();
}

/*
 * brightnessDown() - decrease global brightness
 * 
 */
void brightnessDown() 
{
  globalBrightness -= BRIGHTNESS_INCREMENT;
  updateBrightness();
}

/*
 * updateBrightness() -  checks min / max vol brightness updates the global brightness
 *  
 */
void updateBrightness() {

  if (globalBrightness < MIN_BRIGHTNESS) globalBrightness = MIN_BRIGHTNESS;
  else if (globalBrightness > MAX_BRIGHTNESS) globalBrightness = MAX_BRIGHTNESS;
 
  LEDS.setBrightness(globalBrightness);
  //  if (debugOptions[DEBUG_AUDIO]) {
    Serial.print("Global brightness now: ");
    Serial.println(globalBrightness);
  //}
  
}

/*
 * updateOLED()
 * 
 */
void updateOLED() {

  if (debugOptions[DEBUG_ANIMATION_EYE] || debugOptions[DEBUG_ANIMATION_NECK] || debugOptions[DEBUG_ANIMATION_BODY]) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.print("MODE:");
    display.print(modeText[mode]);   
    display.setCursor(90,0);
    display.print("FPS:");
    display.print(FastLED.getFPS());
    display.setCursor(0,8);
    display.print("E");display.print(eyeAniMode);
    display.setCursor(0,16);
    display.print("N");display.print(neckAniMode);
    display.setCursor(0,24);
    display.print("B");display.print(bodyAniMode);
    
    if (debugOptions[DEBUG_ANIMATION_EYE]) {
      for (int l=0; l<NUM_EYE_LEDS; l++) {
         CRGB val = eyeLEDs[l];
         if (val.blue > 10 || val.red >10) {
          display.drawPixel(SCREEN_WIDTH-(l/2), 12, SSD1306_WHITE);     
         }
      }
    }

    if (debugOptions[DEBUG_ANIMATION_NECK]) {
      uint8_t xOffset=20;
      uint8_t yOffset=18;
      for (int r=0; r<NUM_NECK_RINGS; r++) {
        if (r==1) yOffset = yOffset + 2;
        for (int l=0; l<NUM_NECK_LEDS_PER_RING; l++) {
           CRGB val = neckLEDs[r*NUM_NECK_LEDS_PER_RING+l];
           if (val.blue > 10) {
            display.drawPixel(l+xOffset, r+yOffset, SSD1306_WHITE);     
           }
        }
      }
    }

    if (debugOptions[DEBUG_ANIMATION_BODY]) {
      uint8_t xOffset=20;
      uint8_t yOffset=26;
      uint8_t viewPort = SCREEN_WIDTH - xOffset;
      for (uint16_t l=0; l<NUM_BODY_LEDS; l++) {
        CRGB colorCheck = bodySpotColor;
        if (bodyLEDs[l] == colorCheck) {
          
          uint8_t x = xOffset +  l % viewPort;
          uint8_t y = yOffset + (l / viewPort) * 2; //space lines apart
          display.drawPixel( x , y , SSD1306_WHITE);
        }
      }
    }
   display.display();
  }

}

void clearActions() {
  if (debugOptions[DEBUG_ACTION]) Serial.println("Clearing action queue");
  
  for (uint8_t a = 0; a< NUM_ACTIONS; a++) {
    actionQueue[a] = 0;
  }

}
