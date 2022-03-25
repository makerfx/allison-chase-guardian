/* Based on HIDDumpController
 *  
 *  USB EHCI Host for Teensy 3.6
   Copyright 2017 Paul Stoffregen (paul@pjrc.com)

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the
   "Software"), to deal in the Software without restriction, including
   without limitation the rights to use, copy, modify, merge, publish,
   distribute, shiublicense, and/or sell copies of the Software, and to
   permit persons to whom the Software is furnished to do so, subject to
   the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
   CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
   TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
   SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/
#include "OSUKeyboard.h"



void OSUKeyboard::init()
{
  Serial.println("init!");
  USBHost::contribute_Transfers(mytransfers, sizeof(mytransfers) / sizeof(Transfer_t));
  USBHIDParser::driver_ready_for_hid_collection(this);
}

hidclaim_t OSUKeyboard::claim_collection(USBHIDParser *driver, Device_t *dev, uint32_t topusage)
{
  // only claim RAWHID devices currently: 16c0:0486
  Serial.printf("OSUKeyboard Claim: %x:%x usage: %x", dev->idVendor, dev->idProduct, topusage);
  if (mydevice != NULL && dev != mydevice) {
    Serial.println("- NO (Device)");
    return CLAIM_NO;
  }
  if (usage_ && (usage_ != topusage)) {
    Serial.printf(" - NO (Usage: %x)\n");
    return CLAIM_NO;      // Only claim one
  }
  mydevice = dev;
  collections_claimed++;
  usage_ = topusage;
  driver_ = driver; // remember the driver.
  Serial.println(" - Yes");
  return CLAIM_INTERFACE;  // We want
}

void OSUKeyboard::disconnect_collection(Device_t *dev)
{
  if (--collections_claimed == 0) {
    mydevice = NULL;
    usage_ = 0;
  }
}


void dump_hexbytes(const void *ptr, uint32_t len)
{
  if (ptr == NULL || len == 0) return;
  uint32_t count = 0;
//  if (len > 64) len = 64; // don't go off deep end...
  const uint8_t *p = (const uint8_t *)ptr;
  while (len--) {
    if (*p < 16) Serial.print('0');
    Serial.print(*p++, HEX);
    count++;
    if (((count & 0x1f) == 0) && len) Serial.print("\n");
    else Serial.print(' ');
  } 
  Serial.println();
}


bool OSUKeyboard::hid_process_in_data(const Transfer_t *transfer)
{

  //this code is specific for the Sayobot 5 key mechanical keyboard 
  //teensy doesn't see it as a keyboard, but the raw HID data is there
  
  const uint8_t *p = (const uint8_t *)transfer->buffer;

  //todo, set internal variables to record keydown / then up so that we don't duplicate report.
  bool tempKeys[NUM_KEYS]; 
  
  //init tempKeys
  for (int i=0; i<NUM_KEYS; i++){
    tempKeys[i] = 0;
  }

  //look at data buffer to determine which key states
  for (int i=0; i<NUM_KEYS; i++){
    uint8_t k = *(p+i+3);   //the data packet keys start in 3 positions

    uint8_t keybase = 0x1E;
    for (int i=0; i<NUM_KEYS; i++){
      if (k == keybase + i) tempKeys[i] = true;
    }
  }  

  //if the key changed, call the function
  for (int i=0; i<NUM_KEYS; i++){
    if (tempKeys[i] != keys[i]) {
      if (keys[i] == 0) {
         if (rawKeyPressedFunction) rawKeyPressedFunction(i+1);        
      }
      else if (keys[i] == 1) {
        if (rawKeyReleasedFunction) rawKeyReleasedFunction(i+1);  
      }
    }
    keys[i] = tempKeys[i];
  }
  
  return 0;
}

bool OSUKeyboard::hid_process_out_data(const Transfer_t *transfer)
{
  return true;
}

void indent_level(int level) {
  if ((level > 5) || (level < 0)) return; // bail if something is off...
  while (level--) Serial.print("  ");
}

void OSUKeyboard::hid_input_begin(uint32_t topusage, uint32_t type, int lgmin, int lgmax){}

void OSUKeyboard::hid_input_data(uint32_t usage, int32_t value) {}

void OSUKeyboard::hid_input_end() {}
