/**
 * Copyright 2021 Marc SIBERT
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "bdos.h"

#include <iomanip>
// #include <fstream>

// #define LOG 1

/**
 * CP/M File Control Block
 * The File Control Block is a 36-byte data structure (33 bytes in CP/M 1).
 * ---
 * CR = current record,   ie (file pointer % 16384)  / 128
 * EX = current extent,   ie (file pointer % 524288) / 16384
 * S2 = extent high byte, ie (file pointer / 524288). The CP/M Plus source code refers to this use of the S2 byte as 'module number'.
 */
struct __attribute__ ((packed)) FCB_t {
// Drive. 0 for default, 1-16 for A-P. 
	uint8_t DR;
/**
 * Filename, 7-bit ASCII. The top bits of the filename bytes (usually referred to as F1' to F8') have the following meanings:
 *   F1'-F4' - User-defined attributes. Any program can use them in any way it likes. The filename in the disc directory has the corresponding bits set.
 *   F5'-F8' - Interface attributes. They modify the behaviour of various BDOS functions or indicate error conditions. In the directory these bits are always zero.
 */                        
	char filename[8];
/**
 * Filetype, 7-bit ASCII. T1' to T3' have the following meanings:
 *   T1' - Read-Only. 
 *   T2' - System (hidden). System files in user 0 can be opened from other user areas.
 *   T3' - Archive. Set if the file has not been changed since it was last copied.
 */					
	char filetype[3];
// Set this to 0 when opening a file and then leave it to CP/M. You can rewind a file by setting EX, RC, S2 and CR to 0. 
	uint8_t EX;
// Reserved.
	uint8_t S1;
// Reserved.
	uint8_t S2;
// Set this to 0 when opening a file and then leave it to CP/M.
	uint8_t RC;
// Image of the second half of the directory entry, containing the file's allocation (which disc blocks it owns).
	uint8_t AL;
// Current record within extent. It is usually best to set this to 0 immediately after a file has been opened and then ignore it.
	uint8_t CR;
// Random access record number (not CP/M 1). A 16-bit value in CP/M 2 (with R2 used for overflow); an 18-bit value in CP/M 3.
	uint16_t RN;
};




/**
 * BDOS functions.
 * C register contains the function value.
 * @see http://www.gaby.de/cpm/manuals/archive/cpm22htm/ch5.htm
 */	
void BDos::function(const unsigned char c) {

	std::clog << "DRIVE: " << int(drive) << std::endl;
	
	switch (c) {

		case 0x02 : consoleOutput(); break;
		case 0x09 : printString(); break;
		case 0x0A : readConsoleBuffer(); break;
		case 0x0B : getConsoleStatus(); break;
		case 0x0D : resetDiskSystem(); break;
		case 0x0E : selectDisk(); break;
		
		default:
			std::cerr << "Register C: " << std::hex << std::setw(2) << std::setfill('0') << unsigned(c) << "h";
			std::cerr << " : Unknown BDOS function!" << std::endl;
			
			throw std::runtime_error("Un-emulated BDOS function");
			break;
	}
}
