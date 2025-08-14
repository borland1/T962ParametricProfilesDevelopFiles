#include "LPC214x.h"
#include <stdint.h>
#include <stdio.h>
#include "t962.h"
#include "lcd.h"
#include "nvstorage.h"
#include "eeprom.h"
#include "reflow.h"
#include "reflow_profiles.h"
#include <string.h> // **********************

#define RAMPTEST
#define PIDTEST

extern uint8_t graphbmp[];

// Amtech 4300 63Sn/37Pb leaded profile
static const profile am4300profile = {
	"4300 63SN/37PB", {
		 50, 50, 50, 60, 73, 86,100,113,126,140,143,147,150,154,157,161, // 0-150s
		164,168,171,175,179,183,195,207,215,207,195,183,168,154,140,125, // Adjust peak from 205 to 220C
		111, 97, 82, 68, 54,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};

// NC-31 low-temp lead-free profile
static const profile nc31profile = {
	"NC-31 LOW-TEMP LF", {
		 50, 50, 50, 50, 55, 70, 85, 90, 95,100,102,105,107,110,112,115, // 0-150s
		117,120,122,127,132,138,148,158,160,158,148,138,130,122,114,106, // Adjust peak from 158 to 165C
		 98, 90, 82, 74, 66, 58,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};

// SynTECH-LF normal temp lead-free profile
static const profile syntechlfprofile = {
	"AMTECH SYNTECH-LF", {
		 50, 50, 50, 50, 60, 70, 80, 90,100,110,120,130,140,149,158,166, // 0-150s
		175,184,193,201,210,219,230,240,245,240,230,219,212,205,198,191, // Adjust peak from 230 to 249C
		184,177,157,137,117, 97, 77, 57,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};

#ifdef RAMPTEST
// Ramp speed test temp profile
static const profile rampspeed_testprofile = {
	"RAMP SPEED TEST", {
		 50, 50, 50, 50,245,245,245,245,245,245,245,245,245,245,245,245, // 0-150s
		245,245,245,245,245,245,245,245,245, 50, 50, 50, 50, 50, 50, 50, // 160-310s
		 50, 50, 50, 50, 50, 50, 50, 50,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};
#endif

#ifdef PIDTEST
// PID gain adjustment test profile (5% setpoint change)
static const profile pidcontrol_testprofile = {
	"PID CONTROL TEST",	{
		171,171,171,171,171,171,171,171,171,171,171,171,171,171,171,171, // 0-150s
		180,180,180,180,180,180,180,180,171,171,171,171,171,171,171,171, // 160-310s
		  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0  // 320-470s
	}
};
#endif

const parameterMenuStruct parameterMenu[] = {  // *************************
                // {formatstr, enum item, min val, max val, multiplier} // *************************
// Preheat // *************************
    {"START TEMP(~) %3.0f", PREHEAT_START_TEMP, 0, 350, 1.0f}, // *************************
	{"DWELL TIME(SEC) %3.0f", PREHEAT_START_TEMP_DWELL_TIME, 0, 480, 1.0f}, // *************************
	{"HEAT RATE(~/SEC)  %1.1f", PREHEAT_HEAT_RATE, 0, 25, 0.1f}, // *************************
// Soak // *************************
    {"START TEMP(~) %3.0f", SOAK_START_TEMP, 0, 350, 1.0f}, // *************************
	{"TARGET TEMP(~) %3.0f", SOAK_END_TARGET_TEMP, 0, 350, 1.0f}, // *************************
	{"DWELL TIME(SEC) %3.0f", SOAK_DWELL_TIME, 0, 250, 1.0f}, // *************************
    {"HEAT RATE(~/SEC) %1.1f", SOAK_HEAT_RATE, 0, 25, 0.1f}, // *************************
// Reflow	 // *************************
    {"SOLDER MELT TEMP(~)%3.0f", REFLOW_SOLDER_MELT_TEMP, 0, 350, 1.0f}, // *************************
	{"SOLDER MELT OVERSHOOT\n    DELTA(~) %2.0f", REFLOW_OVERSHOOT_MELT_TEMP_DELTA, 0, 250, 1.0f}, // *************************
	{"HEAT RATE(~/SEC) %1.1f", REFLOW_HEAT_RATE, 0, 25, 0.1f}, // *************************
	{"DWELL TIME ABOVE\n    MELT TEMP(SEC) %3.0f", REFLOW_DWELL_TIME_ABOVE_MELT_TEMP, 0, 250, 1.0f}, // *************************
// Cooldown // *************************
	{"COOLDOWN RATE(~/SEC) %1.1f", COOLDOWN_RATE, 0, 25, 0.1f},  // *************************
    {"END TEMP(~) %3.0f", COOLDOWN_END_TEMP, 0, 350, 1.0f}, // *************************
}; // *************************

#define NUM_PARAMETER_MENU_ITEMS (sizeof(parametermenu) / sizeof(parametermenu[0])) // *************************

// Amtech 4300 63Sn/37Pb leaded profile  // **********
static const am4300_paramProfile = {  // *********************
	"4300 63SN/37PB", {  	// Profile Name  // *****************
    	50,                 	// Preheat Start Temp  // *************
    	20,                 	// Preheat Start Temp Dwell Time (sec)  // ***********
    	15,  // 1.5 after multiplier  // Preheat Heat Rate (deg C/sec)  // ***********
    	140,                	// Soak Start Temp (deg C)  // *******************
    	183,                	// Soak End Target Temp (deg C)  // *************
    	140,                	// Soak Dwell Time (sec)  // ************
    	3,  // 0.3 after multiplier // Soak Heat Rate (deg C/sec)  // ***************
    	183,                	// Reflow Solder Melt Temp (deg C)  // *************
    	33,                 	// Reflow Overshoot Melt Temp Delta (deg C)  // ***********
    	13, // 1.3 after multiplier // Reflow Heat Rate (deg C/sec)  // *************
    	60,                 	// Reflow Dwell Time Above Melt Temp (sec)  // ***********
    	15, // 1.5 after multiplier // Cooldown Rate (deg C/sec)  // ***********
    	50						// Cooldown End Temp (deg C)  // ***********
	}   // ******
};  // **************

// NC-31 low-temp lead-free profile  // ***************
static const nc31_paramProfile = {  // *************
	"NC-31 LOW-TEMP LF", {   // Profile Name  // **************
    	50,                     // Preheat Start Temp  // *************
    	30,                     // Preheat Start Temp Dwell Time (sec) // ************ 
    	16, // 1.6 after multiplier // Preheat Heat Rate (deg C/sec)  // *************
    	85,                     // Soak Start Temp (deg C)  // ***************
    	127,                    // Soak End Target Temp (deg C)  // *************
    	140,                    // Soak Dwell Time (sec)  // *************
    	3,  // 0.3 after multiplier // Soak Heat Rate (deg C/sec)  // *************
    	138,                    // Reflow Solder Melt Temp (deg C)  // **************
    	21,                     // Reflow Overshoot Melt Temp Delta (deg C)  // ************
    	13, // 1.3 after multiplier // Reflow Heat Rate (deg C/sec)  // **********
    	60,                     // Reflow Dwell Time Above Melt Temp (sec)  // ***********
    	8, // 0.8 after multiplier  // Cooldown Rate (deg C/sec)  // ***********
    	50						// Cooldown End Temp (deg C)  // ************
	}   // *******
};  // ***************

// SynTECH-LF normal temp lead-free profile  // *************
static const syntechlf_paramProfile = {  // **************
	"AMTECH SYNTECH-LF", {   // Profile Name  // ********
    	50,                     // Preheat Start Temp  // *****************
    	30,                     // Preheat Start Temp Dwell Time (sec)  // **************
    	10, // 1.0 after multiplier // Preheat Heat Rate (deg C/sec)  // *************
    	85,                     // Soak Start Temp (deg C)  // ***************
    	220,                    // Soak End Target Temp (deg C)  // ***************
    	140,                    // Soak Dwell Time (sec)  // ***************
    	9,  // 0.9 after multiplier // Soak Heat Rate (deg C/sec)  // **********
    	220,                    // Reflow Solder Melt Temp (deg C)  // *************
    	25,                     // Reflow Overshoot Melt Temp Delta (deg C)  // **************
    	13, // 1.3 after multiplier // Reflow Heat Rate (deg C/sec)  // ***************
    	60,                     // Reflow Dwell Time Above Melt Temp (sec)  // ***********
    	8, // 0.8 after multiplier// Cooldown Rate (deg C/sec)  // ************
    	50						// Cooldown End Temp (deg C)  // **********
	}   // *******
};  // ************

// Ramp speed test temp profile  // *************
static const rampspeed_test_paramProfile = {  // ************
	"RAMP SPEED TEST", { 	// Profile Name  // ************
    	50,                 	// Preheat Start Temp  // **********
    	30,                 	// Preheat Start Temp Dwell Time (sec)  // **********
    	250, // 25 after multiplier // Preheat Heat Rate (deg C/sec)  // ***********
    	245,                	// Soak Start Temp (deg C)  // *************
    	245,                	// Soak End Target Temp (deg C)  // ***********
    	210,                	// Soak Dwell Time (sec)  // *************
    	250, // 25 after multiplier	// Soak Heat Rate (deg C/sec)  // ************
    	50,                 	// Reflow Solder Melt Temp (deg C)  // *************
    	0,                  	// Reflow Overshoot Melt Temp Delta (deg C)  // ************
    	250, // 25 after multiplier	// Reflow Heat Rate (deg C/sec)  // ************
    	150,                	// Reflow Dwell Time Above Melt Temp (sec)  // ****************
    	250, // 25 after multiplier	// Cooldown Rate (deg C/sec)  // *****************
    	50                  	// Cooldown End Temp (deg C)  // **************
	}   // **********
};  // ***************

// PID gain adjustment test profile (5% setpoint change)  // ***********
static const pid_control_test_paramProfile = {  // ******************
	"PID CONTROL TEST", {	// Profile Name  // ***********
    	171,                 	// Preheat Start Temp  // *************
    	150,                 	// Preheat Start Temp Dwell Time (sec)  // *************
    	90, // 9 after multiplier	// Preheat Heat Rate (deg C/sec)  // ************
    	180,                	// Soak Start Temp (deg C)  // **************
    	180,                	// Soak End Target Temp (deg C)  // **********
    	80,                		// Soak Dwell Time (sec)  // ***************
    	90,  // 9 after multiplier	// Soak Heat Rate (deg C/sec)  // ***************
    	171,                	// Reflow Solder Melt Temp (deg C)  // *************
    	0,                 		// Reflow Overshoot Melt Temp Delta (deg C)  // **************
    	90, // 9 after multiplier	// Reflow Heat Rate (deg C/sec)  // ************ 
    	80,                 	// Reflow Dwell Time Above Melt Temp (sec)  // ************
    	90, // 9 after multiplier	// Cooldown Rate (deg C/sec)  // ************
    	171                  	// Cooldown End Temp (deg C)  // *************
	}   // *******
};  // **********

// PID gain adjustment test profile (5% setpoint change)  // *************
static const custom1_paramProfile = {  // ***************
	"CUSTOM #1", {	// Profile Name  // *************
    	0,                 	// Preheat Start Temp  // ****************
    	0,                 	// Preheat Start Temp Dwell Time (sec)  // *************
    	1, // 0.1 after multiplier	// Preheat Heat Rate (deg C/sec)  // **********
    	0,                	// Soak Start Temp (deg C)  // ****************
    	0,                	// Soak End Target Temp (deg C)  // **************
    	0,                		// Soak Dwell Time (sec)  // ***************
    	1,  // 0.1 after multiplier	// Soak Heat Rate (deg C/sec)  // **************
    	0,                	// Reflow Solder Melt Temp (deg C)  // **********
    	0,                 		// Reflow Overshoot Melt Temp Delta (deg C)  // ***************
    	1, // 0.1 after multiplier	// Reflow Heat Rate (deg C/sec)  // ************ 
    	0,                 	// Reflow Dwell Time Above Melt Temp (sec)  // *************
    	1, // 0.1 after multiplier	// Cooldown Rate (deg C/sec)  // **********
    	0                  	// Cooldown End Temp (deg C)  // ***********
	}   // *******
};  // **********

// PID gain adjustment test profile (5% setpoint change)  // ************
static const custom2_paramProfile = {  // *****************
	"CUSTOM #2", {	// Profile Name  // *********************
    	0,                 	// Preheat Start Temp  // *******************
    	0,                 	// Preheat Start Temp Dwell Time (sec)  // ************
    	1, // 0.1 after multiplier	// Preheat Heat Rate (deg C/sec)  // ***************
    	0,                	// Soak Start Temp (deg C)  // *********************
    	0,                	// Soak End Target Temp (deg C)  // ****************
    	0,                		// Soak Dwell Time (sec)  // ****************
    	1,  // 0.1 after multiplier	// Soak Heat Rate (deg C/sec)  // ***********
    	0,                	// Reflow Solder Melt Temp (deg C)  // ************
    	0,                 		// Reflow Overshoot Melt Temp Delta (deg C)  // ***********
    	1, // 0.1 after multiplier	// Reflow Heat Rate (deg C/sec)  // *********** 
    	0,                 	// Reflow Dwell Time Above Melt Temp (sec)  // ***************
    	1, // 0.1 after multiplier	// Cooldown Rate (deg C/sec)  // **************
    	0                  	// Cooldown End Temp (deg C)  // ***************
	}   // *******
};  // ***************

// EEPROM profile 1
static ramprofile ee1 = { "CUSTOM #1" };

// EEPROM profile 2
static ramprofile ee2 = { "CUSTOM #2" };

// RAM temperature dataset profile 3  // *****************
static ramprofile ee3; // *****************

// RAM parameter profile 4 // ********************
static parameterProfile ee4;  // ***************

static const profile* profiles[] = {
	&syntechlfprofile,
	&nc31profile,
	&am4300profile,
#ifdef RAMPTEST
	&rampspeed_testprofile,
#endif
#ifdef PIDTEST
	&pidcontrol_testprofile,
#endif
    (profile*) &ee1,
    (profile*) &ee2
};

#define NUMPROFILES (sizeof(profiles) / sizeof(profiles[0]))

static const parameterProfile* parameterProfiles[] = { // ***********
	&syntechlf_paramProfile, // ******************
	&nc31_paramProfile, // *****************
	&am4300_paramProfile, // *************
#ifdef RAMPTEST // *************
	&rampspeed_test_paramProfile, // *************
#endif // ****************
#ifdef PIDTEST // ****************
	&pid_control_test_paramProfile, // **************
#endif // ****************
    &custom1_paramProfile, // *****************
    &custom2_paramProfile // ************
}; // ***********************

#define NUM_PARAMETER_PROFILES (sizeof(parameterProfiles) / sizeof(parameterProfiles[0])) // ***********************

// current profile index
static uint8_t profileidx = 0;

static void ByteswapTempProfile(uint16_t* buf) {
	for (int i = 0; i < NUMPROFILETEMPS; i++) {
		uint16_t word = buf[i];
		buf[i] = word >> 8 | word << 8;
	}
}

void Reflow_LoadCustomProfiles(void) {
	EEPROM_Read((uint8_t*)ee1.temperatures, 2, 96);
	ByteswapTempProfile(ee1.temperatures);
	
	EEPROM_Read((uint8_t*)ee2, addr, sizeof(ee2));
	ByteswapTempProfile(ee2.temperatures); 
}

void Reflow_ValidateNV(void) {
	if (NV_GetConfig(REFLOW_BEEP_DONE_LEN) == 255) {
		// Default 1 second beep length
		NV_SetConfig(REFLOW_BEEP_DONE_LEN, 10);
	}

	if (NV_GetConfig(REFLOW_MIN_FAN_SPEED) == 255) {
		// Default fan speed is now 8
		NV_SetConfig(REFLOW_MIN_FAN_SPEED, 8);
	}

	if (NV_GetConfig(REFLOW_BAKE_SETPOINT_H) == 255 || NV_GetConfig(REFLOW_BAKE_SETPOINT_L) == 255) {
		NV_SetConfig(REFLOW_BAKE_SETPOINT_H, SETPOINT_DEFAULT >> 8);
		NV_SetConfig(REFLOW_BAKE_SETPOINT_L, (uint8_t)SETPOINT_DEFAULT);
		printf("Resetting bake setpoint to default.");
	}

	Reflow_SelectProfileIdx(NV_GetConfig(REFLOW_PROFILE));
}

int Reflow_GetProfileIdx(void) {
	return profileidx;
}

int Reflow_SelectProfileIdx(int idx) {
	if (idx < 0) {
		profileidx = (NUMPROFILES - 1);
	} else if(idx >= NUMPROFILES) {
		profileidx = 0;
	} else {
		profileidx = idx;
	}
	NV_SetConfig(REFLOW_PROFILE, profileidx);
	return profileidx;
}

int Reflow_SelectEEProfileIdx(int idx) {
	if (idx == 1) {
		profileidx = (NUMPROFILES - 2);
	} else if (idx == 2) {
		profileidx = (NUMPROFILES - 1);
	}
	return profileidx;
}

int Reflow_GetEEProfileIdx(void) {
	if (profileidx == (NUMPROFILES - 2)) {
		return 1;
	} else 	if (profileidx == (NUMPROFILES - 1)) {
		return 2;
	} else {
		return 0;
	}
}

int Reflow_SaveEEProfile(void) {
		int retval = 0;
		uint8_t offset;
		uint16_t* tempptr;
		if (profileidx == (NUMPROFILES - 2)) {
		    	offset = 0;
		    	tempptr = ee1.temperatures;
		} else if (profileidx == (NUMPROFILES - 1)) {
	        	offset = 128;
		    	tempptr = ee2.temperatures;
		} else {
				return -1;
	}
	offset += 2; // Skip "magic"
	ByteswapTempProfile(tempptr);

	// Store profile
	retval = EEPROM_Write(offset, (uint8_t*)tempptr, 96);
	ByteswapTempProfile(tempptr);
	return retval;
}

void Reflow_ListProfiles(void) {
	for (int i = 0; i < NUMPROFILES; i++) {
			printf("%d: %s\n", i, profiles[i]->name);
	}
}

char* Reflow_GetProfileName(void) {
	return profiles[profileidx]->name;
}

uint16_t Reflow_GetSetpointAtIdx(uint8_t idx) {
	if (idx > (NUMPROFILETEMPS - 1)) {
		return 0;
	}
	// return profiles[profileidx]->temperatures[idx];  // *******************
	return ee3.temperatures[idx]; // RAM-based temps  // ******************
}

void Reflow_SetSetpointAtIdx(uint8_t idx, uint16_t value) {
		if (idx > (NUMPROFILETEMPS - 1)) { return; }
		if (value > SETPOINT_MAX) { return; }

		uint16_t temp = (uint16_t) &profiles[profileidx]->temperatures[idx];
		if (temp >= (uint16_t) 0x40000000) {
				*temp = value; // If RAM-based
	}
}

void Reflow_PlotProfile(int highlight) {
	LCD_BMPDisplay(graphbmp, 0, 0);

	// No need to plot first value as it is obscured by Y-axis
	for(int x = 1; x < NUMPROFILETEMPS; x++) {
			int realx = (x << 1) + XAXIS;
			int y = ee1.temperatures[x] / 5;
			y = YAXIS - y;
			LCD_SetPixel(realx, y);

			if (highlight == x) {
					LCD_SetPixel(realx - 1, y - 1);
					LCD_SetPixel(realx + 1, y + 1);
					LCD_SetPixel(realx - 1, y + 1);
					LCD_SetPixel(realx + 1, y - 1);
			}
	}

void Reflow_DumpProfile(int profile) {
		if (profile > NUMPROFILES) {
				printf("\nNo profile with id: %d\n", profile);
				return;
		}

		int current = profileidx;
		profileidx = profile;

		for (int i = 0; i < NUMPROFILETEMPS; i++) {
				printf("%4d,", Reflow_GetSetpointAtIdx(i));
				if (i == 15 || i == 31) {
					printf("\n ");
				}
		}
		printf("\n");
		profileidx = current;
}

// Calculate time/temperature profile from profile parameters // ******************
void Reflow_calculateTempProfile( void ) {  // ******************
	// unsigned int  // ******************
	uint16_t temp;  // profile incremental temperatures  // ******************
	float soakEndTemp = 0;  // ******************
	const int16_t inv = 480 / NUMPROFILETEMPS; // time intervals, based on 480 Second Heat Cycle (typ. 10 sec intervals) // ******************
  
	// Soak End Temp ... used in Reflow ET calc below  // ******************
	soakEndTemp = (ee4->parameter[SOAK_START_TEMP]) + (ee4->parameter[SOAK_HEAT_RATE] * parameterMenu[SOAK_HEAT_RATE].multiplier * ee4->parameter[SOAK_DWELL_TIME] );  // ******************
	if ( soakEndTemp > ee4->parameter[SOAK_END_TARGET_TEMP] ){  // ******************
		soakEndTemp = ee4->parameter[SOAK_END_TARGET_TEMP];  // ******************
	}
	// Calculate Profile Phase End Times ( ETs )  // ******************
	// Preheat  // ******************
	endTime.preheat = ( ee4->parameter[SOAK_START_TEMP] - ee4->parameter[PREHEAT_START_TEMP] ) /  // ******************
		( ee4->parameter[PREHEAT_HEAT_RATE] * parameterMenu[PREHEAT_HEAT_RATE].multiplier ) + ee4->parameter[PREHEAT_START_TEMP_DWELL_TIME];  // ***** can't divide by zero if heat rate = zero  // ******************
	// Soak  // ******************
	endTime.soak = endTime.preheat + ee4->parameter[SOAK_DWELL_TIME];  // ******************
	// Reflow  // ******************
	endTime.reflow = endTime.soak + ( ee4->parameter[REFLOW_SOLDER_MELT_TEMP] - soakEndTemp ) /  // ******************
	( ee4->parameter[REFLOW_HEAT_RATE] * parameterMenu[REFLOW_HEAT_RATE].multiplier ) + ee4->parameter[REFLOW_DWELL_TIME_ABOVE_MELT_TEMP];   // ****** can't divide by zero if heat rate = zero  // ******************
	// Cooldown  // ******************
	endTime.cooldown = endTime.reflow + ( ee4->parameter[REFLOW_SOLDER_MELT_TEMP] -  // ******************
	ee4->parameter[COOLDOWN_END_TEMP] ) / ( ee4->parameter[COOLDOWN_RATE] * parameterMenu[COOLDOWN_RATE].multiplier );  // ******* can't divide by zero if heat rate = zero  // ******************
 
	//  Compute profile incremental temperatures  // ******************
	float previousTemp = 0;  // ******************

	for ( int16_t i = 0; i < NUMPROFILETEMPS; ++i ) {  // 0 - 47 temperatures is normal #  // ******************
		int16_t t = i * inv;  // t is current time interval  // ******************
		// Preheat  // ******************
		if( t <= ee4->parameter[PREHEAT_START_TEMP_DWELL_TIME] )  // ******************
      		temp = ee4->parameter[PREHEAT_START_TEMP];  // ******************
    	else if( t <= endTime.preheat ) {  // ******************
    		temp = previousTemp + ( ee4->parameter[PREHEAT_HEAT_RATE] * parameterMenu[PREHEAT_HEAT_RATE].multiplier * inv ); // ******************
    		if( temp > ee4->parameter[SOAK_START_TEMP] )  // ******************
    			temp = ee4->parameter[SOAK_START_TEMP];  // ******************
    	}  // ******************
    	// Soak  // ******************
    	else if( t <= endTime.soak ) {  // ******************
    		temp = previousTemp + ( ee4->parameter[SOAK_HEAT_RATE] * parameterMenu[SOAK_HEAT_RATE].multiplier * inv ); // ******************
      		if( temp > ee4->parameter[SOAK_END_TARGET_TEMP] )  // ******************
				temp = ee4->parameter[SOAK_END_TARGET_TEMP];  // ******************
    	}  // ******************
    	// Reflow  // ******************
    	else if( t <= endTime.reflow - ( ee4->parameter[REFLOW_OVERSHOOT_MELT_TEMP_DELTA] / ( ee4.parameter[REFLOW_HEAT_RATE] * parameterMenu[REFLOW_HEAT_RATE].multiplier )) ) { // ******* can't divide by zero if heat rate = zero // ******************
      		temp = previousTemp + ( ee4->parameter[REFLOW_HEAT_RATE] * parameteerMenu[REFLOW_HEAT_RATE].multiplier * inv );   // ******************
    		if( temp > ee4->parameter[REFLOW_SOLDER_MELT_TEMP] + ee4->parameter[REFLOW_OVERSHOOT_MELT_TEMP_DELTA] )  // ******************
				temp = ee4->parameter[REFLOW_SOLDER_MELT_TEMP] + ee4->parameter[REFLOW_OVERSHOOT_MELT_TEMP_DELTA];  // ******************
    	}  // ******************
    	else if( t <= endTime.reflow ) temp = previousTemp - ( ee4->parameter[REFLOW_HEAT_RATE] * parameterMenu[REFLOW_HEAT_RATE].multiplier * inv );  // ******************
    	// Cooldown  // ******************
    	else if( t <= endTime.cooldown ) {  // ******************
    		temp = previousTemp - ( ee4->parameter[COOLDOWN_RATE] * parameterMenu[COOLDOWN_RATE].multiplier * inv );  // ******************
      		if ( temp < ee4->parameter[COOLDOWN_END_TEMP] )  // ******************
        		temp = ee4->parameter[COOLDOWN_END_TEMP];  // ******************
    	}  	// ******************
    	else temp = 0;  // ends profile and places oven in standby mode.  // ******************

    	ee3.temperatures[i] = temp;  // **** With each loop, Save Temperatures dataset to ramprofile ee3  // **************
    	previousTemp = temp;  // save prevous temp for next loop // ******************
  	}  // ******************
}  // *******************

void Reflow_readEEprofileParameters(void) { // *************
	  int addr = 0; // ****************
	  int curProfile = NV_GetConfig(REFLOW_PROFILE); // ************
	  if (curProfile > 6) curProfile = 6; // *****************
	  switch(curProfile){ // ********************
        case 0: addr = 0x03; break; // **************
        case 1: addr = 0x22; break; // **************
        case 2: addr = 0x42; break; // **************
        case 3: addr = 0x70; break; // **************
        case 4: addr = 0x90; break; // **************
		case 5: addr = 0xb0; break; // **************
		case 6: addr = 0xc0;  // **************
	  } // **************
	  EEPROM_Read( (uint8_t*) &ee4, addr, sizeof(ee4) ); // **********
} // ********************

void Reflow_writeEEprofileParameters(void) { // *************
	  int addr = 0; // ****************
	  int curProfile = NV_GetConfig(REFLOW_PROFILE); // ************
	  if (curProfile > 6) curProfile = 6; // *****************
	  switch(curProfile){ // ********************
        case 0: addr = 0x03; break; // **************
        case 1: addr = 0x22; break; // **************
        case 2: addr = 0x42; break; // **************
        case 3: addr = 0x70; break; // **************
        case 4: addr = 0x90; break; // **************
		case 5: addr = 0xb0; break; // **************
		case 6: addr = 0xc0; // **************
	  } // **************
	  EEPROM_Write( addr, (uint8_t*) &ee4, sizeof(ee4) ); // **********
} // ********************

int Reflow_getNumParameterProfiles(void) { // *************
	return NUM_PARAMETER_PROFILES; // *****************
} // *****************

int Reflow_getNumParamMenuItems(void) { // *************
	return NUM_PARAMETER_MENU_ITEMS; // *****************
} // *****************

void Reflow_editParamMenu_decreaseValue(int item, int amount) { // *************
	int curval = Reflow_Edit_Param_Menu_getRawValue(item) - amount; // *****************

	int minval = setupmenu[item].minval; // *****************
	if (curval < minval) curval = minval; // *****************

	Reflow_Edit_Param_Menu_setValue(item, curval); // *****************
}  // *****************

void Reflow_editParamMenu_increaseValue(int item, int amount) {  // *************
	int curval = Reflow_Edit_Param_Menu_getRawValue(item) + amount; // *****************

	int maxval = parametermenu[item].maxval; // *****************
	if (curval > maxval) curval = maxval; // *****************

	Reflow_Edit_Para_Menu_setValue(item, curval);  // *****************
} // *****************

int Reflow_editParamMenu_getRawValue(int item) {  // *************
	return ee4->parameter[item]);  // *****************
}  // *****************

void Reflow_editParamMenu_setRawValue(int item, int value) {  // *************
	ee4->paarameter[item] = value; // *****************
}  // *****************

float Reflow_editParamMenu_getValue(int item) { // *************
	int intval = Reflow_editParamMenu_getRawValue(item); // *****************
	return ((float)intval) * parametermenu[item].multiplier;  // *****************
} // *****************

int Reflow_editParamMenu_snprintFormattedValue(char* buf, int n, int item) { // *************
	return snprintf(buf, n, parametermenu[item].formatstr, Setup_getValue(item)); // *****************
} // *****************
