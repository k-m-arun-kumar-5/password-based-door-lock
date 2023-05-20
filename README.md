Description :
=============
I, K.M. Arun Kumar alias Arunkumar Murugeswaran, just shared my works, which I worked as learning path and practiced Embedded C programming using PIC18F4550, such as password based door open. Also has feature to configured password for door open. 

Program Description
===================
Electronic door security system. At first, default correct password = "123789" is used and enter the password, which is 6 digits. character '#' acts as backspace and '*' acts as char to configure authication password. 
1: when OPEN_SW is pressed, to allow access to keypad. password is entered.						  
2:  After successuful authentication, Unipolar stepper motor is rotated clockwise from 0 to 90 degrees, then door is opened. 
3:  When DOOR_CLOSE switch is pressed Unipolar stepper motor is rotated anticlockwise for 90 degrees to 0 degrees, then door is closed. 
4:  wait for OPEN_SW to allow access to keypad. 
5:  If first char enter in password is '*', which is invisible in LCD, then system goes into configure mode. Then at first password authication process begins and after successfully authication, set password is entered and then confirm password is entered. 
6: If set password and confirm password matches, then new authentication password is set and writtern to internal EEPROM. If set password and confirm password does not match (no limit), correct password does not change and it goes to Step 1. 
7: If door open authentication password is either wrong or timeout occurs, then numbr of retry is incremented by 1. If it exceeds MAX_RETRY, then microcontroller enters in sleep mode.
8: Timer1 is used for time delay by interrupt.  

CAUTION:
========
Schematics and simulation is done by Proteus CAD. NOT EXPERIMENTED IN REAL TIME ENVIRONMENT.

Purpose :
=========
In all my respective repositories, I just shared my works that I worked as the learning path and practiced, with designed, developed, implemented, simulated and tested, including some projects, assignments, documentations and all other related files and some programming that might not being implement, not being completed, lacks some features or have some bugs. Purpose of all my repositories, if used, can be used for LEARNING AND EDUCATIONAL PURPOSE ONLY. It can be used as the open source and freeware. Kindly read the LICENSE.txt for license, terms and conditions about the use of source codes, binaries, documentation and all other files, located in all my repositories. 

My Thanks and Tribute :
========================
I thank to my family, Friends, Teachers, People behind the toolchains and references that I used, all those who directly or indirectly supported me and/or helped me and/or my family, Nature and God. My tribute to my family, Friends, Teachers, People behind the toolchains and references that I used, Nature, Jimmy Dog, God and all those, who directly or indirectly help and/or support me and/or my family.

Toolchains that I used for PIC18F4550 Application design and development are as follows :
=========================================================================================
1: IDE and compiler for PIC18F4550                                           - Microchip's MPLAB X IDE (v3.45) with MPLAB XC8 compiler(v1.45) 
2: CAD and simulator for PIC18F4550                                          - Proteus 8.0 Professional, Proteus 8.3 Professional SP2, and/or Proteus 8.11 SPO.
3: Desktop Computer Architecture and OS for PIC18F4550 development           - Intel X64 & Windows 10 (64 bit).
4: Code editor                                                               - Notepad++.
5: Documentation                                                             - Microsoft Office 2007 (alternative LibreOffice) and Text Editor.

Some reference that I refered for PIC16F887  Application design and development, are as follows :
==================================================================================================
1: Schaum's Outline of Programming with C, 2nd Edition - Book authored by Byron Gottfried.
2: Understanding and Using C Pointers: Core Techniques for Memory Management - Book authored by Richard M. Reese. 
3: Embedded C - Book authored by Michael J. Pont.
4: Hitachi HD44780U - LCD product data sheet.
5: PIC18F4550 data sheet.

Note :
======
Kindly read in the source codes, if mentioned, about the Program Description or Purpose, Known Bugs, Caution and Notes and documentations. 

My Repositories Web Link :
==========================
https://github.com/k-m-arun-kumar-5

