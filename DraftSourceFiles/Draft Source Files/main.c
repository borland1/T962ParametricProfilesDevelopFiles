/*
 * main.c - T-962 reflow controller
 *
 * Copyright (C) 2014 Werner Johansson, wj@unifiedengineering.se
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "LPC214x.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "serial.h"
#include "lcd.h"
#include "io.h"
#include "sched.h"
#include "onewire.h"
#include "adc.h"
#include "i2c.h"
#include "rtc.h"
#include "eeprom.h"
#include "keypad.h"
#include "reflow.h"
#include "reflow_profiles.h"
#include "sensor.h"
#include "buzzer.h"
#include "nvstorage.h"
#include "version.h"
#include "vic.h"
#include "max31855.h"
#include "systemfan.h"
#include "setup.h"

extern uint8_t logobmp[];
extern uint8_t stopbmp[];
extern uint8_t selectbmp[];
extern uint8_t editbmp[];
extern uint8_t f3editbmp[];
extern uint8_t editParamMenubmp[]; // *********************
extern uint8_t editNameMenubmp[]; // *********************
extern uint8_t editRenameMenubmp[]; // *********************
extern uint8_t scrollBarbmp[]; // *********************
extern uint8_t scrollBarTicksbmp[]; // *********************
extern paramProfile ee4;  // *********************
extern ramprofile ee3;    // *********************
extern endTimes endTime;  // *********************

// No version.c file generated for LPCXpresso builds, fall back to this
__attribute__((weak)) const char* Version_GetGitVersion(void) {
	return "no version info";
}

static char* format_about = \
"\nT-962-controller open source firmware (%s)" \
"\n" \
"\nSee https://github.com/UnifiedEngineering/T-962-improvements for more details." \
"\n" \
"\nInitializing improved reflow oven..."; // fix URL typo // *********************

static char* help_text = \
"\nT-962-controller serial interface.\n\n" \
" about                   Show about + debug information\n" \
" bake <setpoint>         Enter Bake mode with setpoint\n" \
" bake <setpoint> <time>  Enter Bake mode with setpoint for <time> seconds\n" \
" help                    Display help text\n" \
" list profiles           List available reflow profiles\n" \
" list settings           List machine settings\n" \
" quiet                   No logging in standby mode\n" \
" reflow                  Start reflow with selected profile\n" \
" setting <id> <value>    Set setting id to value\n" \
" select profile <id>     Select reflow profile by id\n" \
" stop                    Exit reflow or bake mode\n" \
" values                  Dump currently measured values\n" \
"\n";

static int32_t Main_Work(void);

int main(void) {
	char buf[22];
	int len;

	IO_JumpBootloader();

	PLLCFG = (1 << 5) | (4 << 0); //PLL MSEL=0x4 (+1), PSEL=0x1 (/2) so 11.0592*5 = 55.296MHz, Fcco = (2x55.296)*2 = 221MHz which is within 156 to 320MHz
	PLLCON = 0x01;
	PLLFEED = 0xaa;
	PLLFEED = 0x55; // Feed complete
	while (!(PLLSTAT & (1 << 10))); // Wait for PLL to lock
	PLLCON = 0x03;
	PLLFEED = 0xaa;
	PLLFEED = 0x55; // Feed complete
	VPBDIV = 0x01; // APB runs at the same frequency as the CPU (55.296MHz)
	MAMTIM = 0x03; // 3 cycles flash access recommended >40MHz
	MAMCR = 0x02; // Fully enable memory accelerator

	VIC_Init();
	Sched_Init();
	IO_Init();
	Set_Heater(0);
	Set_Fan(0);
	Serial_Init();
	printf(format_about, Version_GetGitVersion());

	I2C_Init();
	EEPROM_Init();
	NV_Init();

	LCD_Init();
	LCD_BMPDisplay(logobmp, 0, 0);

	IO_InitWatchdog();
	IO_PrintResetReason();

	len = IO_Partinfo(buf, sizeof(buf), "%s rev %c");
	LCD_disp_str((uint8_t*)buf, len, 0, 64 - 6, FONT6X6);
	printf("\nRunning on an %s", buf);

	len = snprintf(buf, sizeof(buf), "%s", Version_GetGitVersion());
	LCD_disp_str((uint8_t*)buf, len, 128 - (len * 6), 0, FONT6X6);

	LCD_FB_Update();
	Keypad_Init();
	Buzzer_Init();
	ADC_Init();
	RTC_Init();
	OneWire_Init();
	SPI_TC_Init();
	Reflow_Init();
	SystemFan_Init();

	Sched_SetWorkfunc(MAIN_WORK, Main_Work);
	Sched_SetState(MAIN_WORK, 1, TICKS_SECS(4)); // Enable in 2 seconds // 2 -> 4 secs // ****************
	
	Buzzer_Beep(BUZZ_NONE, 255, TICKS_MS(100));  // BUZZ_NONE instead of BUZZ_1KHZ

	while (1) {
#ifdef ENABLE_SLEEP
		int32_t sleeptime;
		sleeptime = Sched_Do(0); // No fast-forward support
		//printf("\n%d ticks 'til next activity"),sleeptime);
#else
		Sched_Do(0); // No fast-forward support
#endif
	}
	return 0;
}

typedef enum eMainMode {
	MAIN_HOME = 0,
	MAIN_ABOUT,
	MAIN_SETUP,
	MAIN_BAKE,
	MAIN_SELECT_PROFILE,
	MAIN_EDIT_PROFILE,
	MAIN_EDIT_PROFILE_PARAMETERS,		// ******************
	MAIN_EDIT_PROFILE_NAME,				// ******************
	MAIN_EDIT_PROFILE_RESTORE_DEFAULTS,	// ******************
	MAIN_REFLOW
} MainMode_t;

static int32_t Main_Work(void) {
	static MainMode_t mode = MAIN_HOME;
	static uint16_t setpoint = 0;
	if (setpoint == 0) {
		Reflow_LoadSetpoint();
		setpoint = Reflow_GetSetpoint();
	}
	static int timer = 0;
	
	// profile editing  // not used with parameter profiles editing // ********************
	// static uint8_t profile_time_idx = 0;  // not used with parameter profiles editing // ********************
	// static uint8_t current_edit_profile = 0; // not used with parameter profiles editing // ********************
	
	int32_t retval = TICKS_MS(500);

	char buf[22];
	int len;

	uint32_t keyspressed = Keypad_Get();

	char serial_cmd[255] = "";
	char* cmd_select_profile = "select profile %d";
	char* cmd_bake = "bake %d %d";
	char* cmd_dump_profile = "dump profile %d";
	char* cmd_setting = "setting %d %f";

	if (uart_isrxready()) {
		int len = uart_readline(serial_cmd, 255);

		if (len > 0) {
			int param, param1;
			float paramF;

			if (strcmp(serial_cmd, "about") == 0) {
				printf(format_about, Version_GetGitVersion());
				len = IO_Partinfo(buf, sizeof(buf), "\nPart number: %s rev %c\n");
				printf(buf);
				EEPROM_Dump();

				printf("\nSensor values:\n");
				Sensor_ListAll();

			} else if (strcmp(serial_cmd, "help") == 0 || strcmp(serial_cmd, "?") == 0) {
				printf(help_text);

			} else if (strcmp(serial_cmd, "list profiles") == 0) {
				printf("\nReflow profiles available:\n");

				Reflow_ListProfiles();
				printf("\n");

			} else if (strcmp(serial_cmd, "reflow") == 0) {
				printf("\nStarting reflow with profile: %s\n", Reflow_GetProfileName(&ee3));
				mode = MAIN_HOME;
				// this is a bit dirty, but with the least code duplication.
				keyspressed = KEY_S;

			} else if (strcmp(serial_cmd, "list settings") == 0) {
				printf("\nCurrent settings:\n\n");
				for (int i = 0; i < Setup_getNumItems() ; i++) {
					printf("%d: ", i);
					Setup_printFormattedValue(i);
					printf("\n");
				}

			} else if (strcmp(serial_cmd, "stop") == 0) {
				printf("\nStopping bake/reflow");
				mode = MAIN_HOME;
				Reflow_SetMode(REFLOW_STANDBY);
				retval = 0;

			} else if (strcmp(serial_cmd, "quiet") == 0) {
				Reflow_ToggleStandbyLogging();
				printf("\nToggled standby logging\n");

			} else if (strcmp(serial_cmd, "values") == 0) {
				printf("\nActual measured values:\n");
				Sensor_ListAll();
				printf("\n");

			} else if (sscanf(serial_cmd, cmd_select_profile, &param) > 0) {
				// select profile
				Reflow_SelectProfileIdx(param);
				printf("\nSelected profile %d: %s\n", param, Reflow_GetProfileName(&ee3));

			} else if (sscanf(serial_cmd, cmd_bake, &param, &param1) > 0) {
				if (param < SETPOINT_MIN) {
					printf("\nSetpoint must be >= %ddegC\n", SETPOINT_MIN);
					param = SETPOINT_MIN;
				}
				if (param > SETPOINT_MAX) {
					printf("\nSetpont must be <= %ddegC\n", SETPOINT_MAX);
					param = SETPOINT_MAX;
				}
				if (param1 < 1) {
					printf("\nTimer must be greater than 0\n");
					param1 = 1;
				}

				if (param1 < BAKE_TIMER_MAX) {
					printf("\nStarting bake with setpoint %ddegC for %ds after reaching setpoint\n", param, param1);
					timer = param1;
					Reflow_SetBakeTimer(timer);
				} else {
					printf("\nStarting bake with setpoint %ddegC\n", param);
				}

				setpoint = param;
				Reflow_SetSetpoint(setpoint);
				mode = MAIN_BAKE;
				Reflow_SetMode(REFLOW_BAKE);

			} else if (sscanf(serial_cmd, cmd_dump_profile, &param) > 0) {
				printf("\nDumping profile %d: %s\n ", param, Reflow_GetProfileName(&ee3));
				Reflow_DumpProfile(param);

			} else if (sscanf(serial_cmd, cmd_setting, &param, &paramF) > 0) {
				Setup_setRealValue(param, paramF);
				printf("\nAdjusted setting: ");
				Setup_printFormattedValue(param);

			} else {
				printf("\nCannot understand command, ? for help\n");
			}
		}
	}

	// main menu state machine
	if (mode == MAIN_SETUP) {
		static uint8_t selected = 0;
		int y = 0;

		int keyrepeataccel = keyspressed >> 17; // Divide the value by 2
		if (keyrepeataccel < 1) keyrepeataccel = 1;
		if (keyrepeataccel > 30) keyrepeataccel = 30;

		if (keyspressed & KEY_F1) {
			if (selected > 0) { // Prev row
				selected--;
			} else { // wrap
				selected = Setup_getNumItems() - 1;
			}
		}
		if (keyspressed & KEY_F2) {
			if (selected < (Setup_getNumItems() - 1)) { // Next row
				selected++;
			} else { // wrap
				selected = 0;
			}
		}

		if (keyspressed & KEY_F3) {
			Setup_decreaseValue(selected, keyrepeataccel);
		}
		if (keyspressed & KEY_F4) {
			Setup_increaseValue(selected, keyrepeataccel);
		}

		LCD_FB_Clear();
			len = snprintf(buf, sizeof(buf), "Setup/Calibration");  // includes calibration  // *****************
			LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), y, FONT6X6);
			y += 7;

		for (int i = 0; i < Setup_getNumItems() ; i++) {
				len = Setup_snprintFormattedValue(buf, sizeof(buf), i);
				LCD_disp_str((uint8_t*)buf, len, 0, y, FONT6X6 | (selected == i) ? INVERT : 0);
				y += 7;
		}

		// Key - button press  // ****************
		y = 64 - 7;
		LCD_BMPDisplay(editParamMenubmp, 0, y);  // ****************
		
		// Leave setup
		if (keyspressed & KEY_S) {
			mode = MAIN_HOME;
			Reflow_SetMode(REFLOW_STANDBY);
			retval = 0; // Force immediate refresh
		}
	} else if (mode == MAIN_ABOUT) {
			LCD_FB_Clear();
			LCD_BMPDisplay(logobmp, 0, 0);

			len = snprintf(buf, sizeof(buf), "T-962 controller");
			LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), 0, FONT6X6);

			len = snprintf(buf, sizeof(buf), "%s", Version_GetGitVersion());
			LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), 64 - 6, FONT6X6);

			LCD_BMPDisplay(stopbmp, 127 - 16, 0); // *********** 17 -> 16 

			// Leave about with any key.
			if (keyspressed & KEY_ANY) {
					mode = MAIN_HOME;
					retval = 0; // Force immediate refresh
			}
	} else if (mode == MAIN_REFLOW) {
		uint32_t ticks = RTC_Read();
		char blankLine[20]; // *********************
		memset(blankLine, ' ', sizeof(blankLine) - 1); // *********************
		blankLine[19] = '\0'; // *********************

		if (ticks > 5 && ( !Reflow_IsDone() ) ) { // wait 5 seconds before clearing line // *********************
			LCD_disp_str((uint8_t*)blankLine, sizeof(blankLine), 13, 0, FONT6X6); // Clear line // *********************
			
			char currentPhase[10]; // *********************
			uint32_t remaining; // *********************
			if (ticks < endTime.preheat) {  // *********************
				remaining = endTime.preheat - ticks; // **********
				strcpy(currentPhase, "Preheat"); // *********************
			} else if (ticks < endTime.soak) {  // *********************
				remaining = endTime.soak - ticks;  // *******************
				strcpy(currentPhase, "Soak"); // *********************
			} else if (ticks < endTime.reflow) {  // *********************
				remaining = endTime.reflow - ticks;  // *****************
				strcpy(currentPhase, "Reflow"); // *********************
			} else {  			// ********************* 
				remaining = endTime.cooldown - ticks; // **********
			 	strcpy(currentPhase, "Cooldown"); // *********************
			}; // *********************

			len = snprintf(buf, sizeof(buf), "%s", currentPhase); // *********************
			LCD_disp_str((uint8_t*)currentPhase, sizeof(currentPhase), LCD_ALIGN_RIGHT((uint8_t)len), 0, FONT6X6); // *************
			// Display time remaining  //  **********************
			uint32_t phaseCntDnMin = remaining / 60; // **********************
			uint32_t phaseCntDnSec = remaining % 60; // ***********************
			len = snprintf(buf, sizeof(buf), "%1u", (unsigned int) phaseCntDnMin);  // *************
			LCD_disp_str((uint8_t*)buf, (uint8_t)len, 13, 0, FONT6X6);  // *************
			len = snprintf(buf, sizeof(buf), "%02u",  (unsigned int) phaseCntDnSec); 	// ************** 
			LCD_disp_str((uint8_t*)buf, (uint8_t)len, 13 + 8, 0, FONT6X6);  // *************
			LCD_SetPixel( 13 + 7, 2 ); // Colon between min and sec // *************
			LCD_SetPixel( 13 + 7, 4 ); // *************	
		}		// *********************

		len = snprintf(buf, sizeof(buf), "%03u", Reflow_GetSetpoint());
		LCD_disp_str((uint8_t*)"SET", 3, 110, 7, FONT6X6);
		LCD_disp_str((uint8_t*)buf, len, 110, 13, FONT6X6);

		len = snprintf(buf, sizeof(buf), "%03u", Reflow_GetActualTemp());
		LCD_disp_str((uint8_t*)"ACT", 3, 110, 20, FONT6X6);
		LCD_disp_str((uint8_t*)buf, len, 110, 26, FONT6X6);

		LCD_disp_str((uint8_t*)"RUN", 3, 110, 33, FONT6X6); // *************
		uint32_t runCntDnMin = ( (uint32_t) endTime.cooldown - ticks ) / 60; // **********************
		uint32_t runCntDnSec = ( (uint32_t) endTime.cooldown - ticks ) % 60; // ***********************
		len = snprintf(buf, sizeof(buf), "%1u", (unsigned int) runCntDnMin);  // *************
		LCD_disp_str((uint8_t*)buf, len, 108, 39, FONT6X6);  // *************
		len = snprintf(buf, sizeof(buf), "%02u",  (unsigned int) runCntDnSec); 	// ************** 
		LCD_disp_str((uint8_t*)buf, len, 116, 39, FONT6X6);  // *************
		LCD_SetPixel( 115, 41 ); // Colon between min and sec // *************
		LCD_SetPixel( 115, 43 ); // *************		

		// Abort reflow
		if (Reflow_IsDone() || keyspressed & KEY_S) {
			printf("\nReflow %s\n", (Reflow_IsDone() ? "done" : "interrupted by keypress"));
			if (Reflow_IsDone()) {
				Buzzer_Beep(BUZZ_1KHZ, 255, TICKS_MS(100) * NV_GetConfig(REFLOW_BEEP_DONE_LEN));
			}
			mode = MAIN_HOME;
			Reflow_SetMode(REFLOW_STANDBY);
			retval = 0; // Force immediate refresh
		}

	} else if (mode == MAIN_SELECT_PROFILE) {
			int curprofile = Reflow_GetProfileIdx();
			LCD_FB_Clear();

			// Prev profile
			if (keyspressed & KEY_F1) {
					curprofile--;
					Reflow_SelectProfileIdx(curprofile); // **********************
					Reflow_readEE_profileParams( &ee4, &ee3);  // ********************
			}
			// Next profile
			if (keyspressed & KEY_F2) {
					curprofile++;
					Reflow_SelectProfileIdx(curprofile); // ********************
					Reflow_readEE_profileParams( &ee4, &ee3); // ******************
			}

			// Reflow_SelectProfileIdx( curprofile ); // moved to key F1/F2 checks **************
			
			Reflow_PlotProfile(-1);
			LCD_BMPDisplay(selectbmp, 127 - 17, 0);
			// int eeidx = Reflow_GetEEProfileIdx(); not used with parameter profiles // ********************
			// if (eeidx) { // Display edit button
			// LCD_BMPDisplay(f3editbmp, 127 - 17, 29); // try change 17 -> 16  // ***********
			// }  // ********************
			len = snprintf(buf, sizeof(buf), "%s", Reflow_GetProfileName(&ee3)); // **************
			LCD_disp_str((uint8_t*)buf, len, 13, 0, FONT6X6);

			if ( keyspressed & KEY_F3) {  // ****************
					mode = MAIN_EDIT_PROFILE;
					// current_edit_profile = eeidx; // not used with parameter profiles // ********************
					retval = 0; // Force immediate refresh
			}

			// Select current profile
			if (keyspressed & KEY_S) {
					mode = MAIN_HOME;
					retval = 0; // Force immediate refresh
			}

 	} else if (mode == MAIN_EDIT_PROFILE) { 	// **************
		LCD_FB_Clear();  // *******************
		int keyrepeataccel = keyspressed >> 17; // Divide the value by 2  // *************
		if (keyrepeataccel < 1) keyrepeataccel = 1; // *************
		if (keyrepeataccel > 30) keyrepeataccel = 30; // *************
		// *******************
		LCD_disp_str((uint8_t*)"EDIT PROFILE", 12, LCD_ALIGN_CENTER(12), 0, FONT6X6); // *************
		len = snprintf(buf, sizeof(buf), "%s", Reflow_GetProfileName(&ee3)); // ****
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), 8 * 1, FONT6X6 | INVERT); // *************
		LCD_disp_str((uint8_t*)"F1", 2, 0, 8 * 3, FONT6X6 | INVERT);		// **********
		LCD_disp_str((uint8_t*)"EDIT PARAMETERS", 15, 14, 8 * 3, FONT6X6); // **********
		LCD_disp_str((uint8_t*)"F2", 2, 0, 8 * 4, FONT6X6 | INVERT); // ***********
		LCD_disp_str((uint8_t*)"RENAME TITLE", 12, 14, 8 * 4, FONT6X6); // ********
		LCD_disp_str((uint8_t*)"F3", 2, 0, 8 * 5, FONT6X6 | INVERT); // ***********
		LCD_disp_str((uint8_t*)"RESTORE DEFAULTS", 16, 14, 8 * 5, FONT6X6); // ******
		LCD_disp_str((uint8_t*)"F4", 2, 0, 8 * 6, FONT6X6 | INVERT); // *****************
		LCD_disp_str((uint8_t*)"RETURN", 6, 14, 8 * 6, FONT6X6); // ***********
		
		if (keyspressed & KEY_F1) { // EDIT PROFILE PARAMETERS  // **************
			mode = MAIN_EDIT_PROFILE_PARAMETERS;  // *************
		} // *********************************
		if (keyspressed & KEY_F2) { //  EDIT PROFILE NAME // ******************
			mode = MAIN_EDIT_PROFILE_NAME; // Edit Profile Name // ******************
		}   // ********************
		if (keyspressed & KEY_F3) { // RESTORE PROFILE DEFAULTS // *************
			mode = MAIN_EDIT_PROFILE_RESTORE_DEFAULTS; // Restore Defaults Menu // *******************
		}  // **********************
		if (keyspressed & KEY_F4) { // RETURN  // ***********************
			mode = MAIN_SELECT_PROFILE; // *****************************
		}  // ************************************
	}  // **************************

	else if (mode == MAIN_EDIT_PROFILE_PARAMETERS) {  // *******************************
		static int8_t cursorPos = 0;  // cursor at parameter value position // *************************
		static int8_t line1Idx = 1; // index for display line two, vertical scrolling // *********************
		static int8_t line2Idx = 2; // index for display line three, vertical scrolling // *********************
		int y = 0;	// *************************

		int keyrepeataccel = keyspressed >> 17; // Divide the value by 2 // ************
		if (keyrepeataccel < 1) keyrepeataccel = 1; // ***********************
		if (keyrepeataccel > 30) keyrepeataccel = 30; // **************************
		
		if (keyspressed & KEY_F1) {	// Previous, vertical scrolling // *************
			--cursorPos;  // *********************
			if ((cursorPos == 0 && line2Idx == 3) ||   // ***************
			     (cursorPos == 1 && line2Idx == 4) ||  // ***************
			     (cursorPos == 5 && line2Idx == 9)) { // ***************
 				--line2Idx;  // ***************
			}  // ***************
			else if (cursorPos == 2 && line2Idx == 6) { // *****************
				line1Idx -= 4;  // ***************
				line2Idx -= 2;  // *************** 
			}  // ***************
			else if (cursorPos == 4 && line2Idx == 8) { // ***************
				line2Idx -= 2;  // *************** 
			}  // ***************
			else if (cursorPos == 6 && line2Idx == 11) { // *****************
				line1Idx -= 5;  // ***************
				line2Idx -= 2;  // ***************
			}  // ***************
			else if (cursorPos == 8 && line2Idx == 14) { // ***************
				line2Idx -= 3;  // *************** 					
			}  // ****************
			if (cursorPos < 0) { // wrap around // ****************
				cursorPos = 12;  // *****************
				line1Idx = 10;  // *********************
				line2Idx = 14;  // *******************
			}    // ******************
		} // **************************

		if (keyspressed & KEY_F2) {	// Next, vertical scrolling // *************
			++cursorPos;  // *******************
			if ((cursorPos == 5 && line2Idx == 2) ||   // ***************
			    (cursorPos == 6 && line2Idx == 3) ||  // ***************
			    (cursorPos == 9 && line2Idx == 8)) { // ***************
 				++line2Idx;  // ***************
			}  // ***************
			else if (cursorPos == 7 && line2Idx == 4) { // *****************
				line1Idx += 4;  // ***************
				line2Idx += 2;  // *************** 
			}  // ***************
			else if (cursorPos == 8 && line2Idx == 6) { // ***************
				line2Idx += 2;  // *************** 
			}  // ***************
			else if (cursorPos == 10 && line2Idx == 9) { // *****************
				line1Idx += 5;  // ***************
				line2Idx += 2;  // *************** 
			}  // ***************
			else if (cursorPos == 11 && line2Idx == 11) { // ***************
				line2Idx += 3;  // *************** 					
			}  // ****************
			if (cursorPos > 12) { // wrap around // ****************
				cursorPos = 0;  // *****************
				line1Idx = 1;  // *********************
				line2Idx = 2;  // *******************
			}    // ******************
		}  // ***************************

		if (keyspressed & KEY_F3) {  // *******************************
				Reflow_editParamMenu_decreaseValue(cursorPos, keyrepeataccel); // ****************
			}  // *********************************************
		if (keyspressed & KEY_F4) {  // **************************
				Reflow_editParamMenu_increaseValue(cursorPos, keyrepeataccel);  // ****************
			}  // ************************************* 

		LCD_FB_Clear();  // **********************
		
		// Display Header on line0 // ************************
		len = snprintf(buf, sizeof(buf), "EDIT PARAMETERS"); // *************
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), y, FONT6X6); // ***************
		y += 7; // *************************
			
		// Display Profle Phase on line1 // *****************************
		if (line1Idx == 1 )       // ******************************
			LCD_disp_str((uint8_t*)"-PREHEAT-", 9, LCD_ALIGN_CENTER(9), y, FONT6X6);  // ***************
		else if (line1Idx == 5)   // ***************************
			LCD_disp_str((uint8_t*)"-SOAK-", 6, LCD_ALIGN_CENTER(6), y, FONT6X6);     // ***************
		else if (line1Idx == 10)  // ***************************
			LCD_disp_str((uint8_t*)"-REFLOW-", 8, LCD_ALIGN_CENTER(8), y, FONT6X6);   // ***************
		 
		// Display indexed parameters on LCD lines 2-7   // *******************************
		y += 7;  // **************************
		for (int i = line2Idx; i  < line2Idx + 6; i++) {  // i = line2Idx + # inc.s = line # // *************************
			// adjust cursorOffsets by value of i, where i is line2Idx + # inc.'s // ***********	
			int8_t cursorOffset = 2;  // ***************
				
			if(i > 5 && i < 10) cursorOffset = 3;  // ********************
			else if(i > 10 && i < 13) cursorOffset = 4;  // ************
			else if(i > 13 && i < 16) cursorOffset = 5;  // ***************
			else if(i > 17) cursorOffset = 7;  // **********************
			
			if(	(line2Idx == 2 && i == (line2Idx + 3)) ||  // *******************
				(line2Idx == 3 && i == (line2Idx + 2)) || // ******************
				(line2Idx == 4 && i == (line2Idx + 1)) ) {   // ******************
				LCD_disp_str((uint8_t*)"-SOAK-", 6, LCD_ALIGN_CENTER(6), y, FONT6X6);  // ***************
				y+=7; // ****************************
			}  // ******************************
			else if( (line2Idx == 6 && i == (line2Idx + 4)) ||  // ************************
					 (line2Idx == 8 && i == (line2Idx + 2)) ||  // ************************
					 (line2Idx == 9 && i == (line2Idx + 1)) ) {  // ************************
				LCD_disp_str((uint8_t*)"-REFLOW-", 8, LCD_ALIGN_CENTER(8), y, FONT6X6);  // ***************
				y+=7; // ****************************
			}  // ************************
			else if( line2Idx == 14 && i == (line2Idx + 3) ) {  // *****************************
				LCD_disp_str((uint8_t*)"-COOLDOWN-", 10, LCD_ALIGN_CENTER(10), y, FONT6X6);  // ***************
				y+=7; // ****************************
			} // ****************************
			// for two text line params  // ************************
			else if( // cursorPos == 8  // *******************
					(line2Idx == 8 && i == (line2Idx + 4)) ||  // *************
					(line2Idx == 9 && i == (line2Idx + 3)) ||  // *************
					(line2Idx == 11 && i == (line2Idx + 1)) || // *************
					// cursorPos == 10  // ****************
					(line2Idx == 11 && i == (line2Idx + 4)) || // **************
					(line2Idx == 14 && i == (line2Idx + 1))    // **************
				) {  // *******************
				char bigBuf[45];
				len = Reflow_editParamMenu_snprintFormattedValue(bigBuf, sizeof(bigBuf), i - cursorOffset); // *************
				int count = 0; // *************
				for (int j = 0; bigBuf[j] != '\n'; j++) {  // find count before'\n' *************
					if (bigBuf[j] != '\n') ++count;  // *************
				} // ****************************
				LCD_disp_str((uint8_t*)bigBuf, count , 0, y, FONT6X6 | ( (i - cursorOffset) == cursorPos ) ? INVERT : 0); // **********
				y +=7; // ****************************
				++count; // set count for after '\n' // *************
				LCD_disp_str((uint8_t*)bigBuf + count, len - count, 0, y, FONT6X6 | ( (i - cursorOffset) == cursorPos ) ? INVERT : 0); // ***********
				y +=7; // ****************************
				++i;  // *********************
			} // *************************
			else {  // ***********************
				len = Reflow_editParamMenu_snprintFormattedValue(buf, sizeof(buf), i - cursorOffset); // *************
				LCD_disp_str((uint8_t*)buf, len, 0, y, FONT6X6 | ( (i - cursorOffset)  == cursorPos ) ? INVERT : 0); // **********
				y+=7; // ****************************
			} // ************
		}  // **********************
		// plot vertical scroll bar graphic // *******************************
		// LCD_BMPDisplay(scrollBarTicksbmp, 127, 4); // *************
		LCD_BMPDisplay(scrollBarbmp, 127, 6 + (cursorPos * 3) ); // *************
				
		// Key - button press // *******************************	
		y = 64 - 7; // ******************************************************
		LCD_BMPDisplay(editParamMenubmp, 0, y); // ****************
					
		if (keyspressed & KEY_S) {  // ******************************
			cursorPos = 0;  // Reset parameter index // ********************
			line1Idx = 1; // Reset line counter // ********************
			line2Idx = 2; // Reset header line index // ********************
			Reflow_computeTempDatasetFromParams( &ee4, &ee3 ); // Update profile temp dataset // *********************
			Reflow_writeEE_profileParams( &ee4 ); // Save profile params to EEPROM // *********************
			mode = MAIN_EDIT_PROFILE;  // Return to Edit Profile Menu // *********************
			retval = 0; // Force immediate refresh  // *********************
		}  // *********************************	
	}  // **************************************
	
	else if (mode == MAIN_EDIT_PROFILE_NAME) {  // *********************
		static char editName[PROFILE_NAME_LEN];   // **********************
		static int cursorPos = 0; // Profile Name Cursor Position // *********************
		static uint8_t initialized = 0;  // workaround declare static var // *********************
		
		if (!initialized) { // Init buffer with spaces // *********************
			strncpy(editName, Reflow_GetProfileName(&ee3), PROFILE_NAME_LEN);  // Profile Name Buffer // *********************
			cursorPos = 0; // Profile Name Cursor Position // *********************
			initialized = 1; // Init only once // *********************
		} // *******************************
			
		LCD_FB_Clear();  // *****************************
		int y = 0;  // ******************
		int x = 6;  // position of first character space // ********************
		
		LCD_disp_str((uint8_t*) "EDIT NAME", 9, LCD_ALIGN_CENTER(9), y, FONT6X6); // **********
		y = ( 4 * 7 ); // *************************
		for ( int i = 0; i < PROFILE_NAME_LEN; i++) {  // *******************
			LCD_disp_str((uint8_t*)&editName[i], 1, x + (i * 6), y, FONT6X6 | (cursorPos == i) ? INVERT : 0); // ***********
			for (int j = 0; j < 8; j++) {  // **********************
				if (i == cursorPos + 1) LCD_SetPixel( x + (i * 6), y + j); // ****************
			} // *******************
		}  // *******************************************
		LCD_BMPDisplay(editRenameMenubmp, x - 1, y - 1 ); // plot border text length ****************
		// Key - button press  // *****************************
		LCD_BMPDisplay(editNameMenubmp, 0, 64 - 7 ); // ****************

		if (keyspressed & KEY_F1) {	// move cursor to left or loop back  // *************
			--cursorPos; // Move left one character space // ******************************
			if (cursorPos < 0) {  // ******************************************
				cursorPos = (PROFILE_NAME_LEN - 1);  // Wrap  // ******************************
			} // ****************************************
		}  // *************************************
		if (keyspressed & KEY_F2) { // move cursor to right or loop back  // ***************
			++cursorPos;  // Move right one character space // ******************
			if (cursorPos > (PROFILE_NAME_LEN - 1)) cursorPos = 0;  // wrap  // *******************************
		}  // *******************************************
		if (keyspressed & KEY_F3) {  // ASCII values 32-96, & 123-126 are only available fonts  // *************************
			// Decrease char ASCII decimal value   // *********************
			editName[cursorPos] = (editName[cursorPos] - 1); // *********************
			if ( editName[cursorPos] < 32 ) // *********************************
				editName[cursorPos] = (editName[cursorPos] + 65);  // wrap, (96 - 31) // ****************
		}  // ***************************************
		if (keyspressed & KEY_F4) {  // 32-96, & 123-126 are valid ASCII character values // **************************************
			// Increase char ASCII decimal value   // *********************
			editName[cursorPos] = (editName[cursorPos] + 1);  // *********************
			if (editName[cursorPos] > 96)  // ****************
				editName[cursorPos] = editName[cursorPos] - 65; // wrap, (97 - 32) // ***********
		} // ***************************************
		if (keyspressed & KEY_S) {  // same renamed profile to RAM and EEPROM // ****************************
			Reflow_SetProfileName( editName, &ee4, &ee3 ); // sets in RAM and EEPROM // **************************
			cursorPos = 0; // **************************************
			initialized = 0; // reinit buffer next time // ***********************
			mode = MAIN_EDIT_PROFILE; // *****************************
		}   // **************************************
	}  // ******************************

	else if (mode == MAIN_EDIT_PROFILE_RESTORE_DEFAULTS) {  // *************************
		LCD_FB_Clear();  // ****************************

		len = snprintf(buf, sizeof(buf), "%s", Reflow_GetProfileName(&ee3)); // ***************
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), 0, FONT6X6 | INVERT); // *************
		LCD_disp_str((uint8_t*)"RESTORE DEFAULTS?", 17, 0, 8 * 2, FONT6X6);  // **********
		LCD_disp_str((uint8_t*)"F1", 2, 0, 8 * 4, FONT6X6 | INVERT);  // ****************
		LCD_disp_str((uint8_t*)"CONFIRM RESTORE", 15, 14, 8 * 4, FONT6X6);  // ****************
		LCD_disp_str((uint8_t*)"F2", 2, 0, 8 * 6, FONT6X6 | INVERT);  // **************
		LCD_disp_str((uint8_t*)"CANCEL", 6, 14, 8 * 6, FONT6X6);  // ************

		if (keyspressed & KEY_F1) {  // CONFIRM RESTORE  // ************************
			ee4 = paramProfiles[Reflow_GetProfileIdx()];  // *******************************
			Reflow_writeEE_profileParams(&ee4);  // ************
			Reflow_computeTempDatasetFromParams( &ee4, &ee3 );  // ***********************
			mode = MAIN_EDIT_PROFILE;  // ****************************
		}  // *********************************
		if (keyspressed & KEY_F2) { // CANCEL RESTORE  // **********************
			mode = MAIN_EDIT_PROFILE;  // ********************************
		}  // ****************************************
	}  // ********************************************

	else if (mode == MAIN_BAKE) {
		LCD_FB_Clear();
		LCD_disp_str((uint8_t*)"MANUAL/BAKE MODE", 16, 0, 0, FONT6X6);

		int keyrepeataccel = keyspressed >> 17; // Divide the value by 2
		if (keyrepeataccel < 1) keyrepeataccel = 1;
		if (keyrepeataccel > 30) keyrepeataccel = 30;

		// Setpoint-
		if (keyspressed & KEY_F1) {
			setpoint -= keyrepeataccel;
			if (setpoint < SETPOINT_MIN) setpoint = SETPOINT_MIN;
		}

		// Setpoint+
		if (keyspressed & KEY_F2) {
			setpoint += keyrepeataccel;
			if (setpoint > SETPOINT_MAX) setpoint = SETPOINT_MAX;
		}

		// timer --
		if (keyspressed & KEY_F3) {
			if (timer - keyrepeataccel < 0) {
				// infinite bake
					timer = -1;
			} else {
				timer -= keyrepeataccel;
			}
		}
		// timer ++
		if (keyspressed & KEY_F4) {
			timer += keyrepeataccel;
		}

		int y = 10;
		// display F1 button only if setpoint can be decreased
		char f1function = ' ';
		if (setpoint > SETPOINT_MIN) {
			LCD_disp_str((uint8_t*)"F1", 2, 0, y, FONT6X6 | INVERT);
			f1function = '-';
		}
		// display F2 button only if setpoint can be increased
		char f2function = ' ';
		if (setpoint < SETPOINT_MAX) {
			LCD_disp_str((uint8_t*)"F2", 2, LCD_ALIGN_RIGHT(2), y, FONT6X6 | INVERT);
			f2function = '+';
		}
		len = snprintf(buf, sizeof(buf), "%c SETPOINT %d` %c", f1function, (int)setpoint, f2function);
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), y, FONT6X6);


		y = 18;
		if (timer == 0) {
			len = snprintf(buf, sizeof(buf), "inf TIMER stop +");
		} else if (timer < 0) {
			len = snprintf(buf, sizeof(buf), "no timer    stop");
		} else {
			len = snprintf(buf, sizeof(buf), "- TIMER %3d:%02d +", timer / 60, timer % 60);
		}
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), y, FONT6X6);

		if (timer >= 0) {
			LCD_disp_str((uint8_t*)"F3", 2, 0, y, FONT6X6 | INVERT);
		}
		LCD_disp_str((uint8_t*)"F4", 2, LCD_ALIGN_RIGHT(2), y, FONT6X6 | INVERT);

		y = 26;
		if (timer > 0) {
			int time_left = Reflow_GetTimeLeft();
			if (Reflow_IsPreheating()) {
				len = snprintf(buf, sizeof(buf), "PREHEAT");
			} else if (Reflow_IsDone() || time_left < 0) {
				len = snprintf(buf, sizeof(buf), "DONE");
			} else {
				len = snprintf(buf, sizeof(buf), "%d:%02d", time_left / 60, time_left % 60);
			}
				LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_RIGHT(len), y, FONT6X6);
		}

		len = snprintf(buf, sizeof(buf), "ACT %3.1f`", Sensor_GetTemp(TC_AVERAGE));
		LCD_disp_str((uint8_t*)buf, len, 0, y, FONT6X6);

		y = 34;
		len = snprintf(buf, sizeof(buf), "  L %3.1f`", Sensor_GetTemp(TC_LEFT));
		LCD_disp_str((uint8_t*)buf, len, 0, y, FONT6X6);
		len = snprintf(buf, sizeof(buf), "  R %3.1f`", Sensor_GetTemp(TC_RIGHT));
		LCD_disp_str((uint8_t*)buf, len, LCD_CENTER, y, FONT6X6);

		if (Sensor_IsValid(TC_EXTRA1) || Sensor_IsValid(TC_EXTRA2)) {
			y = 42;
			if (Sensor_IsValid(TC_EXTRA1)) {
				len = snprintf(buf, sizeof(buf), " X1 %3.1f`", Sensor_GetTemp(TC_EXTRA1));
				LCD_disp_str((uint8_t*)buf, len, 0, y, FONT6X6);
			}
			if (Sensor_IsValid(TC_EXTRA2)) {
				len = snprintf(buf, sizeof(buf), " X2 %3.1f`", Sensor_GetTemp(TC_EXTRA2));
				LCD_disp_str((uint8_t*)buf, len, LCD_CENTER, y, FONT6X6);
			}
		}

		y = 50;
		len = snprintf(buf, sizeof(buf), "COLDJUNCTION");
		LCD_disp_str((uint8_t*)buf, len, 0, y, FONT6X6);

		y += 8;
		if (Sensor_IsValid(TC_COLD_JUNCTION)) {
			len = snprintf(buf, sizeof(buf), "%3.1f`", Sensor_GetTemp(TC_COLD_JUNCTION));
		} else {
			len = snprintf(buf, sizeof(buf), "NOT PRESENT");
		}
		LCD_disp_str((uint8_t*)buf, len, (12 * 6) - (len * 6), y, FONT6X6);

		LCD_BMPDisplay(stopbmp, 127 - 17, 0); // *************** 17 -> 16 **********

		Reflow_SetSetpoint(setpoint);

		if (timer > 0 && Reflow_IsDone()) {
			Buzzer_Beep(BUZZ_1KHZ, 255, TICKS_MS(100) * NV_GetConfig(REFLOW_BEEP_DONE_LEN));
			Reflow_SetBakeTimer(0);
			Reflow_SetMode(REFLOW_STANDBY);
		}

		if (keyspressed & KEY_F3 || keyspressed & KEY_F4) {
			if (timer == 0) {
				Reflow_SetMode(REFLOW_STANDBY);
			} else {
				if (timer == -1) {
					Reflow_SetBakeTimer(0);
				} else if (timer > 0) {
					Reflow_SetBakeTimer(timer);
					printf("\nSetting bake timer to %d\n", timer);
				}
				Reflow_SetMode(REFLOW_BAKE);
			}
		}

		// Abort bake
		if (keyspressed & KEY_S) {
			printf("\nEnd bake mode by keypress\n");

			mode = MAIN_HOME;
			Reflow_SetBakeTimer(0);
			Reflow_SetMode(REFLOW_STANDBY);
			retval = 0; // Force immediate refresh
		}
	}
	/*   //  ****** this MAIN_EDIT_PROFILE code block to be moved/replaced  *************************  
	} else if (mode == MAIN_EDIT_PROFILE) {
		LCD_FB_Clear();
		int keyrepeataccel = keyspressed >> 17; // Divide the value by 2
		if (keyrepeataccel < 1) keyrepeataccel = 1;
		if (keyrepeataccel > 30) keyrepeataccel = 30;
		
		int16_t cursetpoint;
		Reflow_SelectEEProfileIdx(current_edit_profile);
		if (keyspressed & KEY_F1 && profile_time_idx > 0) { // Prev time
			profile_time_idx--;
		}
		if (keyspressed & KEY_F2 && profile_time_idx < 47) { // Next time
			profile_time_idx++;
		}
		cursetpoint = Reflow_GetSetpointAtIdx(profile_time_idx);

		if (keyspressed & KEY_F3) { // Decrease setpoint
			cursetpoint -= keyrepeataccel;
		}
		if (keyspressed & KEY_F4) { // Increase setpoint
			cursetpoint += keyrepeataccel;
		}
		if (cursetpoint < 0) cursetpoint = 0;
		if (cursetpoint > SETPOINT_MAX) cursetpoint = SETPOINT_MAX;
		Reflow_SetSetpointAtIdx(profile_time_idx, cursetpoint);

		Reflow_PlotProfile(profile_time_idx);
		LCD_BMPDisplay(editbmp, 127 - 17, 0);

		len = snprintf(buf, sizeof(buf), "%02u0s %03u`", profile_time_idx, cursetpoint);
		LCD_disp_str((uint8_t*)buf, len, 13, 0, FONT6X6);

		// Done editing
		if (keyspressed & KEY_S) {
			Reflow_SaveEEProfile();
			mode = MAIN_HOME;
			retval = 0; // Force immediate refresh
		}
	}
	*/  // end  MAIN_EDIT_PROFILE code block to be moved/replaced  // ***********************
	
	else { // Main menu
		LCD_FB_Clear();

		len = snprintf(buf, sizeof(buf), "MAIN MENU" );
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), 6 * 0, FONT6X6);  // *************
		LCD_disp_str((uint8_t*)"F1", 2, 0, 8 * 1, FONT6X6 | INVERT);
		LCD_disp_str((uint8_t*)"ABOUT", 5, 14, 8 * 1, FONT6X6);
		LCD_disp_str((uint8_t*)"F2", 2, 0, 8 * 2, FONT6X6 | INVERT);
		LCD_disp_str((uint8_t*)"SETUP/CALIBRATION", 17, 14, 8 * 2, FONT6X6);  // ***************
		LCD_disp_str((uint8_t*)"F3", 2, 0, 8 * 3, FONT6X6 | INVERT);
		LCD_disp_str((uint8_t*)"BAKE/MANUAL MODE", 16, 14, 8 * 3, FONT6X6);
		LCD_disp_str((uint8_t*)"F4", 2, 0, 8 * 4, FONT6X6 | INVERT);
		LCD_disp_str((uint8_t*)"SELECT/EDIT PROFILE", 19, 14, 8 * 4, FONT6X6); // ****************
		LCD_disp_str((uint8_t*)"S", 1, 3, 8 * 5, FONT6X6 | INVERT);
		LCD_disp_str((uint8_t*)"RUN REFLOW PROFILE", 18, 14, 8 * 5, FONT6X6);

		len = snprintf(buf, sizeof(buf), "%s", Reflow_GetProfileName(&ee3));  // *************
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), 8 * 6, FONT6X6 | INVERT);

		len = snprintf(buf,sizeof(buf), "OVEN TEMPERATURE %d`", Reflow_GetActualTemp());
		LCD_disp_str((uint8_t*)buf, len, LCD_ALIGN_CENTER(len), (8 * 7) +1, FONT6X6);  // *************

		// Make sure reflow complete beep is silenced when pressing any key
		if (keyspressed) {
			Buzzer_Beep(BUZZ_NONE, 0, 0);
		}

		// About
		if (keyspressed & KEY_F1) {
			mode = MAIN_ABOUT;
			retval = 0; // Force immediate refresh
		}
		if (keyspressed & KEY_F2) { // Setup/cal
			mode = MAIN_SETUP;
			Reflow_SetMode(REFLOW_STANDBYFAN);
			retval = 0; // Force immediate refresh
		}

		// Bake mode
		if (keyspressed & KEY_F3) {
			mode = MAIN_BAKE;
			Reflow_Init();
			retval = 0; // Force immediate refresh
		}

		// Select profile
		if (keyspressed & KEY_F4) {
			mode = MAIN_SELECT_PROFILE;
			retval = 0; // Force immediate refresh
		}

		// Start reflow
		if (keyspressed & KEY_S) {
			mode = MAIN_REFLOW;
			LCD_FB_Clear();
			printf("\nStarting reflow with profile: %s", Reflow_GetProfileName(&ee3));  // *************
			Reflow_Init();
			Reflow_PlotProfile(-1);
			LCD_BMPDisplay(stopbmp, 127 - 16, 0);  // 17 -> 16  // *************
			len = snprintf(buf, sizeof(buf), "%s", Reflow_GetProfileName(&ee3)); // *************
			LCD_disp_str((uint8_t*)buf, len, 13, 0, FONT6X6);
			Reflow_SetMode(REFLOW_REFLOW);
			retval = 0; // Force immediate refresh
		}
	}

	LCD_FB_Update();

	return retval;
}
