## Introduction
The current T-962 firmware uses a 256-byte AT24C02C I2C EEPROM to store configuration
data and custom reflow profiles. t962.h:7 

Here's the complete memory map:

## EEPROM Memory Layout (256 bytes total)

## Addresses 0x00-0x61 (98 bytes): Custom Profile #1

0x00-0x01: Magic bytes (0x56, 0x57) for profile validation t962.h:58-59
0x02-0x61: 48 temperature setpoints (16-bit big-endian values) reflow_profiles.c:99-100

## Addresses 0x62-0x7D (28 bytes): Setup Settings (NV Storage)

0x62-0x63: Magic word (0x574A = "WJ") nvstorage.c:27
0x64: Number of items stored nvstorage.c:31
0x65-0x7D: Configuration values (up to 25 items) nvstorage.h:4-5

## The setup settings stored include:

 REFLOW_BEEP_DONE_LEN: Completion beep duration
 REFLOW_PROFILE: Selected profile index
 TC_LEFT_GAIN/TC_LEFT_OFFSET: Left thermocouple calibration
 TC_RIGHT_GAIN/TC_RIGHT_OFFSET: Right thermocouple calibration
 REFLOW_MIN_FAN_SPEED: Minimum fan speed
 REFLOW_BAKE_SETPOINT_H/REFLOW_BAKE_SETPOINT_L: Bake temperature setpoint nvstorage.h:7-18
 
## Addresses 0x7E-0xFF (130 bytes): Custom Profile #2

0x80-0x81: Magic bytes (0x56, 0x57)
0x82-0xE1: 48 temperature setpoints (16-bit big-endian values) reflow_profiles.c:102-103
0xE2-0xFD: Unused space (28 bytes) t962.h:62-63
0xFE-0xFF: Magic bytes (0x58, 0x59) for profile validation t962.h:58-59

## Data Access
The system loads custom profiles at startup via Reflow_LoadCustomProfiles() reflow_profiles.c:98-104
and manages NV settings through NV_Init() which reads from address 0x62 nvstorage.c:45 .
Profile data is byte-swapped from big-endian storage format for internal use reflow_profiles.c:91-96 .

## Notes
The EEPROM layout is carefully designed to maintain backward compatibility with the original T-962 firmware,
which expects specific magic byte patterns for profile validation. The NV storage system uses only 28 bytes total,
with 3 bytes for housekeeping (magic + count) and space for up to 25 configuration items.
