#ifndef REFLOW_PROFILES_H_
#define REFLOW_PROFILES_H_

// Number of temperature settings in a reflow profile
#define NUMPROFILETEMPS (48)

#define YAXIS (57)
#define XAXIS (12)
#define PROFILE_NAME_LEN 19  // ***********************************
#define PROFILE_PARAM_COUNT 13  // 8 × 9-bit + 5 × 6-bit // *******

typedef struct {
    const char* name;
    const uint16_t temperatures[NUMPROFILETEMPS];
} profile;

typedef struct {
	const char* name;
	uint16_t temperatures[NUMPROFILETEMPS];
} ramprofile;

typedef struct {  // ************************
    char* name[PROFILE_NAME_LEN];  // *********************
    uint16_t params[PROFILE_PARAM_COUNT]; // **************
} paramProfile;  // *********************

typedef struct {  			// *********
	const char* formatstr;  // *********
	const uint8_t minval;   // *********
	const uint8_t maxval;   // *********
	const float multiplier; // *********
} paramMenuStruct;  	// *********

typedef enum { // ***
	PREHEAT_START_TEMP = 0,  // *************
	PREHEAT_START_TEMP_DWELL_TIME,  // ******
	PREHEAT_HEAT_RATE,  // ******************
	SOAK_START_TEMP,  	// ******************
	SOAK_END_TARGET_TEMP, // ****************
	SOAK_DWELL_TIME,  	// ******************
	SOAK_HEAT_RATE,  	// ******************
	REFLOW_SOLDER_MELT_TEMP,  // ************
	REFLOW_OVERSHOOT_MELT_TEMP_DELTA, // ****
	REFLOW_HEAT_RATE,  	// ******************
	REFLOW_DWELL_TIME_ABOVE_MELT_TEMP, // ***
	COOLDOWN_RATE,  	// ******************
	COOLDOWN_END_TEMP  	// ******************
	PARAM_COUNT // total = 13 // ************
}	profileParameters; 	//  *****************


typedef enum { // ********
    PACK_9BIT, // ********
    PACK_6BIT // *********
} PackSize; // ***********

const PackSize paramPacking[PARAM_COUNT] = { // ***********
    PACK_9BIT, // PREHEAT_START_TEMP // ********************
	PACK_9BIT, // PREHEAT_START_TEMP_DWELL_TIME // *********
	PACK_6BIT, // PREHEAT_HEAT_RATE  // ********************
	PACK_9BIT, // SOAK_START_TEMP // ***********************
    PACK_9BIT, // SOAK_END_TARGET_TEMP // ******************
	PACK_9BIT, // SOAK_DWELL_TIME // ***********************
	PACK_6BIT, // SOAK_HEAT_RATE // ************************
	PACK_9BIT, // REFLOW_SOLDER_MELT_TEMP // ***************
    PACK_6BIT, // REFLOW_OVERSHOOT_MELT_TEMP_DELTA // ******
	PACK_6BIT, // REFLOW_HEAT_RATE // **********************
	PACK_9BIT, // REFLOW_DWELL_TIME_ABOVE_MELT_TEMP // *****
	PACK_6BIT, // COOLDOWN_RATE // *************************
    PACK_9BIT  // COOLDOWN_END_TEMP // *********************
}; // ******************************************************


// Profile End Times  // ********************
struct {  // ********************************
    float       preheat;  // ****************
    float       soak;  // *******************
    float       reflow;  // *****************
    float       cooldown;  // ***************
} endTime;  // End Times  // ****************

void Reflow_LoadCustomProfiles(void);
void Reflow_ValidateNV(void);
void Reflow_PlotProfile(int highlight);

int Reflow_GetProfileIdx(void);
int Reflow_SelectProfileIdx(int idx);
int Reflow_SelectEEProfileIdx(int idx);

int Reflow_GetEEProfileIdx(void);
int Reflow_SaveEEProfile(void);
void Reflow_ListProfiles(void);
const char* Reflow_GetProfileName(void);
uint16_t Reflow_GetSetpointAtIdx(uint8_t idx);
void Reflow_SetSetpointAtIdx(uint8_t idx, uint16_t value);
void Reflow_DumpProfile(int profile);

void Reflow_calculateTempProfile(void); // ******************
void Reflow_readEEprofileParams(void); // ******************
void Reflow_writeEEprofileParams(void); // ******************
int Reflow_getNumParamProfiles(void); // *************
int Reflow_getNumParamMenuItems(void); // ******************
void Reflow_editParamMenu_decreaseValue(int item, int amount); // *************
void Reflow_editParamMenu_increaseValue(int item, int amount); // *************
int Reflow_editParamMenu_getRawValue(int item); // *************
void Reflow_editParamMenu_setRawValue(int item, int value); // *************
float Reflow_editParamMenu_getValue(int item); // *************
int Reflow_editParamMenu_snprintFormattedValue(char* buf, int n, int item); // *************
void pack_profile(const Profile *profile, uint8_t *buffer); // ****************************
void unpack_profile(const uint8_t *buffer, Profile *profile); // *************************


#endif /* REFLOW_PROFILES_H */