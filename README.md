A T962 Parametric Profiles Firmware Mod Project, Proof of Concept, Plan of Actions, and Other Support Files Archive

## Introduction
This project’s goal is to provide a firmware modification to public domain project **UnifiedEngineering/T-962-improvements** github repo https://github.com/UnifiedEngineering/T-962-improvements, supporting a parametric based approach to creating and modifying PCB solder reflow profiles.  When completed,  the user should be capable of more easily adjusting all reflow profiles, phases, and shapes. This ability would allow for compensating variations in circuit board reflow requirements, including PCB thermal inertia, flux activation dwell times, and variations in solder flow characteristics.



## Existing Considerations
Currently, **T-962-improvements** firmware (v0.5.2 of 4 Feb 2018) supports seven reflow profiles, however five of these profiles reside in program memory, and only two profiles (i.e., Custom #1 & Custom #2), if created by user, can be saved to non-volatile memory (**EEPROM** storage).  Custom profiles require manually adjusting up to 48 target profile time/temperatures in 10 second intervals. Adjustments are made by either the oven’s control panel push button display, or alternatively, via a serial port (modification is required for this option).

The onboard **EEPROM** used by the T962 reflow oven is IC p/n **24C02** and consists of 256 bytes of storage.  A small portion of this non-volatile storage space is reserved for “NVMAGIC” firmware identification (2 bytes), and for Oven Setup/Calibration settings (9 bytes).  The remaining space (245 bytes), when using the current firmware, limits storage to two custom profiles.   Implementing parametric based profiles along with the use of value constraint ( min value, max value, multiplier value, offset value) stored in program memory, can improve the storage capacity to five reflow profiles, however if a 9-bit encoding is used, the number of stored profiles can be increased to 7 profiles.



## Supporting Files, Links, Documentation

A **DeepWiki** - https://deepwiki.com/UnifiedEngineering/T-962-improvements - has been created for **T-962-improvements** that may help with understanding the existing code structure and organization. Here’s a link to the 
 https://deepwiki.com/UnifiedEngineering/T-962-improvements

**Arduino Sketch Files** – https://github.com/borland1/T962ParametricProfilesDevelopFiles/tree/main/Arduino - Two Arduino sketches were developed to code the structures of profile parameters, calculating the time/temperature dataset, and read/write parameters for each profile non-volatile storage in **EEPROM**.

**Excel Files** – https://github.com/borland1/T962ParametricProfilesDevelopFiles/tree/main/MS%20Excel%20xlsx - Files detailing..
  - Programmatic structure of reflow profile time/temperature dataset values using input parameter values,  
	
  - LCD menu structure for profile editing and restoring firmware default values
  
  - Details of T962 controller (NXP LPC2134) pin remapping (LCD Panel Push Buttons F1, F2, and F3 in Keypad.c file) to allow use OLIMEX Development Board (LPC-P2148) available I/O pins.

**Image Files** – https://github.com/borland1/T962ParametricProfilesDevelopFiles/tree/main/Images - Some firmware Image files were edited to conform with new menu structures. File names were revised to show different version of originals.  Files were edited using MS Paint and saved as **Monochrome Bitmap BMP** (128x64px, 18x64px, or 18x16px) files. 



## Development Constraints

With the current **T-962-improvements** firmware, the boot up sequence first loads from EEPROM and checks the firmware version (**NVMAGIC**), then loads from **EEPROM**, the Setup/Calibration settings and last selected profile, and then loads from **EEPROM** the Custom profiles into RAM ( **ramprofiles ee1** & **ee2**).  With profiles instead being stored as parameters, the RAM size requirements should be reduced. Since only one reflow profile can be selected at any time, it is unnecessary to load from EEPROM or re-construction all reflow profiles.  More **EEPROM** reads would only be necessary when a different profile is selected.  Since **I2C EEPROM** reads are generally slow, boot time should be improved.

Code development on **PlatformIO** (**VS Code**) IDE compiles for **LPC-2134** in a HEX file, which is then used to flash a devlopment board https://www.olimex.com/Products/ARM/NXP/LPC-P2148/ for testing, using a programmer for flash based microprocessors from NXP, called **Flash Magic**  https://www.flashmagictool.com/.  **Flash Magic** does report a conflict when trying to flash a HEX file complied for LCP2134 on a LPC2148, however user can complete the flashing process with **Flash Magic**'s “ignore” prompt. Since the LCP2148 processor has more flash space than the LCP2134 processor, there is no problems with running programs with a smaller footprint.

Developing code on **PlatformIO (VS Code)** IDE, on local source files, required modifing for successful compile using the **maxgerhardt/T-962-improvements** repo on github. https://github.com/maxgerhardt/T-962-improvements

## Development Approach

1.	Multiple branches in the forked github repo here https://github.com/borland1/T962ParametricProfilesDevelopFiles are anticipated. These branches will be to allow adding, and later subtracting, code features only associated with PlatformIO IDE (**maxgerhardt/T-962-improvements**) and the LPC-P2148 development board (pin re-mapping).  A “final branch” in this forked repo will allow for a clean Pull Request to the **UnifiedEngineering/T-962-improvements** repo.

2.	Develop code for conversion of program memory from profiles based on time/temperature dataset, to profile parameters using different C library structures.  It may be more efficient to calculate the selected profile time/temperatures dataset and store in RAM memory, than to calculate upon selecting the “Select Profile” or “Run Profile” main menu options.

3.	New menus will need to be coded to provide user ability to edit/set parameter valves or restore firmware default values from the LCD control panel buttons.  Menu for "**edit profile parameters**" may be all text based, with multi-page, or scrolling single page. A preferred format for this "**edit profile parameters**" menu would be graphic profile plot, with parameter scrolling in some area of the limited LCD screen.

4.	Test for successful functionality being tested on the development board, use Git to commit changes to this forked repo.

5.	After fully developed, the plan is to submit a Pull Request to the  **UnifiedEngineering/T-962-improvements**.




