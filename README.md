Description :
=============
password-based door open. Also has a feature to configure password for door to open, after successful authentication. 

Program Description
===================
Electronic door security system. At first, default correct password = "123789" is used, and enter the password, which is 6 digits. character '#' acts as backspace and '*' acts as char to configure the authentication password. 
1: when OPEN_SW is pressed, to allow access to the keypad. password is entered.						  
2:  After successful authentication, Unipolar stepper motor is rotated clockwise from 0 to 90 degrees, then the door is opened. 
3:  When DOOR_CLOSE switch is pressed Unipolar stepper motor is rotated anticlockwise for 90 degrees to 0 degrees, then door is closed. 
4:  wait for OPEN_SW to allow access to keypad. 
5:  If first char enter in password is '*', which is invisible in LCD, then system goes into configure mode. Then at first password authentication process begins and after successfully authication, set password is entered and then confirm password is entered. 
6: If set password and confirm password matches, then new authentication password is set and written to internal EEPROM. If set password and confirm password does not match (no limit), correct password does not change and it goes to Step 1. 
7: If door open authentication password is either wrong or timeout occurs, then number of retry is incremented by 1. If it exceeds MAX_RETRY, then the microcontroller enters sleep mode.
8: Timer1 is used for time delay by interrupt.  

CAUTION:
========
Schematics and simulation is done by Proteus CAD. NOT EXPERIMENTED IN REAL TIME ENVIRONMENT.

Purpose :
=========
In all my respective repositories, I just shared my works that I worked as the learning path and practiced, with designed, developed, implemented, simulated and tested, including some projects, assignments, documentations and all other related files and some programming that might not being implement, not being completed, lacks some features or have some bugs. Purpose of all my repositories, if used, can be used for EDUCATIONAL PURPOSE ONLY. It can be used as the open source and freeware. Kindly read the LICENSE.txt for license, terms and conditions about the use of source codes, binaries, documentation and all other files, located in all my repositories. 
