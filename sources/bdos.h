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
// #include <filesystem>

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
	uint8_t AL[16];
// Current record within extent. It is usually best to set this to 0 immediately after a file has been opened and then ignore it.
	uint8_t CR;
// Random access record number (not CP/M 1). A 16-bit value in CP/M 2 (with R2 used for overflow); an 18-bit value in CP/M 3.
	uint8_t R[3];
};


template <unsigned MEMORY_SIZE>
class BDos {
public:
	
/**
 * BDOS functions.
 * C register contains the function value.
 * @see http://www.gaby.de/cpm/manuals/archive/cpm22htm/ch5.htm
 */	
	void function(ZZ80State& state, uint8_t *const memory) {
		assert(memory);
		
		switch (state.Z_Z80_STATE_MEMBER_C) {
			case 0x01 : consoleInput(state); break;
			case 0x02 : consoleOutput(state); break;
			case 0x09 : printString(state, memory); break;
			case 0x0A : readConsoleBuffer(state, memory); break;
			case 0x0B : getConsoleStatus(state); break;
			case 0x0C : returnVersionNumber(state); break;
			case 0x0D : resetDiskSystem(state, memory); break;
			case 0x0E : selectDisk(state, memory); break;
			case 0x0F : openFile(state, memory); break;
			case 0x10 : closeFile(state, memory); break;
			case 0x11 : searchForFirst(state, memory); break;
			case 0x12 : searchForNext(state, memory); break;
			case 0x13 : deleteFile(state, memory); break;
			case 0x14 : readSequential(state, memory); break;
			case 0x15 : writeSequential(state, memory); break;
			case 0x16 : makeFile(state, memory); break;
			case 0x19 : returnCurrentDisk(state, memory); break;
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
	
/**
 * BDOS function 0 (P_TERMCPM) - System Reset
 * Supported by: CP/M 1, 2, 3; MP/M; Concurrent CP/M.
 * Entered with C=0. Does not return.
 * Quit the current program, return to command prompt. This call is hardly ever used in 8-bit CP/M since the RST 0 instruction does the same thing and saves four bytes.
 */
 	void systemReset();

/**
 * BDOS function 1 (C_READ) - Console input
 * Supported by: All versions
 * Entered with C=1. Returns A=L=character.
 * Wait for a character from the keyboard; then echo it to the screen and return it.
 */
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
	
/**
 * BDOS function 3 (A_READ) - Auxiliary (Reader) input
 * Supported by: All CP/M versions except MP/M and Concurrent CP/M
 * Entered with C=3. Returns A=L=ASCII character
 * Note that this call can hang if the auxiliary input never sends data.
 */
	void readerInput();
	
/**
 * BDOS function 4 (A_WRITE) - Auxiliary (Punch) output
 * Supported by: All versions except MP/M and Concurrent CP/M.
 * Entered with C=4, E=ASCII character.
 * If the device is permanently not ready, this call can hang.
 */
	void punchOutput();
	
/**
 * BDOS function 5 (L_WRITE) - Printer output
 * Supported by: All versions
 * Entered with C=2, E=ASCII character.
 * If the printer is permanently offline or busy, this call can hang.
 */
	void listOutput();
	
/**
 * BDOS function 6
 */
	void directConsoleIO();
	
/**
 * BDOS function 7
 */
	void getIOByte();
	
/**
 * BDOS function 8
 */
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
	
/**
 * BDOS function 12
 */
	void returnVersionNumber(ZZ80State& state) {
#if LOG
		std::clog << "Version number CP/M 2.2" << std::endl;
#endif
		returnCode(state, 0x0022);	// hard coded CPM 2.2
	}
	
/**
 * BDOS function 13 (DRV_ALLRESET) - Reset discs
 * Supported by: All versions.
 * Entered with C=0Dh. Returned values vary.
 * Resets disc drives. Logs out all discs and empties disc buffers. Sets the currently selected drive to A:. Any drives set to Read-Only in software become Read-Write; replacement BDOSses tend to leave them Read-Only.
 * In versions 1 and 2, logs in drive A: and returns 0FFh if there is a file present whose name begins with a $, otherwise 0. Replacement BDOSses may modify this behaviour.
 * In multitasking versions, returns 0 if succeeded, or 0FFh if other processes have files open on removable or read-only drives. 
 */
	void resetDiskSystem(ZZ80State& state, uint8_t *const memory) {
#if LOG
		std::clog << "Reset drive ; default to A" << std::endl;
#endif
		memory[DRIVE] = 0;
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
 	void selectDisk(ZZ80State& state, uint8_t *const memory) {
		if (state.Z_Z80_STATE_MEMBER_E <= 15) {
#if LOG
			std::clog << "Select disc to " << char('A' + state.Z_Z80_STATE_MEMBER_E) << std::endl;
#endif
			const char dir[2] = { char('A' + state.Z_Z80_STATE_MEMBER_E), '\0' };
			struct stat st;
			const int err = stat(dir, &st);
			if (!err) {
				memory[DRIVE] = state.Z_Z80_STATE_MEMBER_E;
				returnCode(state, 0);
				return;
			}
			state.Z_Z80_STATE_MEMBER_H = errno;
			std::cerr << ">> Error on path '" << char('A' + state.Z_Z80_STATE_MEMBER_E) << "/': " << strerror(errno) << "!" << std::endl;
		} else {
			std::cerr << ">> Invalid disk (A-P only)!" << std::endl;
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
		char filename[15];	// DIR + "/" + NAME + "." + EXT
		fcbToFilename(pFCB, memory[DRIVE], filename);

		std::fstream *const ps = getStream();
		assert(ps);
		ps->open(filename, std::ios::binary|std::ios::out|std::ios::in);
		if (!(*ps)) {
			std::cerr << ">> Error opening file '" << filename << "': " << strerror(errno) << "!" << std::endl;
			returnCode(state, 0xFF);
			releaseStream(ps);
			return;
		}
		memcpy(pFCB->AL, &ps, sizeof(ps));
		returnCode(state, 0x00);
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
 	void closeFile(ZZ80State& state, uint8_t *const memory) {
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
		std::clog << "Close file (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
		std::fstream* ps;
		memcpy(&ps, pFCB->AL, sizeof(ps));
		assert(ps);

		ps->close();
		if (ps->is_open()) {
			std::cerr << ">> Error closing file: " << strerror(errno) << "!" << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}
		
		releaseStream(ps);
		returnCode(state, 0x00);	// OK
	}
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
#if LOG
		std::clog << "Search for first (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
		const char dir[2] = { char('A' + (pFCB->DR ? pFCB->DR-1 : memory[DRIVE])), '\0' };

		memcpy(filter, pFCB->filename, 11);
		filter[11] = '\0';
		
		pDir = opendir(dir);
		if (pDir == NULL) {
			std::cerr << ">> Error on path '/" << dir << "': " << strerror(errno) << "!" << std::endl;
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
			std::cerr << ">> No search for first!" << std::endl;
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
 * BDOS function 19
 */
 	void deleteFile(ZZ80State& state, uint8_t *const memory) {
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
		const char dir[2] = { char('A' + (pFCB->DR ? pFCB->DR-1 : memory[DRIVE])), '\0' };
#if LOG
		std::clog << "Delete file (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif

		char filter[12];
		memcpy(filter, pFCB->filename, 11);
		filter[11] = '\0';
		
		const auto pDir = opendir(dir);
		if (pDir == NULL) {
			std::cerr << ">> Error on path '/" << dir << "': " << strerror(errno) << "!" << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}

		char cpm[12];
		unsigned nb = 0;
		while (findFile(pDir, filter, cpm)) {
			char dos[13];
			filenameCPM2DOS(cpm, dos);
	
			char filename[15];
			strcat(strcat(strcpy(filename, dir), "/"), dos);

			if (remove(filename)) {	// error
				std::cerr << ">> Error deleting file '" << filename << "': " << strerror(errno) << "!" << std::endl;
				returnCode(state, 0xFF);
				closedir(pDir);
				return;
			} else ++nb;
		}
		returnCode(state, nb ? 0x00 : 0xFF);
		closedir(pDir);
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
 	void readSequential(ZZ80State& state, uint8_t *const memory) {
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
		std::clog << "Read next record (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
		std::fstream* ps;
		memcpy(&ps, pFCB->AL, sizeof(ps));
		assert(ps);

		if (dma + SECTOR_SIZE >= MEMORY_SIZE) {
			std::cerr << ">> Writing DMA out of memory!" << std::endl;
			returnCode(state, 0xFF);	// OK
			return;
		}
		
		ps->read(reinterpret_cast<char*>(memory + dma), SECTOR_SIZE);
		if (*ps) {
			returnCode(state, 0x00);	// OK
		} else {
			if (ps->eof()) {
				if (ps->gcount()) {		// few read
					memset(memory + ps->gcount(), '\0', SECTOR_SIZE - ps->gcount());	// padding with '\0'
					returnCode(state, 0x00);	// OK
				} else {
					returnCode(state, 0x01);	// EOF
				} 
			} else {
				std::cerr << ">> Error reading: " << strerror(errno) << "!" << std::endl;
				
				std::cerr << ">> " << ps->gcount() << " bytes read" << std::endl;
				std::cerr << ">> good: " << ps->good() << std::endl;
				std::cerr << ">> fail: " << ps->fail() << std::endl;
				std::cerr << ">> bad: " << ps->bad() << std::endl;
				std::cerr << ">> eof: " << ps->eof() << std::endl;
				returnCode(state, 0xFF);	// OK
			}
		}
		return;
	}

/**
 * BDOS function 21
 */
	void writeSequential(ZZ80State& state, uint8_t *const memory) {
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
		std::clog << "Write next record (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
		std::fstream* ps;
		memcpy(&ps, pFCB->AL, sizeof(ps));
		assert(ps);

		if (dma + SECTOR_SIZE >= MEMORY_SIZE) {
			std::cerr << ">> Reading DMA out of memory!" << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}

		try {
			ps->write(reinterpret_cast<char*>(memory + dma), SECTOR_SIZE);
		} catch (std::exception& e) {
			std::cerr << ">> Error writing: " << strerror(errno) << "," << e.what() << "!" << std::endl;
			
			std::cerr << ">> " << ps->gcount() << " bytes wrote" << std::endl;
			std::cerr << ">> good: " << ps->good() << std::endl;
			std::cerr << ">> fail: " << ps->fail() << std::endl;
			std::cerr << ">> bad: " << ps->bad() << std::endl;
			std::cerr << ">> eof: " << ps->eof() << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}
		
		if (!ps->good()) {
			std::cerr << ">> Error writing: " << strerror(errno) << "!" << std::endl;
			std::cerr << ">> good: " << ps->good() << std::endl;
			std::cerr << ">> fail: " << ps->fail() << std::endl;
			std::cerr << ">> bad: " << ps->bad() << std::endl;
			std::cerr << ">> eof: " << ps->eof() << std::endl;
			returnCode(state, 0xFF);	// KO
			return;
		}

		returnCode(state, 0x00);	// OK
	}
	
/**
 * BDOS function 22 (F_MAKE) - create file
 * Supported by: All versions
 * Entered with C=16h, DE=address of FCB. Returns error codes in BA and HL.
 * Creates the file specified. Returns A=0FFh if the directory is full.
 * If the file exists already, then the default action is to return to the command prompt, but CP/M 3 may return a hardware error instead.
 * Under MP/M II, set F5' to open the file in "unlocked" mode.
 * Under MP/M II and later versions, set F6' to create the file with a password; the DMA address should point at a 9-byte buffer:
 *  DEFS    8   ; Password
 *  DEFB    1   ; Password mode
 */ 
	void makeFile(ZZ80State& state, uint8_t *const memory) {
		FCB_t *const pFCB = reinterpret_cast<FCB_t *const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
		std::clog << "Make file (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
		char filename[15];	// DIR + "/" + NAME + "." + EXT
		fcbToFilename(pFCB, memory[DRIVE], filename);
		
		auto s = std::ifstream(filename, std::ios_base::in);
		if (s.is_open()) {	// Existing file
			s.close();
			std::cerr << ">> Error creating file '" << filename << "': Already existing file!" << std::endl;
			returnCode(state, 0xFF);
			return;						
		}

		std::fstream *const ps = getStream();
		assert(ps);
		ps->open(filename, std::ios::binary|std::ios::out|std::ios::in|std::ios::trunc);	// create if not exists
		if (!(*ps)) {
			std::cerr << ">> Error opening file '" << filename << "': " << strerror(errno) << "!" << std::endl;
			returnCode(state, 0xFF);
			releaseStream(ps);
			return;
		}

		memcpy(pFCB->AL, &ps, sizeof(ps));
		returnCode(state, 0x00);
	}

/**
 * BDOS function 23 (F_RENAME) - Rename file
 * Supported by: All versions
 * Entered with C=17h, DE=address of FCB. Returns error codes in BA and HL.
 * Renames the file specified to the new name, stored at FCB+16. This function cannot rename across drives so the "drive" bytes of both filenames should be identical. Returns A=0-3 if successful; A=0FFh if error. Under CP/M 3, if H is zero then the file could not be found; if it is nonzero it contains a hardware error number.
 * Under Concurrent CP/M, set F5' if an extended lock on the file should be held through the rename. Otherwise the lock will be released.
 */ 
 	void renameFile(ZZ80State& state, uint8_t *const memory);
 	
/**
 * BDOS function 24 (DRV_LOGINVEC) - Return bitmap of logged-in drives
 * Supported by: All versions
 * Entered with C=18h. Returns bitmap in HL.
 * Bit 7 of H corresponds to P: while bit 0 of L corresponds to A:. A bit is set if the corresponding drive is logged in.
 * In DOSPLUS v2.1, the three top bits (for the floating drives) will mirror the status of the corresponding host drives). This does not happen in earlier DOSPLUS / Personal CP/M-86 systems.
 */ 
 	void returnLogicVector(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 25 (DRV_GET) - Return current drive
 * Supported by: All versions
 * Entered with C=19h. Returns drive in A. Returns currently selected drive. 0 => A:, 1 => B: etc.
 */
 	void returnCurrentDisk(ZZ80State& state, uint8_t *const memory) {
#if LOG
		std::clog << "Get drive (" << char('A' + memory[DRIVE]) << ')' << std::endl;
#endif
		returnCode(state, memory[DRIVE]);	// ok - drive numb.
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
 * BDOS function 27 (DRV_ALLOCVEC) - Return address of allocation map
 * Supported by: All versions, but differs in banked versions.
 * Entered with C=1Bh. Returns address in HL (16-bit versions use ES:BX).
 * Return the address of the allocation bitmap (which blocks are used and which are free) in HL. Under banked CP/M 3 and MP/M, this will be an address in bank 0 (the system bank) and not easily accessible.
 * Under previous versions, the format of the bitmap is a sequence of bytes, with bit 7 of the byte representing the lowest-numbered block on disc, and counting starting at block 0 (the directory). A bit is set if the corresponding block is in use.
 * Under CP/M 3, the allocation vector may be of this form (single-bit) or allocate two bits to each block (double-bit). This information is stored in the SCB.
 */
	void getAddrAlloc(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 28 (DRV_SETRO) - Software write-protect current disc
 * Supported by: All versions, with differences
 * Entered with C=1Ch.
 * Temporarily set current drive to be read-only; attempts to write to it will fail. Under genuine CP/M systems, this continues until either call 13 (disc reset) or call 37 (selective disc reset) is called; in practice, this means that whenever a program returns to the command prompt, all drives are reset to read/write. Newer BDOS replacements only reset the drive when function 37 is called.
 * Under multitasking CP/Ms, this can fail (returning A=0FFh) if another process has a file open on the drive.
 */
	void writeProtectDisk(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 29 (DRV_ROVEC) - Return bitmap of read-only drives
 * Supported by: All versions
 * Entered with C=1Dh. Returns bitmap in HL.
 * Bit 7 of H corresponds to P: while bit 0 of L corresponds to A:. A bit is set if the corresponding drive is set to read-only in software.
 */
	void getROVector(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 30 (F_ATTRIB) - set file attributes
 * Supported by: All versions
 * Entered with C=1Eh, DE=address of FCB. Returns error codes in BA and HL.
 * Set and reset the bits required. Standard CP/M versions allow the bits F1', F2', F3', F4', T1' (read-only), T2' (system) and T3' (archive) to be changed. Some alternative BDOSses allow F5', F6', F7' and F8' to be set, but this is not to be encouraged since setting these bits can cause CP/M 3 to behave differently.
 * Under Concurrent CP/M, if the F5' bit is not set and the file has an extended file lock, the lock will be released when the attributes are set. If F5' is set the lock stays.
 * Under CP/M 3, the Last Record Byte Count is set by storing the required value at FCB+32 (FCB+20h) and setting the F6' bit.
 * The code returned in A is 0-3 if the operation was successful, or 0FFh if there was an error. Under CP/M 3, if A is 0FFh and H is nonzero, H contains a hardware error.
 */
	void setFileAttributes(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 31 (DRV_DPB) - get DPB address
 * Supported by: CP/M 2 and later.
 * Entered with C=1Fh. Returns address in HL.
 * Returns the address of the Disc Parameter Block for the current drive. See the formats listing for details of the DPBs under various CP/M versions.
 */
	void getAddrDiskParms(ZZ80State& state, uint8_t *const memory);
	
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

/**
 * BDOS function 33
 */
	void readRandom(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 34
 */
	void writeRandom(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 35
 */
	void computeFileSize(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 36
 */
	void setRandomRecord(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 37
 */
	void resetDrive(ZZ80State& state, uint8_t *const memory);

/**
 * BDOS function 40
 */
	void writeRandomWithZeroFill(ZZ80State& state, uint8_t *const memory);



/**
 * Return a free fstream ans set it as occuped.
 * @return a reference on a free fstream.
 */
	std::fstream *const getStream() {
		for (auto i = 0; i < 10; ++i) {
			if (fileStream[i] == NULL) {
				fileStream[i] = new std::fstream;
				return fileStream[i];
			}
		}
		std::cerr << "Can't get another stream in BDOS::getStream!" << std::endl;
		throw(std::runtime_error("Can't get another stream in BDOS::getStream!"));
	}
	
/**
 * Release the fstream sent.
 * @param sStream a fstream to be released.
 */
	void releaseStream(const std::fstream *const apStream) {
		assert(apStream);
		for (auto i = 0; i < 10; ++i) {
			if (apStream == fileStream[i]) {
				delete fileStream[i];
				fileStream[i] = NULL;
				return;
			}
		}
		std::cerr << "Can't release this stream in BDOS::getStream!" << std::endl;
		throw(std::runtime_error("Can't release this stream in BDOS::getStream!"));
	}
 
 
 

	inline
	void returnCode(ZZ80State& state, const uint16_t hl) const  {
		state.Z_Z80_STATE_MEMBER_HL = hl;
		state.Z_Z80_STATE_MEMBER_A = state.Z_Z80_STATE_MEMBER_L;
		state.Z_Z80_STATE_MEMBER_B = state.Z_Z80_STATE_MEMBER_H;
	}

	inline
	void returnCode(ZZ80State& state, const uint8_t a, const uint8_t b) const  {
		state.Z_Z80_STATE_MEMBER_L = state.Z_Z80_STATE_MEMBER_A = a;
		state.Z_Z80_STATE_MEMBER_H = state.Z_Z80_STATE_MEMBER_B = b;
	}

	void filenameCPM2DOS(const char cpm[11], char dos[]) const {
		char name[9];
		memcpy(name, cpm, 8);
		name[8] = '\0';
		
		for (auto i = 7; (i >= 0) && (name[i] == ' '); --i) name[i] = '\0';
		strcpy(dos, name);
		
		char ext[4];
		memcpy(ext, cpm + 8, 3);

		if (ext[2] == ' ') {
			if (ext[1] == ' ') {
				if (ext[0] == ' ') return;	// no ext
				ext[1] = '\0';
			} else ext[2] = '\0';
		} else ext[3] = '\0';
		strcat(strcat(dos, "."), ext);
	}

	bool filenameDOS2CPM(const char dos[], char cpm[11]) const {
		char f[11];
		memset(f, ' ', 11);

		const auto p = strchr(dos, '.');
		const auto l = p ? p - dos : strlen(dos);
		if (l > 8) return false;	// Invalid CPM name
		for (auto i = 0; i < l; ++i) {
			f[i] = toupper(dos[i]);
		}
		if (p) {
			const auto l = strlen(dos) - (p - dos + 1);
			if (l > 3) return false;
			for (auto i = 0; i < l; ++i) {
				f[8 + i] = toupper(p[i + 1]);
			}
		}
		memcpy(cpm, f, 11);
		return true;
	}

	bool findFile(DIR *const pDir, const char filter[12], char filename[12]) const {
		struct dirent* pEnt;
		while (true) {
			auto* pEnt = readdir(pDir);
			
			if (pEnt == NULL) {
				return false;
			}
				
			if ((!strcmp(pEnt->d_name, ".")) || (!strcmp(pEnt->d_name, ".."))) continue;	// "." & ".."

			if (!filenameDOS2CPM(pEnt->d_name, filename)) continue;	// invalide CPM name
			filename[11] = '\0';

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
	
	void fcbToFilename(const FCB_t *const pFCB, const unsigned char drive, char path[15]) const {
		// filename[15];	// DIR + "/" + NAME + "." + EXT

		const char dir[2] = { char('A' + (pFCB->DR ? pFCB->DR-1 : drive)), '\0' };

		char filename[13];
		filenameCPM2DOS(pFCB->filename, filename);
		strcat(strcat(strcpy(path, dir), "/"), filename);
	}

private:

	static const auto SECTOR_SIZE = 128;

/**
 * Current drive.
 */
//	uint8_t drive = 0;	// A
	static const auto DRIVE = 4;

/**
 * DMA's address.
 */
	uint16_t dma = 128;

/**
 * Current user.
 */
	uint8_t user = 0;

/**
 * Scanning a path.
 */
	DIR* pDir = NULL;

/**
 * Filter used during scanning a path.
 */
	char filter[12] = "";
	
/**
 * List of available fstream.
 */
	std::fstream* fileStream[10] = { NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL };


};

