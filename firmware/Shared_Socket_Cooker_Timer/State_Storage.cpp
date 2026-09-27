// SPDX-FileCopyrightText: Johannes Stockhammer
// SPDX-License-Identifier: Apache-2.0
/*
 * State_Storage.cpp
 *
 *  Author: Johannes Stockhammer
 *
 * Saves phase and remaining time in a ring buffer in the EEPROM (64 slots x 8 bytes).
 * Each save goes into the next slot with a running sequence number and a CRC-8.
 * The newest record is the valid slot whose next slot is invalid or does not have
 * sequence number +1. A slot half written at a power loss fails the CRC check,
 * then the record before it counts (at most 1 min older).
 * EEPROM 100,000 write cycles per cell: ~72 saves per hour spread over 64 slots
 * = ~1.1 writes per slot and hour -> ~89,000 h (~10 years) of continuous operation.
 */

//Includes
#include <avr/eeprom.h>
#include <util/crc16.h>
#include "State_Storage.h"

//Record: 8 bytes (AVR has no padding)
struct Record
{
	uint8_t marker;									//STORAGE_MARKER, otherwise slot empty
	uint16_t sequence;								//Running number, marks the newest slot
	uint16_t cycle;									//Running phase
	uint16_t time_left;								//Remaining time of the phase in ticks
	uint8_t checksum;								//CRC-8 over the 7 bytes before, written last
};

//Variables
uint8_t write_slot=0;								//Next slot to write
uint16_t sequence=0;								//Sequence number of the next record

//Prototypes
uint8_t Calc_Checksum(Record&record);
bool Record_Valid(Record&record);
void Read_Record(uint8_t slot, Record&record);

bool State_Load(uint16_t&cycle, uint16_t&time_left)
{
	Record record, next;
	uint8_t slot;

	for(slot=0;slot<STORAGE_SLOTS;slot++)
	{
		Read_Record(slot,record);
		if(!Record_Valid(record)) continue;								//Empty or half written
		Read_Record((slot+1)%STORAGE_SLOTS,next);
		if(Record_Valid(next)&&(next.sequence==(uint16_t)(record.sequence+1))) continue;	//There is a newer one

		cycle=record.cycle;											//Newest state found
		time_left=record.time_left;
		write_slot=(slot+1)%STORAGE_SLOTS;							//Write the slot after it next
		sequence=record.sequence+1;
		return 1;
	}//end for slot

	write_slot=0;													//EEPROM empty -> start at the front
	sequence=0;
	return 0;
}//end State_Load

void State_Save(uint16_t cycle, uint16_t time_left)
{
	Record record;

	record.marker=STORAGE_MARKER;
	record.sequence=sequence;
	record.cycle=cycle;
	record.time_left=time_left;
	record.checksum=Calc_Checksum(record);
	eeprom_update_block(&record,(void *)(STORAGE_START+write_slot*sizeof(Record)),sizeof(Record));	//Writes only changed bytes, ~8 x 3,4ms

	write_slot=(write_slot+1)%STORAGE_SLOTS;						//Next slot in the ring
	sequence++;
}//end State_Save

//----------------------------------------------------------//

uint8_t Calc_Checksum(Record&record)				//CRC-8 over all bytes except the checksum
{
	uint8_t *data=(uint8_t *)&record;
	uint8_t crc=0, k;

	for(k=0;k<sizeof(Record)-1;k++) crc=_crc8_ccitt_update(crc,data[k]);
	return crc;
}//end Calc_Checksum

bool Record_Valid(Record&record)					//Marker and CRC correct
{
	return (record.marker==STORAGE_MARKER)&&(record.checksum==Calc_Checksum(record));
}//end Record_Valid

void Read_Record(uint8_t slot, Record&record)
{
	eeprom_read_block(&record,(const void *)(STORAGE_START+slot*sizeof(Record)),sizeof(Record));
}//end Read_Record
