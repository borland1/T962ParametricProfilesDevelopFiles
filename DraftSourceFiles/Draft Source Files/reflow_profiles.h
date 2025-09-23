#ifndef REFLOW_PROFILES_H_
#define REFLOW_PROFILES_H_

// Number of temperature settings in a reflow profile
#define NUMPROFILETEMPS (48)

#define YAXIS (57)
#define XAXIS (12)

// Constants // ***********************
#define PROFILE_NAME_LEN 19		// ***********************************
#define PROFILE_EEPROM_SIZE 32	// Size of each packed profile in EEPROM // *************
#define NUM_PARAMETER_PROFILES  7  // Number of parameter-based profiles // ***************
// #define NUM_PARAMETER_PROFILES (sizeof(paramProfiles) / sizeof(paramProfiles[0])) // **what's wrong with this? *********************

// enum for Parameter indices // *************	
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
	COOLDOWN_END_TEMP,  	// ******************
	PROFILE_PARAM_COUNT // total = 13 // ************
}	profileParameters; 	//  *****************

typedef struct {
    const char* name;
    const uint16_t temperatures[NUMPROFILETEMPS];
} profile;

typedef struct {
	char name[PROFILE_NAME_LEN];  // ***************
	uint16_t temperatures[NUMPROFILETEMPS];
} ramprofile;

// Profile structure for parameter based profiles // *************
typedef struct {  // ************************
    char name[PROFILE_NAME_LEN];  // *********************
    uint16_t params[PROFILE_PARAM_COUNT]; // **************
} paramProfile;  // *********************

extern const paramProfile paramProfiles[]; // *********************

// Parameter menu structure // *************
typedef struct {  			// *********
	const char* formatstr;  // *********
	const profileParameters param_item; // *********
	const uint8_t minval;   // *********
	const uint16_t maxval;   // *********
	const float multiplier; // *********
} paramMenuStruct;  	// *********

// Profile End Times  // ********************
typedef struct {  // ********************************
    float       preheat;  // ****************
    float       soak;  // *******************
    float       reflow;  // *****************
    float       cooldown;  // ***************
} endTimes;  	// ****************

//Function declarations // ******************
void Reflow_LoadCustomProfiles(void);
void Reflow_ValidateNV(void);
void Reflow_PlotProfile(int highlight);

int Reflow_GetProfileIdx(void);
int Reflow_SelectProfileIdx(int idx);
int Reflow_SelectEEProfileIdx(int idx);

int Reflow_GetEEProfileIdx(void);
int Reflow_SaveEEProfile(void);
void Reflow_ListProfiles(void);
char* Reflow_GetProfileName(ramprofile *e3); // ***************
void Reflow_SetProfileName(char *buff, paramProfile *e4, ramprofile *e3); // ***************
uint16_t Reflow_GetSetpointAtIdx(uint8_t idx);
void Reflow_SetSetpointAtIdx(uint8_t idx, uint16_t value);
void Reflow_DumpProfile(int profile);

void Reflow_computeTempDatasetFromParams( paramProfile *e4, ramprofile *e3 ); // ******************
void Reflow_readEE_profileParams(paramProfile *e4, ramprofile *e3); // ******************
void Reflow_writeEE_profileParams(paramProfile *e4); // ******************
int Reflow_getNumParamProfiles(void); // *************
int Reflow_getNumParamMenuItems(void); // ******************
void Reflow_editParamMenu_decreaseValue(int item, int amount); // *************
void Reflow_editParamMenu_increaseValue(int item, int amount); // *************
int Reflow_editParamMenu_getRawValue(int item); // *************
void Reflow_editParamMenu_setRawValue(int item, int value); // *************
float Reflow_editParamMenu_getValue(int item); // *************
int Reflow_editParamMenu_snprintFormattedValue(char* buf, int n, int item); // *************
void Reflow_pack_profile(const paramProfile *p_profile, uint8_t *buffer); // ****************************
void Reflow_unpack_profile(const uint8_t *buffer, paramProfile *p_profile); // *************************

#endif /* REFLOW_PROFILES_H */