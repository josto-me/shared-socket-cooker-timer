// SPDX-FileCopyrightText: Johannes Stockhammer
// SPDX-License-Identifier: Apache-2.0
/*
 * State_Storage.h
 *
 *  Author: Johannes Stockhammer
 */


#ifndef STATE_STORAGE_H_
#define STATE_STORAGE_H_

#include <stdint.h>
#define STORAGE_START 0								//First EEPROM address of the ring buffer
#define STORAGE_SLOTS 64							//64 records x 8 bytes = 512 bytes
#define STORAGE_MARKER 0xB5							//Marker byte, erased EEPROM (0xFF) is invalid
bool State_Load(uint16_t&cycle, uint16_t&time_left);
void State_Save(uint16_t cycle, uint16_t time_left);

#endif /* STATE_STORAGE_H_ */
