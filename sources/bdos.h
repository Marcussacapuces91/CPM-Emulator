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

#pragma once

#include "Z80.h"

#include <dirent.h>
#include <sys/stat.h>
#include <cstring>
#include <iostream>
#include <iomanip>

#define LOG 1

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



class BDos {
public:

/**
 * BDOS functions.
 * C register contains the function value.
 * @see http://www.gaby.de/cpm/manuals/archive/cpm22htm/ch5.htm
 */	
void function(ZZ80State& state, uint8_t *const memory) {
	assert(memory);

	std::clog << "DRIVE: " << int(drive) << " - ";
	
	switch (state.Z_Z80_STATE_MEMBER_C) {
		case 0x01 : consoleInput(state); break;
		case 0x02 : consoleOutput(state); break;
		case 0x09 : printString(state, memory); break;
		case 0x0A : readConsoleBuffer(state, memory); break;
		case 0x0B : getConsoleStatus(state); break;
		case 0x0D : resetDiskSystem(state); break;
		case 0x0E : selectDisk(state); break;
		case 0x0F : openFile(state, memory); break;
		case 0x11 : searchForFirst(state, memory); break;
		case 0x12 : searchForNext(state, memory); break;
		case 0x19 : returnCurrentDisk(state); break;
		case 0x1A : setDMAAddress(state); break;
		case 0x20 : setGetUserCode(state); break;
		
		default:
			std::cerr << "Register C: " << std::hex << std::setw(2) << std::setfill('0') << unsigned(state.Z_Z80_STATE_MEMBER_C) << "h";
			std::cerr << " : Unknown BDOS function!" << std::endl;
			
			throw std::runtime_error("Un-emulated BDOS function");
			break;
	}
}

protected:
	inline
	void returnCode(ZZ80State& state, const uint16_t val) const  {
		state.Z_Z80_STATE_MEMBER_HL = val;
		state.Z_Z80_STATE_MEMBER_A = val & 0x00FF;
		state.Z_Z80_STATE_MEMBER_B = val >> 8;
	}
	
	void systemReset();
	
	void consoleInput(ZZ80State& state) {
		const auto c = std::cin.get();
		returnCode(state, c);
	}

/**
 * BDOS function 2 (C_WRITE) - Console output
 * Supported by: All versions
 * Entered with C=2, E=ASCII character.
 * Send the character in E to the screen. Tabs are expanded to spaces. Output can be paused with ^S and restarted with ^Q (or any key under versions prior to CP/M 3). While the output is paused, the program can be terminated with ^C.
 */
	void consoleOutput(ZZ80State& state) {
 #ifdef LOG
		std::clog << "Write console ASCII " << unsigned(state.Z_Z80_STATE_MEMBER_E) << " (";
		switch(state.Z_Z80_STATE_MEMBER_E) {
			case 0x00 : std::clog << "NUL"; break;
			case 0x0a : std::clog << "LF"; break;
			case 0x0d : std::clog << "CR"; break;
			default:
				std::clog << char(state.Z_Z80_STATE_MEMBER_E);
				break;
		}
		std::clog << ")" <<  std::endl;
#endif
		if (state.Z_Z80_STATE_MEMBER_E) std::cout << state.Z_Z80_STATE_MEMBER_E;
		returnCode(state, 0);
	}		
	
	void readerInput();
	
	void punchOutput();
	
	void listOutput();
	
	void directConsoleIO();
	
	void getIOByte();
	
	void setIOByte();
	
/**
 * BDOS function 9 (C_WRITESTR) - Output string
 * Supported by: All versions
 * Entered with C=9, DE=address of string.
 * Display a string of ASCII characters, terminated with the $ character. Thus the string may not contain $ characters - so, for example, the VT52 cursor positioning command ESC Y y+32 x+32 will not be able to use row 4.
 * Under CP/M 3 and above, the terminating character can be changed using BDOS function 110.
 */
	void printString(ZZ80State& state, const uint8_t *const memory) const {
#if LOG
		std::clog << "Output string (Buffer " << std::hex << state.Z_Z80_STATE_MEMBER_DE << "h)" << std::endl;
#endif
		const auto* c = &memory[state.Z_Z80_STATE_MEMBER_DE];
		while (*c != '$') {
			std::cout << char(*(c++));
		}
		returnCode(state, 0);
	}
	
/**
 * BDOS function 10 (C_READSTR) - Buffered console input
 * Supported by: All versions, with variations
 * Entered with C=0Ah, DE=address or zero.
 * This function reads characters from the keyboard into a memory buffer until RETURN is pressed. The Delete key is handled correctly. In later versions, more features can be used at this point; ZPM3 includes a full line editor with recall of previous lines typed.
 * On entry, DE is the address of a buffer. If DE=0 (in CP/M-86 versions DX=0FFFFh), the DMA address is used (CP/M 3 and later) and the buffer already contains data:
 * DE=address:                 DE=0 / DX=0FFFFh:
 * buffer: DEFB    size        buffer: DEFB    size
 *         DEFB    ?                   DEFB    len
 *         DEFB    bytes               DEFB    bytes
 * The value at buffer+0 is the amount of bytes available in the buffer. Once the limit has been reached, no more can be added, although the line editor can still be used.
 * If DE=0 (in 16-bit versions, DX=0FFFFh) the next byte contains the number of bytes already in the buffer; otherwise this is ignored. On return from the function, it contains the number of bytes present in the buffer.
 * The bytes typed then follow. There is no end marker.
 */
	void readConsoleBuffer(ZZ80State& state, uint8_t *const memory) {
#if LOG
		std::clog << "Buffered console input (Buffer " << std::hex << state.Z_Z80_STATE_MEMBER_DE << "h)" << std::endl;
//			std::clog << "mx" << unsigned(memory[DE+0]) << std::endl;
//			std::clog << "nc" << unsigned(memory[DE+1]) << std::endl;
#endif			
		std::string line;
		std::getline(std::cin, line);
		line = line.substr(0, memory[state.Z_Z80_STATE_MEMBER_DE]);

		memory[state.Z_Z80_STATE_MEMBER_DE + 1] = line.length();
		auto *s = memory + state.Z_Z80_STATE_MEMBER_DE + 2;
		for (auto i = line.begin(); i != line.end(); ++i) {
			*s++ = *i;
		}
		returnCode(state, 0);
	}

/**
 * BDOS function 11 (C_STAT) - Console status
 * Supported by: All versions
 * Entered with C=0Bh. Returns A=L=status
 * Returns A=0 if no characters are waiting, nonzero if a character is waiting.
 */
	void getConsoleStatus(ZZ80State& state) {
#if LOG
		std::clog << "Console status" << std::endl;
#endif
		char c;
		const auto n = std::cin.readsome(&c, 1);
		if (!n) {
			returnCode(state, 0x00);
		} else {
			std::cin.putback(c);
			returnCode(state, 0xFF);
		}
	}
	
	void returnVersionNumber(ZZ80State& state) {
#if LOG
		std::clog << "Version number" << std::endl;
#endif
		returnCode(state, 0x0022);
	};
	
/**
 * BDOS function 13 (DRV_ALLRESET) - Reset discs
 * Supported by: All versions.
 * Entered with C=0Dh. Returned values vary.
 * Resets disc drives. Logs out all discs and empties disc buffers. Sets the currently selected drive to A:. Any drives set to Read-Only in software become Read-Write; replacement BDOSses tend to leave them Read-Only.
 * In versions 1 and 2, logs in drive A: and returns 0FFh if there is a file present whose name begins with a $, otherwise 0. Replacement BDOSses may modify this behaviour.
 * In multitasking versions, returns 0 if succeeded, or 0FFh if other processes have files open on removable or read-only drives. 
 */
	void resetDiskSystem(ZZ80State& state) {
#if LOG
		std::clog << "Reset drive ; default to A" << std::endl;
#endif
		drive = 0;
		dma = 0x80;
		returnCode(state, 0);
	}
		
/**
 * BDOS function 14 (DRV_SET) - Select disc
 * Supported by: All versions
 * Entered with C=0Eh, E=drive number. Returns L=A=0 or 0FFh.
 * The drive number passed to this routine is 0 for A:, 1 for B: up to 15 for P:.
 * Sets the currently selected drive to the drive in A; logs in the disc. Returns 0 if successful or 0FFh if error. Under MP/M II and later versions, H can contain a physical error number. 
 */
 	void selectDisk(ZZ80State& state) {
		if (state.Z_Z80_STATE_MEMBER_E <= 15) {
#if LOG
			std::clog << "Select disc to " << char('A' + state.Z_Z80_STATE_MEMBER_E) << std::endl;
#endif
			const char dir[2] = { char('A' + state.Z_Z80_STATE_MEMBER_E), '\0' };
			struct stat st;
			const int err = stat(dir, &st);
			if (!err) {
				drive = state.Z_Z80_STATE_MEMBER_E;
				returnCode(state, 0);
				return;
			}
			state.Z_Z80_STATE_MEMBER_H = errno;
			std::cerr << "Error selecting disk '" << char('A' + state.Z_Z80_STATE_MEMBER_E) << ":' " << strerror(errno) << "!" << std::endl;
		} else {
			std::cerr << "Invalid disk (A-P only)!" << std::endl;
		}
		returnCode(state, 0xFF);
	}
	
/**
 * BDOS function 15 (F_OPEN) - Open file
 * Supported by: All versions
 * Entered with C=0Fh, DE=FCB address. Returns error codes in BA and HL.
 * This function opens a file to read or read/write. The FCB is a 36-byte data structure, most of which is maintained by CP/M. Look here for details.
 * The FCB should have its DR, Fn and Tn fields filled in, and the four fields EX, S1, S2 and RC set to zero. Under CP/M 3 and later, if CR is set to 0FFh then on return CR will contain the last record byte count. Note that CR should normally be reset to zero if sequential access is to be used.
 * Under MP/M II, the file is normally opened exclusively - no other process can access it. Two bits in the FCB control the mode the file is opened in:
 *  * F5' - set to 1 for "unlocked" mode - other programs can use the file.
 *  * F6' - set to 1 to open the file in read-only mode - other programs can use the file read-only. If both F6' and F5' are set, F6' wins.
 * If the file is opened in "unlocked" mode, the file's identifier (used for record locking) will be returned at FCB+21h.
 * Under MP/M II and later versions, a password can be supplied to this function by pointing the DMA address at the password.
 * On return from this function, A is 0FFh for error, or 0-3 for success. Some versions (including CP/M 3) always return zero; others return 0-3 to indicate that an image of the directory entry is to be found at (80h+20h*A).
 * If A=0FFh, CP/M 3 returns a hardware error in H and B. It also sets some bits in the FCB:
 *  * F7' is set if the file is read-only because writing is password protected and no password was supplied;
 *  * F8' is set if the file is read-only because it is a User 0 system file opened from another user area.
 */
	void openFile(ZZ80State& state, uint8_t *const memory) {
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
		std::clog << "Open file (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
//		std::cerr << "Open file " << pFCB->filename << '.' << pFCB->filetype << std::endl;
		returnCode(state, 0xFF);
	}

/**
 * BDOS function 16 (F_CLOSE) - Close file
 * Supported by: All versions
 * Entered with C=10h, DE=FCB address. Returns error codes in BA and HL.
 * This function closes a file, and writes any pending data. This function should always be used when a file has been written to.
 * On return from this function, A is 0FFh for error, or 0-3 for success. Some versions always return zero; others return 0-3 to indicate that an image of the directory entry is to be found at (80h+20h*A).
 * Under CP/M 3, if F5' is set to 1 then the pending data are written and the file is made consistent, but it remains open.
 * If A=0FFh, CP/M 3 returns a hardware error in H and B.
 */
/*
		case 0x10 : {
			std::clog << "BDOS - Close file " << std::hex << unsigned(DE) << "h" << std::endl;
			std::clog << "!! Always success !!" << std::endl;
			A = 0;
			break;
		 }
*/

/**
 * BDOS function 17 (F_SFIRST) - search for first
 * Supported by: All versions
 * Entered with C=11h, DE=address of FCB. Returns error codes in BA and HL.
 * Search for the first occurrence of the specified file; the filename should be stored in the supplied FCB. The filename can include ? marks, which match anything on disc. If the first byte of the FCB is ?, then any directory entry (including disc labels, date stamps etc.) will match. The EX byte is also checked; normally it should be set to zero, but if it is set to ? then all suitable extents are matched.
 * Returns A=0FFh if error (CP/M 3 returns a hardware error in H and B), or A=0-3 if successful. The value returned can be used to calculate the address of a memory image of the directory entry; it is to be found at DMA+A*32.
 * Under CP/M-86 v4, if the first byte of the FCB is '?' or bit 7 of the byte is set, subdirectories as well as files will be returned by this search.
 */
	void searchForFirst(ZZ80State& state, uint8_t *const memory) {
		const FCB_t *const pFCB = reinterpret_cast<const FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
		const char dir[2] = { char('A' + (pFCB->DR ? pFCB->DR-1 : drive)), '\0' };
#if LOG
		std::clog << "Search for first (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif

// Init filter
		memcpy(filter, pFCB->filename, 11);
		filter[11] = '\0';
		
		pDir = opendir(dir);
		if (pDir == NULL) {
			std::cerr << "Can't open local dir /" << dir << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}

		char filename[12];
		if (findFile(pDir, filter, filename)) {
			memory[dma] = pFCB->DR;
			memcpy(memory + dma + 1, filename, 11);
			returnCode(state, 0x00);	// OK
		} else {
			returnCode(state, 0xFF);	// KO
			closedir(pDir);
			pDir = NULL;
		}
	}

/**
 * BDOS function 18 (F_SNEXT) - search for next
 * Supported by: All versions
 * Entered with C=12h, (DE=address of FCB)?. Returns error codes in BA and HL.
 * This function should only be executed immediately after function 17 or another invocation of function 18. No other disc access functions should have been used.
 * Function 18 behaves exactly as number 17, but finds the next occurrence of the specified file after the one returned last time. The FCB parameter is not documented, but Jim Lopushinsky states in LD301.DOC:
 * In none of the official Programmer's Guides for any version of CP/M does it say that an FCB is required for Search Next (function 18). However, if the FCB passed to Search First contains an unambiguous file reference (i.e. no question marks), then the Search Next function requires an FCB passed in reg DE (for CP/M-80) or DX (for CP/M-86).
 */
	void searchForNext(ZZ80State& state, uint8_t *const memory) {
		
		
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
		std::clog << "Search for next (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
		if (pDir == NULL) {
			std::cerr << "No search for first!" << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}
		
		char filename[12];
		if (findFile(pDir, filter, filename)) {
			memory[dma] = pFCB->DR;
			memcpy(memory + dma + 1, filename, 11);
			returnCode(state, 0x00);	// OK
		} else {
			returnCode(state, 0xFF);	// KO
			closedir(pDir);
			pDir = NULL;
		}
	}

/**
 * BDOS function 20 (F_READ) - read next record
 * Entered with C=14h, DE=address of FCB. Returns error codes in BA and HL.
 * Supported by: All versions
 * Load a record (normally 128 bytes, but under CP/M 3 this can be a multiple of 128 bytes) at the previously specified DMA address. Values returned in A are:
 *   0 OK,
 *   1 end of file,
 *   9 invalid FCB,
 *  10 (CP/M) media changed; (MP/M) FCB checksum error,
 *  11 (MP/M) unlocked file verification error,
 *  0FFh hardware error.
 * If on return A is not 0FFh, H contains the number of 128-byte records read before the error (MP/M II and later).
 */
 /*
		case 0x14 : {
			FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
			std::clog << "Read next record (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#if LOG
			std::clog << "Read next record" << std::endl;
#endif
			state.Z_Z80_STATE_MEMBER_A = 1;
			break;
		}
*/

/**
 * BDOS function 25 (DRV_GET) - Return current drive
 * Supported by: All versions
 * Entered with C=19h. Returns drive in A. Returns currently selected drive. 0 => A:, 1 => B: etc.
 */
 	void returnCurrentDisk(ZZ80State& state) {
#if LOG
		std::clog << "Get drive (" << drive << ')' << std::endl;
#endif
		returnCode(state, drive);	// ok - drive numb.
	}

/**
 * BDOS function 26 (F_DMAOFF) - Set DMA address
 * Supported by: All versions
 * Entered with C=1Ah, DE=address.
 * Set the Direct Memory Access address; a pointer to where CP/M should read or write data. Initially used for the transfer of 128-byte records between memory and disc, but over the years has gained many more functions.
 */
 	void setDMAAddress(ZZ80State& state) {
#if LOG
		std::clog << "Set DMA address  (" << std::hex << state.Z_Z80_STATE_MEMBER_DE << ')' << std::endl;
#endif
		dma = state.Z_Z80_STATE_MEMBER_DE;
		returnCode(state, 0);	// OK
	}
		
/**
 * BDOS function 32 (F_USERNUM) - get/set user number
 * Supported by: CP/M 2 and later.
 * Entered with C=20h, E=number. If E=0FFh, returns number in A.
 * Set current user number. E should be 0-15, or 255 to retrieve the current user number into A. Some versions can use user areas 16-31, but these should be avoided for compatibility reasons.
 * DOS+ returns the number set in A.
 */
	void setGetUserCode(ZZ80State& state) {
		if (state.Z_Z80_STATE_MEMBER_E == 0xFF) {
#if LOG
			std::clog << "Get user number (" << unsigned(user) << ")" << std::endl;
#endif
			returnCode(state, user);	// OK - user numb.
		} else {
#if LOG
			std::clog << "Set user number to " << unsigned(state.Z_Z80_STATE_MEMBER_E) << std::endl;
#endif
			user = state.Z_Z80_STATE_MEMBER_E;
			returnCode(state, 0);	// OK
		}
	}


	bool findFile(DIR *const pDir, const char filter[12], char filename[12]) {
		struct dirent* pEnt;
		while (true) {
			auto* pEnt = readdir(pDir);
			
			if (pEnt == NULL) {
				return false;
			}
				
			if ((!strcmp(pEnt->d_name, ".")) || (!strcmp(pEnt->d_name, ".."))) continue;	// "." & ".."

			if (strchr(pEnt->d_name, '.')) {		// with dot in name
				if (strlen(pEnt->d_name) > 12) continue;	// > 12 char --> CPM invalid
				if ((strchr(pEnt->d_name, '.') - pEnt->d_name) > 8) continue;	// name > 8 char --> CPM invalid
			} else {
				if (strlen(pEnt->d_name) > 8) continue;	// name > 8 char --> CPM invalid
			}
			
			memset(filename, ' ', 11);
			filename[11] = '\0';

			const auto p = strchr(pEnt->d_name, '.');
			const auto l = p ? p - pEnt->d_name : strlen(pEnt->d_name);
			for (auto i = 0; i < l; ++i) {
				filename[i] = toupper(pEnt->d_name[i]);
			}
			if (p) {
				const auto l = strlen(pEnt->d_name) - (p - pEnt->d_name + 1);
				for (auto i = 0; i < l; ++i) {
					filename[8 + i] = toupper(p[i + 1]);
				}
			}
			
			bool ok = true;
			for (auto i = 0; i < 11; ++i) {
				if ((filter[i] != '?') && (filter[i] != filename[i])) {
					ok = false;
					break;
				}
			}
			if (!ok) continue;	// filename invalid for filter

			return true;
		}
	}





private:
/**
 * Cuurent drive.
 */
	uint8_t drive;

/**
 * DMA's address.
 */
	uint16_t dma;

/**
 * Current user.
 */
	uint8_t user;

/**
 * Scanning a path.
 */
	DIR* pDir;

/**
 * Filter used during scanning a path.
 */
	char filter[12];

};

