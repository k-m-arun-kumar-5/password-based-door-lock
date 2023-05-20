#include <stdio.h>
#include <stdlib.h>



void EEPROM_Write(int,char);        /* Write byte to EEPROM data memory */
char EEPROM_Read(int);              /* Read Byte From EEPROM data memory */
void EEPROM_WriteString(int,char*); /* Write byte to EEPROM data memory */