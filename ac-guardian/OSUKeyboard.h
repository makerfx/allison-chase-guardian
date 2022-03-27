#include <Arduino.h>
#include <USBHost_t36.h>

#define NUM_KEYS 5

class OSUKeyboard : public USBHIDInput {
public:
  OSUKeyboard(USBHost &host, uint32_t usage = 0) : fixed_usage_(usage) { init(); }
  uint32_t usage(void) {return usage_;}
  void     attachRawPress(void (*f)(uint8_t keycode)) { rawKeyPressedFunction = f; }
  void     attachRawRelease(void (*f)(uint8_t keycode)) { rawKeyReleasedFunction = f; }
  
protected:
  virtual hidclaim_t claim_collection(USBHIDParser *driver, Device_t *dev, uint32_t topusage);
  virtual bool hid_process_in_data(const Transfer_t *transfer);
  virtual bool hid_process_out_data(const Transfer_t *transfer);
  virtual void hid_input_begin(uint32_t topusage, uint32_t type, int lgmin, int lgmax);
  virtual void hid_input_data(uint32_t usage, int32_t value);
  virtual void hid_input_end();
  virtual void disconnect_collection(Device_t *dev);
private:
  void init();
  USBHIDParser *driver_;
  uint8_t collections_claimed = 0;
  volatile int hid_input_begin_level_ = 0;
  uint32_t fixed_usage_;
  uint32_t usage_ = 0;
  // Track changing fields. 
  const static int MAX_CHANGE_TRACKED = 512;
  uint32_t usages_[MAX_CHANGE_TRACKED];
  int32_t values_[MAX_CHANGE_TRACKED];
  int count_usages_ = 0;
  int index_usages_ = 0;

  bool keys[NUM_KEYS];
  
  void (*rawKeyPressedFunction)(uint8_t keycode) = nullptr;
  void (*rawKeyReleasedFunction)(uint8_t keycode) = nullptr;
  
  // See if we can contribute transfers
  Transfer_t mytransfers[2] __attribute__ ((aligned(32)));
};
