## Introduction
The current T-962 firmware uses a 256-byte AT24C02C I2C EEPROM to store configuration
data and custom reflow profiles. t962.h:7 

Here's the complete memory map:

## EEPROM Memory Layout (256 bytes total)

## Addresses 0x00-0x61 (98 bytes): Custom Profile #1

- 0x00-0x01: Magic bytes (0x56, 0x57) for profile validation t962.h:58-59

- 0x02-0x61: 48 temperature setpoints (16-bit big-endian values) reflow_profiles.c:99-100

## Addresses 0x62-0x7D (28 bytes): Setup Settings (NV Storage)

- 0x62-0x63: Magic word (0x574A = "WJ") nvstorage.c:27

- 0x64: Number of items stored nvstorage.c:31

- 0x65-0x7D: Configuration values (up to 25 items) nvstorage.h:4-5

## The setup settings stored include:

- REFLOW_BEEP_DONE_LEN: Completion beep duration

- REFLOW_PROFILE: Selected profile index
 
- TC_LEFT_GAIN/TC_LEFT_OFFSET: Left thermocouple calibration
 
- TC_RIGHT_GAIN/TC_RIGHT_OFFSET: Right thermocouple calibration
 
- REFLOW_MIN_FAN_SPEED: Minimum fan speed
 
- REFLOW_BAKE_SETPOINT_H/REFLOW_BAKE_SETPOINT_L: Bake temperature setpoint nvstorage.h:7-18
 
## Addresses 0x7E-0xFF (130 bytes): Custom Profile #2

- 0x80-0x81: Magic bytes (0x56, 0x57)

- 0x82-0xE1: 48 temperature setpoints (16-bit big-endian values) reflow_profiles.c:102-103

- 0xE2-0xFD: Unused space (28 bytes) t962.h:62-63

- 0xFE-0xFF: Magic bytes (0x58, 0x59) for profile validation t962.h:58-59

## Data Access
The system loads custom profiles at startup via Reflow_LoadCustomProfiles() reflow_profiles.c:98-104

and manages NV settings through NV_Init() which reads from address 0x62 nvstorage.c:45 .

Profile data is byte-swapped from big-endian storage format for internal use reflow_profiles.c:91-96 .

## Notes
The EEPROM layout is carefully designed to maintain backward compatibility with the original T-962 firmware,
which expects specific magic byte patterns for profile validation.

The NV storage system uses only 28 bytes total, with 3 bytes for housekeeping (magic + count)

and space for up to 25 configuration items.
__________________________________________________________________________


# Magic Number Validation 

The magic bytes in the T-962's custom profiles provide a validation mechanism to ensure profile data integrity
and maintain compatibility with the original firmware .

## Magic Byte Validation System
The custom profiles use a specific magic byte pattern for validation t962.h:58-59 . Each profile requires:

- Start magic bytes: 0x56, 0x57 at the beginning of each profile
- End magic bytes: 0x58, 0x59 at the end of each profile

## Profile Memory Layout with Magic Bytes

## Custom Profile #1 (addresses 0x00-0x61):

- 0x00-0x01: Magic bytes 0x56, 0x57
- 0x02-0x61: 48 temperature setpoints (96 bytes)

## Custom Profile #2 (addresses 0x7E-0xFF):

- 0x80-0x81: Magic bytes 0x56, 0x57
- 0x82-0xE1: 48 temperature setpoints (96 bytes)
- 0xFE-0xFF: Magic bytes 0x58, 0x59

## Validation Purpose
The magic bytes serve multiple purposes t962.h:58-63 :

1. Profile Recognition: The original T-962 software checks for these specific byte patterns to recognize valid custom profiles
2. Data Integrity: Ensures the EEPROM contains valid profile data rather than random or corrupted values
3. Backward Compatibility: Maintains compatibility with the original firmware's profile validation logic

## Profile Loading Process
When the system loads custom profiles, it reads the EEPROM data and performs byte-swapping from big-endian format reflow_profiles.c:98-104 .
The magic bytes are part of this data structure and would be validated by any firmware checking for proper profile format.

## Notes
The magic byte validation is a legacy requirement from the original T-962 firmware design. While the improved firmware loads profiles regardless, maintaining this format ensures compatibility and provides a basic integrity check for the stored temperature profile data.
