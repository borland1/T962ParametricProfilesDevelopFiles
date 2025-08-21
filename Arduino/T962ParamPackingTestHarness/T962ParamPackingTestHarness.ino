// Arduino Sketch: Test Harness

#include <stdio.h>

#define PROFILE_NAME_LEN 19

typedef enum {
  PREHEAT_START_TEMP = 0,
  PREHEAT_START_TEMP_DWELL_TIME,
  PREHEAT_HEAT_RATE,          // 6-bit
  SOAK_START_TEMP,
  SOAK_END_TARGET_TEMP,
  SOAK_DWELL_TIME,
  SOAK_HEAT_RATE,           // 6-bit
  REFLOW_SOLDER_MELT_TEMP,
  REFLOW_OVERSHOOT_MELT_TEMP_DELTA, // 6-bit
  REFLOW_HEAT_RATE,         //  6-bit
  REFLOW_DWELL_TIME_ABOVE_MELT_TEMP,
  COOLDOWN_RATE,            // 6-bit
  COOLDOWN_END_TEMP,
  PROFILE_PARAM_COUNT  // 13 Total 8 9-bit, 5 6-bit
} ProfileParam;

typedef enum {
  PARAM_TYPE_9BIT,
  PARAM_TYPE_6BIT
} ParamType;

typedef struct {
  char name[PROFILE_NAME_LEN];       // 152 bits
  uint16_t params[PROFILE_PARAM_COUNT]; // All parameters stored as 16-bit
} Profile;

const ParamType param_types[PROFILE_PARAM_COUNT] = {
  PARAM_TYPE_9BIT, 
  PARAM_TYPE_9BIT,
  PARAM_TYPE_6BIT, // PREHEAT_HEAT_RATE
  PARAM_TYPE_9BIT,
  PARAM_TYPE_9BIT,
  PARAM_TYPE_9BIT,
  PARAM_TYPE_6BIT, // SOAK_HEAT_RATE
  PARAM_TYPE_9BIT,
  PARAM_TYPE_6BIT, // REFLOW_OVERSHOOT_MELT_TEMP_DELTA
  PARAM_TYPE_6BIT, // REFLOW_HEAT_RATE
  PARAM_TYPE_9BIT,
  PARAM_TYPE_6BIT, // COOLDOWN_RATE
  PARAM_TYPE_9BIT
};

// Function Prototypes  
void pack_profile(const Profile *profile, uint8_t *buffer);
void unpack_profile(const uint8_t *buffer, Profile *profile);
void test_pack_unpack();

uint8_t buffer[32] = {0};
Profile recovered;

Profile original = {
  "TestProfile01",
  {50, 20, 15, 140, 183, 140, 3, 183, 33, 13, 60, 15, 50}
};

void setup() {
  Serial.begin(115200); // Any baud rate should work
  Serial.println("Hello Arduino");
  test_pack_unpack();
}

void loop() {

}

void test_pack_unpack() {
  pack_profile(&original, buffer);
  unpack_profile(buffer, &recovered);
  // Compare results
  char buffer;
  sprintf(buffer, "Name: %s", recovered.name);
  Serial.println(buffer);
  for (int i = 0; i < PROFILE_PARAM_COUNT; i++) {
    sprintf(buffer,"Param %d (%s): %u\n", i, 
      ( param_types[i] == PARAM_TYPE_9BIT ) ? "9-bit" : "6-bit",
      recovered.params[i]);
      Serial.print(buffer);
  }
}

void pack_profile(const Profile *profile, uint8_t *buffer) {
  uint16_t bit_cursor = 0;

  // Copy name
  memcpy(buffer, profile->name, PROFILE_NAME_LEN);
  bit_cursor = PROFILE_NAME_LEN * 8;
  
  // Pack parameters
  for (int i = 0; i < PROFILE_PARAM_COUNT; i++) {
    uint16_t val = profile->params[i];
    uint8_t bit_len = (param_types[i] == PARAM_TYPE_9BIT) ? 9 : 6;

    for (int b = 0; b < bit_len; b++) {
      uint16_t bit_pos = bit_cursor + b;
      buffer[bit_pos / 8] |= ((val >> b) & 0x01) << (bit_pos % 8);
    }
    bit_cursor += bit_len;
  }
}

void unpack_profile(const uint8_t *buffer, Profile *profile) {
  uint16_t bit_cursor = 0;

  // Copy name
  memcpy(profile->name, buffer, PROFILE_NAME_LEN);
  bit_cursor = PROFILE_NAME_LEN * 8;

  // Unpack parameters
  for (int i = 0; i < PROFILE_PARAM_COUNT; i++) {
    uint16_t val = 0;
    uint8_t bit_len = (param_types[i] == PARAM_TYPE_9BIT) ? 9 : 6;
    for (int b = 0; b < bit_len; b++) {
      uint16_t bit_pos = bit_cursor + b;
      val |= ((buffer[bit_pos / 8] >> (bit_pos % 8)) & 0x01) << b;
        }
    profile->params[i] = val;
        bit_cursor += bit_len;
  }
}
