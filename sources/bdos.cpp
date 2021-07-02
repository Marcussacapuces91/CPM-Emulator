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

#include <dirent.h>
#include <sys/stat.h>

#include "computer.h"

// #define LOG 1

/**
 * BDOS functions.
 * C register contains the function value.
 * @see http://www.gaby.de/cpm/manuals/archive/cpm22htm/ch5.htm
 */	
void Computer::bdos(ZZ80State& state) {
	static int user = 0;
	static unsigned drive = 0;
	static uint16_t dma = 0x80;	// default init value
	static DIR *pDir = NULL;	// parcours de drive virtuel
	
	switch (state.Z_Z80_STATE_MEMBER_C) {

/**
* BDOS function 2 (C_WRITE) - Console output
* Supported by: All versions
* Entered with C=2, E=ASCII character.
* Send the character in E to the screen. Tabs are expanded to spaces. Output can be paused with ^S and restarted with ^Q (or any key under versions prior to CP/M 3). While the output is paused, the program can be terminated with ^C.
*/
		case 0x02 : {
#if LOG
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
			break;
		}
		
/**
 * BDOS function 9 (C_WRITESTR) - Output string
 * Supported by: All versions
 * Entered with C=9, DE=address of string.
 * Display a string of ASCII characters, terminated with the $ character. Thus the string may not contain $ characters - so, for example, the VT52 cursor positioning command ESC Y y+32 x+32 will not be able to use row 4.
 * Under CP/M 3 and above, the terminating character can be changed using BDOS function 110.
 */
 		case 0x09 : {
#if LOG
			std::clog << "Output string (Buffer " << std::hex << state.Z_Z80_STATE_MEMBER_DE << "h)" << std::endl;
#endif
			for (uint16_t i = state.Z_Z80_STATE_MEMBER_DE ; memory[i] != '$' ; ++i) {
				std::cout << char(memory[i]);
			}
			break;
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

		case 0x0A : {
#if LOG
			std::clog << "Buffered console input (Buffer " << std::hex << state.Z_Z80_STATE_MEMBER_DE << "h)" << std::endl;
//			std::clog << "mx" << unsigned(memory[DE+0]) << std::endl;
//			std::clog << "nc" << unsigned(memory[DE+1]) << std::endl;
#endif			
			std::string line;
			std::cin >> line;
			memory[state.Z_Z80_STATE_MEMBER_DE + 1] = line.length();
			for (unsigned i = 0; i < line.length(); ++i) {
				memory[state.Z_Z80_STATE_MEMBER_DE + 2 + i] = line[i];
			}
//			memory[DE+2 + line.length()] = 0;
			
			break;
		 }

/**
 * BDOS function 11 (C_STAT) - Console status
 * Supported by: All versions
 * Entered with C=0Bh. Returns A=L=status
 * Returns A=0 if no characters are waiting, nonzero if a character is waiting.
 */
		case 0x0B : {
#if LOG
			std::clog << "Console status - always ok" << std::endl;
#endif
			state.Z_Z80_STATE_MEMBER_A = 0;
			break;
		}

/**
 * BDOS function 13 (DRV_ALLRESET) - Reset discs
 * Supported by: All versions.
 * Entered with C=0Dh. Returned values vary.
 * Resets disc drives. Logs out all discs and empties disc buffers. Sets the currently selected drive to A:. Any drives set to Read-Only in software become Read-Write; replacement BDOSses tend to leave them Read-Only.
 * In versions 1 and 2, logs in drive A: and returns 0FFh if there is a file present whose name begins with a $, otherwise 0. Replacement BDOSses may modify this behaviour.
 * In multitasking versions, returns 0 if succeeded, or 0FFh if other processes have files open on removable or read-only drives. 
 */
		case 0x0D : {
#if LOG
			std::clog << "Reset drive ; default to A" << std::endl;
#endif
			drive = 0;
			break;
		}
		
/**
 * BDOS function 14 (DRV_SET) - Select disc
 * Supported by: All versions
 * Entered with C=0Eh, E=drive number. Returns L=A=0 or 0FFh.
 * The drive number passed to this routine is 0 for A:, 1 for B: up to 15 for P:.
 * Sets the currently selected drive to the drive in A; logs in the disc. Returns 0 if successful or 0FFh if error. Under MP/M II and later versions, H can contain a physical error number. 
 */
		case 0x0E : {
			if (state.Z_Z80_STATE_MEMBER_E <= 15) {
#if LOG
				std::clog << "Select disc to " << char('A' + state.Z_Z80_STATE_MEMBER_E) << std::endl;
#endif
				const char dir[2] = { char('A' + state.Z_Z80_STATE_MEMBER_E), '\0' };
				struct stat st;
				const int err = stat(dir, &st);
				if (!err) {
					drive = state.Z_Z80_STATE_MEMBER_E;
					state.Z_Z80_STATE_MEMBER_L = 0;
					state.Z_Z80_STATE_MEMBER_A = 0;
					break;
				}
				state.Z_Z80_STATE_MEMBER_H = errno;
				std::cout << "Error selecting disk '" << char('A' + state.Z_Z80_STATE_MEMBER_E) << ":' " << strerror(errno) << "!" << std::endl;
			} else {
				std::cout << "Invalid disk [A..P]!" << std::endl;
			}
			state.Z_Z80_STATE_MEMBER_L = 0xFF; 
			state.Z_Z80_STATE_MEMBER_A = 0xFF;
			break;
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
		case 0x0F : {
 			struct __attribute__ ((packed)) FCB {
 				uint8_t DR;
 				char filename[8];
 				char filetype[3];
 				uint8_t EX;
 				uint8_t S1;
 				uint8_t S2;
 				uint8_t RC;
 				uint8_t AL;
 				uint8_t CR;
 				uint16_t RN;
			};
			FCB*const pFCB = reinterpret_cast<FCB*const>(memory + state.Z_Z80_STATE_MEMBER_DE);
			std::clog << "Open file (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;

			std::clog << "Open file " << pFCB->filename << '.' << pFCB->filetype << std::endl;

			break;
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
 		case 0x11 : {
 			struct __attribute__ ((packed)) FCB {
 				uint8_t DR;
 				char filename[8];
 				char filetype[3];
 				uint8_t EX;
 				uint8_t S1;
 				uint8_t S2;
 				uint8_t RC;
 				uint8_t AL;
 				uint8_t CR;
 				uint16_t RN;
			 };
			FCB*const pFCB = reinterpret_cast<FCB*const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
			std::clog << "Search for first (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
//			const std::string filename = std::string(pFCB->filename) + '.' + pFCB->filetype;
//			std::cout << filename << std::endl;
			
			const char dir[2] = { char('A' + drive), '\0' };
			pDir = opendir(dir);
			if (pDir == NULL) {
				state.Z_Z80_STATE_MEMBER_A = 0xFF;
				break;
			}
			struct dirent* pEnt = NULL;
			do {
				pEnt = readdir(pDir);
			} while ( (pEnt != NULL) && ((!strcmp(pEnt->d_name, ".")) || (!strcmp(pEnt->d_name, ".."))) );

			if (pEnt == NULL) {
				state.Z_Z80_STATE_MEMBER_A = 0xFF;
				break;
			}
			memory[dma] = '\0';
			memset(memory + dma + 1, ' ', 11);
			const int p = strchr(pEnt->d_name, int('.')) - pEnt->d_name;
			for (int i = 0; i < p; ++i) {
				memory[dma+i+1] = pEnt->d_name[i];
			}
			for (int i = 0; i < 3; ++i) {
				memory[dma+9+i] = pEnt->d_name[p+i+1];
			}
			
			state.Z_Z80_STATE_MEMBER_A = 0;	// OK
			break;
		}

/**
 * BDOS function 18 (F_SNEXT) - search for next
 * Supported by: All versions
 * Entered with C=12h, (DE=address of FCB)?. Returns error codes in BA and HL.
 * This function should only be executed immediately after function 17 or another invocation of function 18. No other disc access functions should have been used.
 * Function 18 behaves exactly as number 17, but finds the next occurrence of the specified file after the one returned last time. The FCB parameter is not documented, but Jim Lopushinsky states in LD301.DOC:
 * In none of the official Programmer's Guides for any version of CP/M does it say that an FCB is required for Search Next (function 18). However, if the FCB passed to Search First contains an unambiguous file reference (i.e. no question marks), then the Search Next function requires an FCB passed in reg DE (for CP/M-80) or DX (for CP/M-86).
 */
 		case 0x12 : {
 			struct FCB {
 				uint8_t DR;
 				char filename[8];
 				char filetype[3];
 				uint8_t EX;
 				uint8_t S1;
 				uint8_t S2;
 				uint8_t RC;
 				uint8_t AL;
 				uint8_t CR;
 				uint16_t RN;
			 };
			 
			FCB*const pFCB = reinterpret_cast<FCB*const>(memory + state.Z_Z80_STATE_MEMBER_DE);
#if LOG
			std::clog << "Search for next (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;
#endif
//			const std::string filename = std::string(pFCB->filename) + '.' + pFCB->filetype;
//			const std::string dir = std::string("CPM22-b");
			if (pDir == NULL) {
				state.Z_Z80_STATE_MEMBER_A = 0xFF;
				break;
			}

			struct dirent *const pEnt = readdir(pDir);
			if (pEnt == NULL) {
				state.Z_Z80_STATE_MEMBER_A = 0xFF;
				break;
			}
			memory[dma] = '\0';
			memset(memory + dma + 1, ' ', 11);
			const int p = strchr(pEnt->d_name, int('.')) - pEnt->d_name;
			for (int i = 0; i < p; ++i) {
				memory[dma+i+1] = pEnt->d_name[i];
			}
			for (int i = 0; i < strlen(pEnt->d_name) - p - 1; ++i) {
				memory[dma+9+i] = pEnt->d_name[p+i+1];
			}
			
			state.Z_Z80_STATE_MEMBER_A = 0;	// OK
			break;
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
		case 0x14 : {
 			struct FCB {
 				uint8_t DR;
 				char filename[8];
 				char filetype[3];
 				uint8_t EX;
 				uint8_t S1;
 				uint8_t S2;
 				uint8_t RC;
 				uint8_t AL;
 				uint8_t CR;
 				uint16_t RN;
			};
			FCB*const pFCB = reinterpret_cast<FCB*const>(memory + state.Z_Z80_STATE_MEMBER_DE);
			std::clog << "Read next record (FCB: " << std::hex << unsigned(state.Z_Z80_STATE_MEMBER_DE) << "h)" << std::endl;

#if LOG
			std::clog << "Read next record" << std::endl;
#endif
			state.Z_Z80_STATE_MEMBER_A = 1;
			break;
		}

/**
 * BDOS function 25 (DRV_GET) - Return current drive
 * Supported by: All versions
 * Entered with C=19h. Returns drive in A. Returns currently selected drive. 0 => A:, 1 => B: etc.
 */
		case 0x19 : {
#if LOG
			std::clog << "Get drive (" << drive << ')' << std::endl;
#endif
			state.Z_Z80_STATE_MEMBER_A = drive;
			break;
		}

/**
 * BDOS function 26 (F_DMAOFF) - Set DMA address
 * Supported by: All versions
 * Entered with C=1Ah, DE=address.
 * Set the Direct Memory Access address; a pointer to where CP/M should read or write data. Initially used for the transfer of 128-byte records between memory and disc, but over the years has gained many more functions.
 */
		case 0x1A : {
#if LOG
			std::clog << "Set DMA address  (" << std::hex << state.Z_Z80_STATE_MEMBER_DE << ')' << std::endl;
#endif
			dma = state.Z_Z80_STATE_MEMBER_DE;
			break;
		}
		
/**
 * BDOS function 32 (F_USERNUM) - get/set user number
 * Supported by: CP/M 2 and later.
 * Entered with C=20h, E=number. If E=0FFh, returns number in A.
 * Set current user number. E should be 0-15, or 255 to retrieve the current user number into A. Some versions can use user areas 16-31, but these should be avoided for compatibility reasons.
 * DOS+ returns the number set in A.
 */
		case 0x20 : {
			if (state.Z_Z80_STATE_MEMBER_E == 0xFF) {
#if LOG
				std::clog << "Get user number (" << unsigned(user) << ")" << std::endl;
#endif
				state.Z_Z80_STATE_MEMBER_A = user;
			} else {
#if LOG
				std::clog << "Set user number to " << unsigned(state.Z_Z80_STATE_MEMBER_E) << std::endl;
#endif
				user = state.Z_Z80_STATE_MEMBER_E;
			}
			break;
		}
		
		default:
			std::cerr << "Register C: " << std::hex << std::setw(2) << std::setfill('0') << unsigned(state.Z_Z80_STATE_MEMBER_C) << "h";
			std::cerr << " : Unknown BDOS function!" << std::endl;
			
			throw std::runtime_error("Un-emulated BDOS function");
			break;
	}
}
