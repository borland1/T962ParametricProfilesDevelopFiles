#ifndef REFLOW_PROFILES_H_
#define REFLOW_PROFILES_H_

// Number of temperature settings in a reflow profile
#define NUMPROFILETEMPS (48)

#define YAXIS (57)
#define XAXIS (12)

typedef struct {
    const char* name;
    const uint16_t temperatures[NUMPROFILETEMPS];
} profile;

typedef struct {
	const char* name;
	uint16_t temperatures[NUMPROFILETEMPS];
} ramprofile;

typedef struct {  // ********************************
    char* name[19];  // ********************************
    uint16_t parameter[13]; // ******************
} parameterProfile;  // ********************************

typedef struct {  // ********
	const char* formatstr;  // *********
	const uint8_t minval;   // *********
	const uint8_t maxval;   // *********
	const float multiplier; // *********
} parameterMenuStruct;  // *********

typedef enum edit_profile_items { // ********
	PREHEAT_START_TEMP,  // *****************
	PREHEAT_START_TEMP_DWELL_TIME,  // ******
	PREHEAT_HEAT_RATE,  // ******************
	SOAK_START_TEMP,  // ********************
	SOAK_END_TARGET_TEMP, // ***************
	SOAK_DWELL_TIME,  // ********************
	SOAK_HEAT_RATE,  // *********************
	REFLOW_SOLDER_MELT_TEMP,  // ************
	REFLOW_OVERSHOOT_MELT_TEMP_DELTA, // ******
	REFLOW_HEAT_RATE,  // ********************
	REFLOW_DWELL_TIME_ABOVE_MELT_TEMP, // *****
	COOLDOWN_RATE,  // ************************
	COOLDOWN_END_TEMP  // *********************
}	parameterMenuItem;  // *********************

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
void Reflow_readEEprofileParameters(void); // ******************
void Reflow_writeEEprofileParameters(void); // ******************
int Reflow_getNumParameterProfiles(void); // *************
int Reflow_getNumParamMenuItems(void); // ******************
void Reflow_editParamMenu_decreaseValue(int item, int amount); // *************
void Reflow_editParamMenu_increaseValue(int item, int amount); // *************
int Reflow_editParamMenu_getRawValue(int item); // *************
void Reflow_editParamMenu_setRawValue(int item, int value); // *************
float Reflow_editParamMenu_getValue(int item); // *************
int Reflow_editParamMenu_snprintFormattedValue(char* buf, int n, int item); // *************

#endif /* REFLOW_PROFILES_H */