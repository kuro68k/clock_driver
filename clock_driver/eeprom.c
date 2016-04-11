/*
 * eeprom.c
 *
 * Created: 11/04/2016 11:16:57
 *  Author: paul.qureshi
 *
 * EEPROM driver for XMEGA E5
 */ 

#include <string.h>
#include <avr/io.h>

#include "eeprom.h"

/**************************************************************************************************
** Wait for NVM access to finish.
*/
inline void EEP_WaitForNVM(void)
{
	while ((NVM.STATUS & NVM_NVMBUSY_bm) == NVM_NVMBUSY_bm)
		;
}

/**************************************************************************************************
** Write EEPROM page buffer to EEPROM memory. EEPROM will be erased before writing. Only page
** buffer locations that have been written will be written, others will remain untouched.
**
** page_addr should be between 0 and EEPROM_SIZE/EEPROM_PAGE_SIZE
*/
void EEP_AtomicWritePage(uint8_t page_addr)
{
	EEP_WaitForNVM();

	// Calculate page address
	uint16_t address = (uint16_t)(page_addr*EEPROM_PAGE_SIZE);

	// Set address
	NVM.ADDR0 = address & 0xFF;
	NVM.ADDR1 = (address >> 8) & 0x1F;
	NVM.ADDR2 = 0x00;

	// Issue EEPROM Atomic Write (Erase&Write) command
	NVM.CMD = NVM_CMD_ERASE_WRITE_EEPROM_PAGE_gc;
	NVM_EXEC();
	
	EEP_WaitForNVM();
}
