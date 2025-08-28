
#include <stdint.h>
#include <stdio.h>
#include "Wire.h"
#include "I2C_eeprom.h"

I2C_eeprom ee(0x50, I2C_DEVICESIZE_24LC02);

uint32_t start, duration;

#define NUMPROFILETEMPS (48)

typedef struct {  // ********************************
    char* name;  // ********************************
    uint16_t params[13]; // ********************************
} paramProfile;  // ********************************

typedef enum { // **********************************
  PREHEAT_START_TEMP,  // **********************************
  PREHEAT_START_TEMP_DWELL_TIME,  // **********************************
  PREHEAT_HEAT_RATE,  // **********************************
  SOAK_START_TEMP,  // **********************************
  SOAK_END_TARGET_TEMP,  // **********************************
  SOAK_DWELL_TIME,  // **********************************
  SOAK_HEAT_RATE,  // **********************************
  REFLOW_SOLDER_MELT_TEMP,  // **********************************
  REFLOW_OVERSHOOT_MELT_TEMP_DELTA,  // **********************************
  REFLOW_HEAT_RATE,  // **********************************
  REFLOW_DWELL_TIME_ABOVE_MELT_TEMP,  // **********************************
  COOLDOWN_RATE,  // **********************************
  COOLDOWN_END_TEMP  // **********************************
} profileParameters;  // **********************************

// Profile End Times
struct {
    float    preheat;
    float    soak;
    float    reflow;
    float    cooldown;
} endTime;  // End Times

typedef struct {
    char* name;
    uint16_t      temperatures[NUMPROFILETEMPS];
} ramprofile;


// Amtech 4300 63Sn/37Pb Leaded Profile
const paramProfile am4300_profile = {
    "4300 63SN/37PB",{  // Profile Name
    50,                 // Preheat Start Temp
    20,                 // Preheat Start Temp Dwell Time (sec)
    15, // 1.5          // Preheat Heat Rate (deg C/sec)
    140,                // Soak Start Temp (deg C)
    183,                // Soak End Target Temp (deg C)
    140,                // Soak Dwell Time (sec)
    3, // 0.3           // Soak Heat Rate (deg C/sec)
    183,                // Reflow Solder Melt Temp (deg C)
    33,                 // Reflow Overshoot Melt Temp Delta (deg C)
    13, // 1.3          // Reflow Heat Rate (deg C/sec)
    60,                 // Reflow Dwell Time Above Melt Temp (sec)
    15, // 1.5          // Cooldown Rate (deg C/sec)
    50                  // Cooldown End Temp (deg C)
  }
};

// NC-31 low-temp Lead-Free Profile
const paramProfile nc31_profile = {
    "NC-31 LOW-TEMP LF", { // Profile Name
    50,                    // Preheat Start Temp
    30,                    // Preheat Start Temp Dwell Time (sec)
    16, // 1.6             // Preheat Heat Rate (deg C/sec)
    85,                    // Soak Start Temp (deg C)
    127,                   // Soak End Target Temp (deg C)
    140,                   // Soak Dwell Time (sec)
    3, // 0.3              // Soak Heat Rate (deg C/sec)
    138,                   // Reflow Solder Melt Temp (deg C)
    21,                    // Reflow Overshoot Melt Temp Delta (deg C)
    13, // 1.3             // Reflow Heat Rate (deg C/sec)
    60,                    // Reflow Dwell Time Above Melt Temp (sec)
    8, // 0.8              // Cooldown Rate (deg C/sec)
    50                     // Cooldown End Temp (deg C)
  }
};

// SynTECH-LF Normal Temp Lead-Free Profile
const paramProfile syntechlf_profile = {
    "AMTECH SYNTECH-LF", { // Profile Name
    50,                    // Preheat Start Temp
    30,                    // Preheat Start Temp Dwell Time (sec)
    1,                     // Preheat Heat Rate (deg C/sec)
    85,                    // Soak Start Temp (deg C)
    220,                   // Soak End Target Temp (deg C)
    140,                   // Soak Dwell Time (sec)
    9, // 0.9              // Soak Heat Rate (deg C/sec)
    220,                   // Reflow Solder Melt Temp (deg C)
    25,                    // Reflow Overshoot Melt Temp Delta (deg C)
    13, // 1.3             // Reflow Heat Rate (deg C/sec)
    55,                    // Reflow Dwell Time Above Melt Temp (sec)
    13, // 1.3             // Cooldown Rate (deg C/sec)
    50                     // Cooldown End Temp (deg C)
  }
};

// Temperature Ramp Speed Test Profile
const paramProfile rampspeed_test_profile = {
    "RAMP SPEED TEST",{ // Profile Name
    50,                 // Preheat Start Temp
    30,                 // Preheat Start Temp Dwell Time (sec)
    250, // 25          // Preheat Heat Rate (deg C/sec)
    245,                // Soak Start Temp (deg C)
    245,                // Soak End Target Temp (deg C)
    210,                // Soak Dwell Time (sec)
    250, // 25          // Soak Heat Rate (deg C/sec)
    50,                 // Reflow Solder Melt Temp (deg C)
    0,                  // Reflow Overshoot Melt Temp Delta (deg C)
    250, // 25          // Reflow Heat Rate (deg C/sec)
    150,                // Reflow Dwell Time Above Melt Temp (sec)
    250, // 25          // Cooldown Rate (deg C/sec)
    50                  // Cooldown End Temp (deg C)
 }
};


// PID Gain Adjustment Test Profile (5% Setpoint Change)
const paramProfile pid_control_test_profile = {
    "PID CONTROL TEST",{ // Profile Name
    171,                 // Preheat Start Temp
    150,                 // Preheat Start Temp Dwell Time (sec)
    90,  // 9            // Preheat Heat Rate (deg C/sec)
    180,                 // Soak Start Temp (deg C)
    180,                 // Soak End Target Temp (deg C)
    80,                  // Soak Dwell Time (sec)
    90, // 9             // Soak Heat Rate (deg C/sec)
    171,                 // Reflow Solder Melt Temp (deg C)
    0,                   // Reflow Overshoot Melt Temp Delta (deg C)
    90,  // 9            // Reflow Heat Rate (deg C/sec)
    80,                  // Reflow Dwell Time Above Melt Temp (sec)
    90, // 9             // Cooldown Rate (deg C/sec)
    171                  // Cooldown End Temp (deg C)
  }
};

const paramProfile* paramProfiles[] = {
  &am4300_profile,
  &nc31_profile,
  &syntechlf_profile,
  &rampspeed_test_profile,
  &pid_control_test_profile
};

#define NUMPROFILES (sizeof(paramProfiles) / sizeof(paramProfiles[0]))

ramprofile ee1;

// current profile index
// static int 
static int16_t  profileidx = 0;

// **************************
paramProfile* ee2 = paramProfiles[profileidx]; 
uint16_t var1 = ee2->params[PREHEAT_START_TEMP];
// **************************

// ****************************************
char* tmpProfileName[19] = {"                  "};
//                           ******************   18 characters max
// ****************************************


float testCalc = 0;
// int 
int16_t Reflow_GetProfileIdx(void);
void Reflow_CalculateTempProfile( void );



void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  while (!Serial);      // wait for Serial to connect.
  ee.begin();
  if (! ee.isConnected())
  {
    Serial.println("ERROR: Can't find eeprom (stopped)...");
    // while (1);
  }


  
  // Convert profile parameters to ramprofile temperatures
  extern Reflow_GetProfileIdx(void);
  int16_t curprofile = Reflow_GetProfileIdx();

  ee2 = paramProfiles[curprofile];
  
  // ****************************************
  strcpy(*tmpProfileName, ee2->name );
  Serial.print( "tmpProfileName): ");
  Serial.println( *tmpProfileName );
  *tmpProfileName = "New Name";
  Serial.print( "tmpProfileName) revised: ");
  Serial.println( *tmpProfileName );
  
  // ****************************************
  
  Serial.print("Preheat StartTemp: ");
  Serial.print(var1);
  Serial.println();

  ee2->params[PREHEAT_START_TEMP] = 40;
  var1 = ee2->params[PREHEAT_START_TEMP];
  
  Serial.print("New Preheat StartTemp (var1): ");
  Serial.print(var1);
  Serial.println();
  Serial.println();
  // ***************************

  
  Serial.print("curprofile: ");
  Serial.println(curprofile);
  // ee2->name = "12345678901234567";  // testing size of ee2 written/read to/from EEPROM
  // int ee2currentByteSize = strlen(ee2->name) + 26;  // Test after testing EEwrite of ee2 and parmeter "cooldownEndTemp".
  Serial.print("ee2->name: ");
  Serial.println(ee2->name);
  // Serial.print("paramProfiles[curprofile]->name: ");
  // Serial.println(paramProfiles[curprofile]->name);
  Serial.println();

  Serial.print("Number of profiles ( NUMPROFILES ): ");
  Serial.println(NUMPROFILES);
  Serial.println();
  
  // ***********************
  int profileCount = 0;
  int addr = 0; // eeprom address to write to
  ee2->params[PREHEAT_START_TEMP] = 56;
  // for (int i = 0; i < NUMPROFILES; i++) {
    // switch(profileCount){
      // case 0: addr = 0x00; break;
      // case 1: addr = 0x2C; break;
      // case 2: addr = 0x6E; break;
      // case 3: addr = 0x9A; break;
      // case 4: addr = 0xC6; break;
    //}
    //ee2 = paramProfiles[profileCount];
    // start = micros();
    
    int len = strlen(ee2->name);
    Serial.print( "strlen(ee2->name): ");
    Serial.println( strlen(ee2->name) );
     
    ee.writeBlock(addr, (uint8_t *)&ee2, sizeof(*ee2) );
    Serial.print("length of strlen(ee2->name): ");
    Serial.println( strlen(ee2->name) );
    // Serial.print("sizeof(profileCount): ");
    // Serial.println(profileCount);
    // Serial.print("sizeof(*ee2): ");
    // Serial.println(sizeof(*ee2));
    //profileCount++;
    // *****************************
    // ee.updateBlock(addr, (uint8_t *)&ee2, (uint16_t) sizeof(*ee2) );
    // *****************************
    // duration = micros() - start;
    
    delay(15);
    //}
    
    ee2 = paramProfiles[curprofile];
    // Serial.print("write time (micro sec): \t");
    // Serial.println(duration);
    // Serial.println();

    ee2->params[PREHEAT_START_TEMP] = 0;  // clear value in RAM previously stored in EEPROM
    ee2->params[COOLDOWN_END_TEMP] = 140;  // was 50 before saved to EEPROM
    
    ee.readBlock(addr, (uint8_t *)&ee2, (uint16_t) sizeof(*ee2) );  // restore preheatStartTemp value 

    Serial.print("Post EE read, ee2->parameter[PREHEAT_START_TEMP] = ");
    Serial.println( ee2->params[PREHEAT_START_TEMP] );
    Serial.println();
    Serial.print("Profile: ");
    Serial.println(ee2->name);   // test of end of parameter range restored from EEPROM
    Serial.print("Cooldown End Temp: ");  // test of end of parameter range restored from EEPROM
    Serial.println( ee2->params[COOLDOWN_END_TEMP] );  // should be restored to 50

    
    // Serial.println("i: ");
    // Serial.print(i);
    // Serial.println();
    // profileCount++;
    // };  // for loop end
    // ***********************

  
    //*//
    // Print sizeof( );
    Serial.print("Profile Name: ");
    Serial.println(ee2->name);
    Serial.print("Sizeof( *ee2 ) (bytes): ");
    Serial.println( sizeof(*ee2) );
    Serial.println();
    
    // profileCount++;
  //} // end of... for loop
  //*/ //
    
 // Reflow_CalculateTempProfile();

  /*
  Serial.print("Preheat End Time: \t\t");
  Serial.println(endTime.preheat);
  Serial.print("Soak End Time: \t\t\t");
  Serial.println(endTime.soak);
  Serial.print("reflow End Time: \t\t");
  Serial.println(endTime.reflow);
  Serial.print("Cooldown End Time: \t\t");
  Serial.println(endTime.cooldown);
  Serial.println("\n");
  */ //

  /*
  // Print profile time/temperature dataset
  for (int16_t i=0; i < NUMPROFILETEMPS; ++i) {
    Serial.print("Time(sec):  \t");
    Serial.print( i * 10 );
    Serial.print("  Temperature(deg C):  \t");
    Serial.print(ee1.temperatures[i]);
    Serial.println();  
  } */ //
  
  
  Serial.println(".... done.");
}

void loop() {
  // put your main code here, to run repeatedly:

}

// Calculate time/temperature profile from profile parameters
void Reflow_CalculateTempProfile( void ) {
  // unsigned int 
  uint16_t temp;  // profile incremental temperatures
  float soakEndTemp = 0;
  const float multiplier = 0.1; // heat rate multiplier
  const int16_t inv = 480 / NUMPROFILETEMPS; // time intervals, based on 480 Second Heat Cycle (typ. 10 sec intervals)
  
  // Soak End Temp ... used in Reflow ET calc below
  soakEndTemp = (ee2->params[SOAK_START_TEMP]) + (ee2->params[SOAK_HEAT_RATE] * multiplier * ee2->params[SOAK_DWELL_TIME] );
  if ( soakEndTemp > ee2->params[SOAK_END_TARGET_TEMP] ){
        soakEndTemp = ee2->params[SOAK_END_TARGET_TEMP];
  }
  // Calculate Profile Phase End Times ( ETs )
  // Preheat
  endTime.preheat = ee2->params[SOAK_START_TEMP] - ( ( ee2->params[PREHEAT_HEAT_RATE] == 0 ) ? 0 : ( ee2->params[PREHEAT_START_TEMP] /
      ( ee2->params[PREHEAT_HEAT_RATE] * multiplier ) ) ) + ee2->params[PREHEAT_START_TEMP_DWELL_TIME];
  // Soak
  endTime.soak = endTime.preheat + ee2->params[SOAK_DWELL_TIME];
  // Reflow
  endTime.reflow = endTime.soak + ( ( ee2->params[REFLOW_HEAT_RATE] == 0 ) ? 0 : ( ( ee2->params[REFLOW_SOLDER_MELT_TEMP] - soakEndTemp ) /
    ( ee2->params[REFLOW_HEAT_RATE] * multiplier  ) ) ) + ee2->params[REFLOW_DWELL_TIME_ABOVE_MELT_TEMP];
  // Cooldown
  endTime.cooldown = endTime.reflow + ( ( ee2->params[COOLDOWN_RATE] == 0 ) ? 0 : ( ( ee2->params[REFLOW_SOLDER_MELT_TEMP] -
    ee2->params[COOLDOWN_END_TEMP] ) / ( ee2->params[COOLDOWN_RATE] * multiplier ) ) );
 
  //  Compute profile incremental temperatures
  float previousTemp = 0;
  
  for ( int16_t i = 0; i < NUMPROFILETEMPS; ++i ) {  // 0 - 47 temperatures is normal #
    int16_t t = i * inv;  // t is current time interval
    // Preheat
    if( t <= ee2->params[PREHEAT_START_TEMP_DWELL_TIME] )
      temp = ee2->params[PREHEAT_START_TEMP];
    else if( t <= endTime.preheat ) {
      temp = previousTemp + ( ee2->params[PREHEAT_HEAT_RATE] * multiplier * inv );
      if( temp > ee2->params[SOAK_START_TEMP] )
        temp = ee2->params[SOAK_START_TEMP];
    }
    // Soak
    else if( t <= endTime.soak ) {
      temp = previousTemp + ( ee2->params[SOAK_HEAT_RATE] * multiplier * inv );
      if ( temp > ee2->params[SOAK_END_TARGET_TEMP] )
        temp = ee2->params[SOAK_END_TARGET_TEMP];
    }
    // Reflow
    else if( t <= endTime.reflow - ( ( ee2->params[REFLOW_HEAT_RATE] == 0 ) ? 0 : ( ee2->params[REFLOW_OVERSHOOT_MELT_TEMP_DELTA] /
      ( ee2->params[REFLOW_HEAT_RATE] * multiplier ) ) ) ) {
          temp = previousTemp + ( ee2->params[REFLOW_HEAT_RATE] * multiplier * inv );
          if( temp > ee2->params[REFLOW_SOLDER_MELT_TEMP] + ee2->params[REFLOW_OVERSHOOT_MELT_TEMP_DELTA] )
            temp = ee2->params[REFLOW_SOLDER_MELT_TEMP] + ee2->params[REFLOW_OVERSHOOT_MELT_TEMP_DELTA];
          else if( t <= endTime.reflow ) 
            temp = previousTemp - ( ee2->params[REFLOW_HEAT_RATE] * multiplier * inv );
    }
    // Cooldown
    else if( t <= endTime.cooldown ) {
      temp = previousTemp - ( ee2->params[COOLDOWN_RATE] * multiplier * inv );
      if ( temp < ee2->params[COOLDOWN_END_TEMP] )
          temp = ee2->params[COOLDOWN_END_TEMP];
    }
    else temp = 0;  // ends profile and places oven in standby mode.

    ee1.temperatures[i] = temp;
    previousTemp = temp;  // save prevous temp for next loop
  }
}

int16_t Reflow_GetProfileIdx(void) {
  return profileidx;
}
