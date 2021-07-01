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

#include <cassert>

#include <iostream>
#include <iomanip>
#include <string>
#include <fstream>
#include <exception>

#include "Z80.h"

#pragma once

#define S(x) #x
#define S_(x) S(x)
#define S__LINE__ S_(__LINE__)

#define LOG		1

/**
 * @see http://www.cpm.z80.de/manuals/cpm22-m.pdf
 *
 * Reserved location in page 0
 *
 * Main memory page zero, between locations 0H and 0FFH, contains several segments of code and
 * data that are used during CP/M processing. The code and data areas are given in the following
 * table.
 *
 * Table 6-6. Reserved Locations in Page Zero
 
 * Locations		Contents
 * 0000H-0002H 		Contains a jump instruction to the warm start entry location 4A03H+b. 
 * 					This allows a simple programmed restart (JMP 0000H) or manual restart 
 *					from the front panel.
 *
 * 0003H-0003H		Contains the Intel standard IOBYTE is optionally included in the user's 
 * 					CBIOS (refer to Section 6.6).
 *
 * 0004H-0004H		Current default drive number (0=A,...,15=P).
 *
 * 0005H-0007H		Contains a jump instruction to the BDOS and serves two purposes: JMP 
 * 					0005H provides the primary entry point to the BDOS, as described in 
 * 					Section 5, and LHLD 0006H brings the address field of the instruction to 
 * 					the HL register pair. This value is the lowest address in memory used by 
 * 					CP/M, assuming the CCP is being overlaid. The DDT program changes the
 * 					address field to reflect the reduced memory size in debug mode.
 *
 * 0008H-0027H		Interrupt locations I through 5 not used.
 *
 * 0030H-0037H		Interrupt location 6 (not currently used) is reserved.
 *
 * 0038H-003AH		Restart 7; contains a jump instruction into the DDT or SID program when 
 *					running in debug mode for programmed breakpoints, but is not otherwise 
 *					used by CP/M.
 *
 * 003BH-003FH		Not currently used; reserved
 *
 * 0040H-004FH		A 16-byte area reserved for scratch by CBIOS, but is not used for any 
 *					purpose in the distribution version of CP/M.
 *
 * 0050H-005BH		Not currently used; reserved.
 *
 * 005CH-007CH		Default File Control Block produced for a transient program by the CCP.
 *
 * 007DH-007FH		Optional default random record position.
 *
 * 0080H-00FFH		Default 128-byte disk buffer, also filled with the command line when a 
 *					transient is loaded under the CCP.
 * This information is set up for normal operation under the CP/M system, but can be overwritten
 * by a transient program if the BDOS facilities are not required by the transient.
 * If, for example, a particular program performs only simple I/O and must begin execution at
 * location 0, it can first be loaded into the TPA, using normal CP/M facilities, with a small
 * memory move program that gets control when loaded. The memory move program must get
 * control from location 0100H, which is the assumed beginning of all transient programs. The
 * move program can then proceed to the entire memory image down to location 0 and pass control
 * to the starting address of the memory load. 
 * If the BIOS is overwritten or if location 0, containing the warm start entry point, is overwritten,
 * the operator must bring the CP/M system back into memory with a cold start sequence
 
 * CCP : 0x3400-0x3B80
 * BDOS : 0x3c00-0x4980 + BIAS
 * BIOS : 0x4a00-0x4c80 + BIAS
 *	0x4a00 : JMP BOOT (COLD START)
 *  0x4a03 : JMP WBOOT (WARM START)
 *  0x4a06 : JMP CONST (check for console char ready)
 *  0x4a09 ...
 *  ...
 *  0x4a30 : JMP SECTRAN (sector translate subroutine)
 */
 
class Computer {

public:
	void init() {
		std::cout << "Zilog Z80 CPU Emulator" << std::endl;
//		std::cout << "Copyright © 1999-2018 Manuel Sainz de Baranda y Goñi." << std::endl;
		std::cout << "Copyright (c) 1999-2018 Manuel Sainz de Baranda y Goni." << std::endl;
		std::cout << "Released under the terms of the GNU General Public License v3." << std::endl;
		std::cout << std::endl;
		
		cpu.context = this;
		cpu.read = Computer::read;
		cpu.write = Computer::write;
		cpu.in = Computer::in;
		cpu.out = Computer::out;
		cpu.int_data = NULL;
		cpu.halt = NULL;
		z80_power(&cpu, true);
		z80_reset(&cpu);
		
		std::cout << "CPM 2.2 Emulator - Copyright (c) 2021 by M. Sibert" << std::endl;
		std::cout << std::endl;
		
		memory[0x0006] = 0x00; 
		memory[0x0007] = 0x3C; 
		cpu.state.Z_Z80_STATE_MEMBER_SP = 0x80 + 128; // TBUFF + 80h
		cpu.state.Z_Z80_STATE_MEMBER_C = 0;	// Default user 0xF0 & Default disk 0x0F
	}
		
	int load(const std::string& aFile, const uint16_t aAddr=0x3400 + BIAS) {
		assert((!aFile.empty()));
		
		auto fs = std::ifstream(aFile, std::ios_base::binary | std::ios_base::in);
		try {
			fs.exceptions(std::fstream::badbit);
		} catch (std::ifstream::failure& e) {
			std::cerr << "Error opening file \"" << aFile << "\": " << e.what() << "!" << std::endl;
			return -1;
		}
	
		std::clog << "Loading " << aFile << "... ";
		auto addr = aAddr;
		
		while (fs.good()) {
			const uint8_t c = fs.get();
			if (!fs.eof()) {
				memory[addr++] = c;
			}
		}
		fs.close();
		std::clog << addr - aAddr << " bytes read." << std::endl;
		return 0;
	}
	
	void run(const uint16_t aAddr=0x3400 + BIAS) {
		cpu.state.Z_Z80_STATE_MEMBER_PC = aAddr;
		while (true) {
			if (cpu.state.Z_Z80_STATE_MEMBER_PC == 0x0000) {	// Reset
				return;
			} else if (cpu.state.Z_Z80_STATE_MEMBER_PC == 0x0005) {	// BDOS
				bdos(cpu.state);
				cpu.state.Z_Z80_STATE_MEMBER_PC = memory[cpu.state.Z_Z80_STATE_MEMBER_SP++];
				cpu.state.Z_Z80_STATE_MEMBER_PC += memory[cpu.state.Z_Z80_STATE_MEMBER_SP++] * 256U;
			} else {
				if (memory[cpu.state.Z_Z80_STATE_MEMBER_PC] == 0x07) {	// RLCA
					const auto A = cpu.state.Z_Z80_STATE_MEMBER_A;
					const auto F = cpu.state.Z_Z80_STATE_MEMBER_F;
					const uint8_t RA = (A << 1) | (A >> 7);
					const uint8_t RF = (cpu.state.Z_Z80_STATE_MEMBER_F & 0b11000100) | 
						((A & 0x80) ? 0x1 : 0x0) | 		// CF
						((RA & 0x08) ? 0x8 : 0x0) | 	// XF
						((RA & 0x20) ? 0x20 : 0x0);		// YF
					
//					logState(cpu.state);
					z80_run(&cpu, 1);
//					logState(cpu.state);
					
					if (cpu.state.Z_Z80_STATE_MEMBER_A != RA) {
						throw std::runtime_error("Accumulator difference in RLCA!");
					} else if (cpu.state.Z_Z80_STATE_MEMBER_F != RF) {
						std::clog << std::hex << int(cpu.state.Z_Z80_STATE_MEMBER_F) << " != " << int(RF) << std::endl;
						throw std::runtime_error("Flags difference in RLCA!");
					}	
				} else if (memory[cpu.state.Z_Z80_STATE_MEMBER_PC] == 0x17) {	// RLA
					const auto A = cpu.state.Z_Z80_STATE_MEMBER_A;
					const auto F = cpu.state.Z_Z80_STATE_MEMBER_F;
					const uint8_t RA = (A << 1) | (F & 0x1 ? 0x1 : 0x0);
					const uint8_t RF = (cpu.state.Z_Z80_STATE_MEMBER_F & 0b11000100) | 
						((A & 0x80) ? 0x1 : 0x0) | 		// CF
						((RA & 0x08) ? 0x8 : 0x0) | 	// XF
						((RA & 0x20) ? 0x20 : 0x0);		// YF
					
//					logState(cpu.state);
					z80_run(&cpu, 1);
//					logState(cpu.state);
					
					if (cpu.state.Z_Z80_STATE_MEMBER_A != RA) {
						throw std::runtime_error("Accumulator difference in RLA!");
					} else if (cpu.state.Z_Z80_STATE_MEMBER_F != RF) {
						std::clog << std::hex << int(cpu.state.Z_Z80_STATE_MEMBER_F) << " != " << int(RF) << std::endl;
						throw std::runtime_error("Flags difference in RLA!");
					}
				} else if (memory[cpu.state.Z_Z80_STATE_MEMBER_PC] == 0x0F) {	// RRCA
					const auto A = cpu.state.Z_Z80_STATE_MEMBER_A;
					const auto F = cpu.state.Z_Z80_STATE_MEMBER_F;
					const uint8_t RA = (A >> 1) | (A & 0x1 ? 0x80 : 0x00);
					const uint8_t RF = (cpu.state.Z_Z80_STATE_MEMBER_F & 0b11000100) | 
						((A & 0x01) ? 0x1 : 0x0) | 		// CF
						((RA & 0x08) ? 0x8 : 0x0) | 	// XF
						((RA & 0x20) ? 0x20 : 0x0);		// YF
					
//					logState(cpu.state);
					z80_run(&cpu, 1);
//					logState(cpu.state);
					
					if (cpu.state.Z_Z80_STATE_MEMBER_A != RA) {
						throw std::runtime_error("Accumulator difference in RRCA!");
					} else if (cpu.state.Z_Z80_STATE_MEMBER_F != RF) {
						std::clog << std::hex << int(cpu.state.Z_Z80_STATE_MEMBER_F) << " != " << int(RF) << std::endl;
						throw std::runtime_error("Flags difference in RRCA!");
					}
				} else if (memory[cpu.state.Z_Z80_STATE_MEMBER_PC] == 0x1F) {	// RRA
					const auto A = cpu.state.Z_Z80_STATE_MEMBER_A;
					const auto F = cpu.state.Z_Z80_STATE_MEMBER_F;
					const uint8_t RA = (A >> 1) | (F & 0x1 ? 0x80 : 0x00);
					const uint8_t RF = (cpu.state.Z_Z80_STATE_MEMBER_F & 0b11000100) | 
						((A & 0x01) ? 0x1 : 0x0) | 		// CF
						((RA & 0x08) ? 0x8 : 0x0) | 	// XF
						((RA & 0x20) ? 0x20 : 0x0);		// YF
					
//					logState(cpu.state);
					z80_run(&cpu, 1);
//					logState(cpu.state);
					
					if (cpu.state.Z_Z80_STATE_MEMBER_A != RA) {
						throw std::runtime_error("Accumulator difference in RRA!");
					} else if (cpu.state.Z_Z80_STATE_MEMBER_F != RF) {
						std::clog << std::hex << int(cpu.state.Z_Z80_STATE_MEMBER_F) << " != " << int(RF) << std::endl;
						throw std::runtime_error("Flags difference in RRA!");
					}
				} else if (memory[cpu.state.Z_Z80_STATE_MEMBER_PC] == 0xCB) {	// RLC
					const auto A = cpu.state.Z_Z80_STATE_MEMBER_A;
					const auto F = cpu.state.Z_Z80_STATE_MEMBER_F;
					
					uint8_t* r;
					switch (memory[cpu.state.Z_Z80_STATE_MEMBER_PC+1]) {
						case 0x1 : r = &cpu.state.Z_Z80_STATE_MEMBER_B; break;
						case 0x2 : r = &cpu.state.Z_Z80_STATE_MEMBER_C; break;
						case 0x3 : r = &cpu.state.Z_Z80_STATE_MEMBER_D; break;
						case 0x4 : r = &cpu.state.Z_Z80_STATE_MEMBER_E; break;
						case 0x5 : r = &cpu.state.Z_Z80_STATE_MEMBER_H; break;
						case 0x6 : r = &cpu.state.Z_Z80_STATE_MEMBER_L; break;
						case 0x7 : r = &memory[cpu.state.Z_Z80_STATE_MEMBER_HL]; break;
						case 0x8 : r = &cpu.state.Z_Z80_STATE_MEMBER_A; break;
					}
					
					const uint8_t RA = (*r << 1) | (*r & 0x08 ? 0x01 : 0x00);
					const uint8_t RF = (cpu.state.Z_Z80_STATE_MEMBER_F & 0b00000000) | 
						((*r & 0x80) ? 0x80 : 0x00) |	// SF
						((*r == 0  ) ? 0x40 : 0x00) |	// ZF
						((RA & 0x20) ? 0x20 : 0x00) |	// YF
						((RA & 0x08) ? 0x08 : 0x00) | 	// XF
						(parity(*r)  ? 0x04 : 0x00) | 	// PF
						((*r & 0x80) ? 0x01 : 0x00); 	// CF
					
//					logState(cpu.state);
					z80_run(&cpu, 1);
//					logState(cpu.state);
					
					if (cpu.state.Z_Z80_STATE_MEMBER_A != RA) {
						throw std::runtime_error("Accumulator difference in RLCA!");
					} else if (cpu.state.Z_Z80_STATE_MEMBER_F != RF) {
						std::clog << std::hex << int(cpu.state.Z_Z80_STATE_MEMBER_F) << " != " << int(RF) << std::endl;
						throw std::runtime_error("Flags difference in RLCA!");
					}

/*				
					logSpecAddr(cpu.state);
					logInst(cpu.state);
					logState(cpu.state);
					
					std::clog << "Execution: ";
					const unsigned c = z80_run(&cpu, 1);	// return cycles
					std::clog << c << " cycles" << std::endl;
					
					logState(cpu.state);
					std::clog << std::endl;
*/					
				} else {
#ifdef LOG				
					logSpecAddr(cpu.state);
					logInst(cpu.state);
#endif
					const unsigned c = z80_run(&cpu, 1);	// return cycles
				}
			}
		}
	}

protected:
	
/**
 * Callback used by Z80 to read from the memory.
 * @param context Pointer on Computer's instance.
 * @param address Memory address to be read.
 * @return read value. 
 */	
	static zuint8 read(void* context, zuint16 address) {
		auto c = static_cast<Computer *const>(context);
		return c->memory[address];
	}

/**
 * Callback used by Z80 to write in memory.
 * @param context Pointer on Computer's instance.
 * @param address Memory address to be write.
 * @param value Value to be write in memory.
 */	
	static void write(void* context, zuint16 address, zuint8 value) {
		auto c = static_cast<Computer *const>(context);
		c->memory[address] = value;
	}

/**
 * Callback used by Z80 to read from ports.
 * @param context Pointer on Computer's instance.
 * @param address Port address to be read.
 * @return read value. 
 */	
	static zuint8 in(void* context, zuint16 address) {
		throw std::runtime_error("Not implemented at " __FILE__ ": " S__LINE__);
		return 0;
	}

/**
 * Callback used by Z80 to write on ports.
 * @param context Pointer on Computer's instance.
 * @param address Port address to be write.
 * @param value Value to be write in ports.
 */	
	static void out(void* context, zuint16 address, zuint8 value) {
		throw std::runtime_error("Not implemented at " __FILE__ ": " S__LINE__);
	}

	static zuint32 int_data(void* context) {
		throw std::runtime_error("Not implemented at " __FILE__ ": " S__LINE__);
		return 0;
	}

/**
 * Execute indicated BDOS function (usually C reg.).
 * @param function Func. number.
 * @see http://www.gaby.de/cpm/manuals/archive/cpm22htm/ch5.htm
 */	
	void bdos(ZZ80State& state);
	
/**
 * Add a comment for special addr found in CCP source code.
 * @param CPU state.
 */
	void logSpecAddr(const ZZ80State& state) const {
		switch (state.Z_Z80_STATE_MEMBER_PC) {
			case 0x0005 : std::clog << "; BDOS function #" << std::dec << int(state.Z_Z80_STATE_MEMBER_C) << " - "; break;
			case 0xDC8C : std::clog << "; Routine Print" << std::endl; break;
			case 0xDC92 : std::clog << "; Routine Print / save BC" << std::endl; break;
			case 0xDC98 : std::clog << "; Routine Print CR/LF" << std::endl; break;
			case 0xDCA2 : std::clog << "; Routine Print Space" << std::endl; break;
			case 0xDCA7 : std::clog << "; Routine Print Line" << std::endl; break;
			case 0xDCB8 : std::clog << "; Routine Reset disk" << std::endl; break;
			case 0xDCBD : std::clog << "; Routine Select disk" << std::endl; break;
			case 0xDCC3 : std::clog << "; Routine Call bdos & save return" << std::endl; break;
			case 0xDCCB : std::clog << "; Routine Open file (DE) point FCB" << std::endl; break;
			case 0xDDA7 : std::clog << "; Convert input line to upper case." << std::endl; break;
			case 0xDE09 : std::clog << "; Print back file name with a '?' to indicate a syntax error." << std::endl; break;
			case 0xDE30 : std::clog << "; Check character at (DE) for legal command input. Note that the zero flag is set if the character is a delimiter." << std::endl; break;
			case 0xDE5E : std::clog << "; Convert the first name in (FCB)." << std::endl; break;
			case 0xDEFE : std::clog << "; Check to see if this is an ambigeous file name specification." << std::endl; break;
			case 0xDF2E : std::clog << "; Search the command table for a match with what has just been entered." << std::endl; break;
			case 0xDF5C : std::clog << "; C C P  -   C o n s o l e   C o m m a n d   P r o c e s s o r" << std::endl; break;
/*			case 0xdfc0 : 
				for (unsigned i = 0xDFC1; i <= 0xDFCF; ++i) {
					std::clog << std::hex << std::setw(2) << int(memory[i])<< " ";
				}
				std::clog << std::endl;
				break; */
			case 0xE077 : std::clog << "; D I R E C T O R Y   C O M M A N D" << std::endl; break;
			case 0xE210 : std::clog << "; R E N A M E   C O M M A N D" << std::endl; break;
			case 0xE28E : std::clog << "; U S E R   C O M M A N D" << std::endl; break;
			case 0xE2A5 : std::clog << "; T R A N S I A N T   P R O G R A M   C O M M A N D" << std::endl; break;
			
			case 0x1dce : std::clog << "; PUSHs, call BDOS, POPs" << std::endl; break;
			case 0x012f : std::clog << "; DONE !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!" << std::endl; exit(0); break;
			case 0x1ae2 : std::clog << "; stt: Start Test pointed by (HL)" << std::endl; break;
			case 0x1c38 : std::clog << "; clrmem: clear memory at hl, bc bytes" << std::endl; break;
			case 0x1c49 : std::clog << "; initmask: initialise counter or shifter (DE & HL)" << std::endl; break;
		}
	}
	
	void logInst(const ZZ80State& state) const {
		const uint16_t PC = state.Z_Z80_STATE_MEMBER_PC;
		const uint8_t inst = memory[PC];
		
		switch (inst) {
			case 0x00 : {	// NOP
				logAddrInst(PC, inst);
				std::clog << "NOP " << std::endl;
				break;
			}

			case 0x01 : 
			case 0x11 : 
			case 0x21 : 
			case 0x31 : {	// LD dd,nn
				const uint16_t nn = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "LD " << ddName(inst >> 4) << ',' << std::dec << unsigned(nn) << std::endl;
				break;
			}

			case 0x02 : 
			case 0x12 : {	// LD (rr),A
				logAddrInst(PC, inst);
				std::clog << "LD (" << ddName(inst >> 4) << "),A" << std::endl;
				break;
			}

			case 0x03 :
			case 0x13 : 
			case 0x23 : 
			case 0x33 : {	// INC ss
				logAddrInst(PC, inst);
				std::clog << "INC " << ddName(inst >> 4) << std::endl;
				break;
			}

			case 0x04 :
			case 0x0C :
			case 0x14 :
			case 0x1C :
			case 0x24 :
			case 0x2C :
			case 0x34 :
			case 0x3C : {	// INC r
				logAddrInst(PC, inst);
				std::clog << "INC " << rName(inst >> 3) << std::endl;
				break;
			}
	
			case 0x05 :
			case 0x0D :
			case 0x15 :
			case 0x1D :
			case 0x25 :
			case 0x2D :
			case 0x35 :
			case 0x3D : {	// DEC r
				logAddrInst(PC, inst);
				std::clog << "DEC " << rName(inst >> 3) << std::endl;
				break;
			}

			case 0x06 : 
			case 0x0E : 
			case 0x16 : 
			case 0x1E : 
			case 0x26 : 
			case 0x2E : 
			case 0x36 : 
			case 0x3E : {	// LD r,n & LD (HL),n
				const uint8_t n = memory[PC+1];
				logAddrInst(PC, inst, n);
				std::clog << "LD " << rName(inst >> 3) << ',' << std::dec << unsigned(n) << std::endl;
	  			break;
			}

			case 0x07 : {	// RLCA
				logAddrInst(PC, inst);
				std::clog << "RLCA" << std::endl;
				break;
			}

			case 0x09 :
			case 0x19 :
			case 0x29 :
			case 0x39 : {	// ADD HL,ss
				logAddrInst(PC, inst);
				std::clog << "ADD HL," << ddName(inst >> 4) << std::endl;
				break;
			}

			case 0x0A :
			case 0x1A : {	// LD A,(dd)
				logAddrInst(PC, inst);
				std::clog << "LD A,(" << ddName(inst >> 4) << ")" << std::endl;
				break;
			}

			case 0x0B :
			case 0x1B : 
			case 0x2B : 
			case 0x3B : {	// DEC ss
				logAddrInst(PC, inst);
				std::clog << "DEC " << ddName(inst >> 4) << std::endl;
				break;
			}

			case 0x0F : { 	// RRCA
				logAddrInst(PC, inst);
				std::clog << "RRCA" << std::endl;
				break;
			}
	
			case 0x17 : { 	// RLA
				logAddrInst(PC, inst);
				std::clog << "RLA" << std::endl;
				break;
			}
			
			case 0x1F : { 	// RRA
				logAddrInst(PC, inst);
				std::clog << "RRA " << std::endl;
				break;
			}
			
			case 0x22 : {	// LD (addr), HL
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "LD (" << std::hex << std::setw(4) << std::setfill('0') << addr << "h),HL" << std::endl;
				break;
			}

			case 0x2A : {	// LD HL,(nn)
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "LD HL,(" << std::hex << std::setw(4) << std::setfill('0') << addr << "h)" << std::endl;
				break;
			}
				
			case 0x32 : {	// LD (nn),A
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "LD (" << std::setw(4) << std::setfill('0') << addr << "h),A" << std::endl;
				break;
			}
	
			case 0x3A : {	// LD A,(addr)
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "LD A,(" << std::setw(4) << std::setfill('0') << addr << "h)" << std::endl;
				break;
			}
				
			case 0x40 :
			case 0x41 :
			case 0x42 :
			case 0x43 :
			case 0x44 :
			case 0x45 :
			case 0x46 :
			case 0x47 :
			case 0x48 :
			case 0x49 :
			case 0x4A :
			case 0x4B :
			case 0x4C :
			case 0x4D :
			case 0x4E :
			case 0x4F :
			case 0x50 :
			case 0x51 :
			case 0x52 :
			case 0x53 :
			case 0x54 :
			case 0x55 :
			case 0x56 :
			case 0x57 :
			case 0x58 :
			case 0x59 :
			case 0x5A :
			case 0x5B :
			case 0x5C :
			case 0x5D :
			case 0x5E :
			case 0x5F :
			case 0x60 :
			case 0x61 :
			case 0x62 :
			case 0x63 :
			case 0x64 :
			case 0x65 :
			case 0x66 :
			case 0x67 :
			case 0x68 :
			case 0x69 :
			case 0x6A :
			case 0x6B :
			case 0x6C :
			case 0x6D :
			case 0x6E :
			case 0x6F :
			case 0x70 :
			case 0x71 :
			case 0x72 :
			case 0x73 :
			case 0x74 :
			case 0x75 :
			case 0x76 :
			case 0x77 :
			case 0x78 :
			case 0x79 :
			case 0x7A :
			case 0x7B :
			case 0x7C :
			case 0x7D :
			case 0x7E :
			case 0x7F : {	// LD r,r'
				logAddrInst(PC, inst);
				std::clog << "LD " << rName(inst >> 3) << ',' << rName(inst) << std::endl;
				break;
			}

			case 0x80 :
			case 0x81 :
			case 0x82 :
			case 0x83 :
			case 0x84 :
			case 0x85 :
			case 0x86 :
			case 0x87 : {	// ADD A,r
				logAddrInst(PC, inst);
				std::clog << "ADD A," << rName(inst) << std::endl;
				break;
			}
			
			case 0x88 :
			case 0x89 :
			case 0x8A :
			case 0x8B :
			case 0x8C :
			case 0x8D :
			case 0x8E :
			case 0x8F : {	// ADC A,s
				logAddrInst(PC, inst);
				std::clog << "ADC A," << rName(inst) << std::endl;
				break;
			}
			
			case 0xA0 :
			case 0xA1 :
			case 0xA2 :
			case 0xA3 :
			case 0xA4 :
			case 0xA5 :
			case 0xA6 :
			case 0xA7 : {	// AND r (A ^= r)
				logAddrInst(PC, inst);
				std::clog << "AND " << rName(inst) << std::endl;
				break;
			}
			
			case 0xA8 :
			case 0xA9 :
			case 0xAA :
			case 0xAB :
			case 0xAC :
			case 0xAD :
			case 0xAE :
			case 0xAF : {	// XOR r (A (^)= r)
				logAddrInst(PC, inst);
				std::clog << "XOR " << rName(inst) << std::endl;
				break;
			}
	
			case 0xB0 :
			case 0xB1 :
			case 0xB2 :
			case 0xB3 :
			case 0xB4 :
			case 0xB5 :
			case 0xB6 :
			case 0xB7 : { 	// OR r ( A ^= r)
				logAddrInst(PC, inst);
				std::clog << "OR " << rName(inst) << std::endl;
				break;
			}

			case 0xB8 :
			case 0xB9 :
			case 0xBA :
			case 0xBB :
			case 0xBC :
			case 0xBD :
			case 0xBE :
			case 0xBF : { 	// CP r ( A - r)
				logAddrInst(PC, inst);
				std::clog << "CP " << rName(inst) << std::endl;
				break;
			}
			
			case 0xC0 :
			case 0xC8 :
			case 0xD0 :
			case 0xD8 :
			case 0xE0 :
			case 0xE8 :
			case 0xF0 :
			case 0xF8 : {	// RET cc
				logAddrInst(PC, inst);
				std::clog << "RET " << ccName(inst >> 3) << std::endl;
				break;
			}

			case 0xC1 :
			case 0xD1 :
			case 0xE1 :
			case 0xF1 : {	// POP qq
				logAddrInst(PC, inst);
				std::clog << "POP " << qqName(inst >> 4) << std::endl;
				break;
			}
	
			case 0xC2 :
			case 0xCA :
			case 0xD2 :
			case 0xDA :
			case 0xE2 :
			case 0xEA :
			case 0xF2 :
			case 0xFA : { 	// JP cc,addr
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "JP " << ccName(inst >> 3) << ',' << std::setw(4) << addr << 'h' << std::endl;
				break;
			}
	
			case 0xC3 : { 	// JP direct
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "JP " << std::setw(4) << addr << 'h' << std::endl;
				break;
			}

			case 0xC4 :
			case 0xCC :
			case 0xD4 :
			case 0xDC :
			case 0xE4 :
			case 0xEC :
			case 0xF4 :
			case 0xFC : { 	// CALL cc,addr
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "CALL " << ccName(inst >> 3) << ',' << std::setw(4) << addr << 'h' << std::endl;
				break;
			}

			case 0xC5 :
			case 0xD5 :
			case 0xE5 :
			case 0xF5 : { 	// PUSH qq
				logAddrInst(PC, inst);
				std::clog << "PUSH " << qqName(inst >> 4) << std::endl;
				break;
			}

			case 0xC6 : { // ADD A,n
				const uint8_t v = memory[PC+1];
				logAddrInst(PC, inst, v);
				std::clog << "ADD A," << std::dec << unsigned(v) << std::endl;
				break;
			}

			case 0xC9 : { 	// RET
				logAddrInst(PC, inst);
				std::clog << "RET" << std::endl;
				break;
			}
	
			case 0xCD : { 	// CALL nn
				const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
				logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
				std::clog << "CALL " << std::hex << std::setw(4) << addr << 'h' << std::endl;
				break;
			}
	
			case 0xD6 : { // SUB nn
				const uint8_t v = memory[PC+1];
				logAddrInst(PC, inst, v);
				std::clog << "SUB " << std::dec << unsigned(v) << std::endl;
				break;
			}

/*	
	// IX instructions 
	
			case 0xDD : {
				const uint8_t inst2 = memory[PC+1];
				switch (inst2) {
					case 0x2A : {	// LD IX,(nn)
						const uint16_t addr = memory[PC+3] * 256U + memory[PC+2];
	#if LOG
						logAddrInst(PC, inst, inst2, memory[PC+2], memory[PC+3]);
						std::clog << "LD IX,(" << std::hex << std::setw(4) << ")" << std::endl;
	#endif
						IX = memory[addr] + 256 * memory[addr+1];
						PC += 4;
						break;
					}
	
					case 0xE1 : {	// POP IX
	#if LOG
						logAddrInst(PC, inst, inst2);
						std::clog << "POP IX" << std::endl;
	#endif
						IX = memory[SP++];
						IX += memory[SP++] * 256;
						PC += 2;
						break;
					}
	
					case 0xE5 : {	// PUSH IX
	#if LOG
						logAddrInst(PC, inst, inst2);
						std::clog << "PUSH IX" << std::endl;
	#endif
						memory[--SP] = (IX >> 8);
						memory[--SP] = IX & 0x00FF;
						PC += 2;
						break;
					}
	
					default:
						logAddrInst(PC, inst, inst2, memory[PC+2]);
						std::clog << " : Unknown IX instruction!" << std::endl;
						
						throw(std::string("Not emulated instruction"));
						break;
				
				}
				break;
			}
*/	

			case 0xE6 : { // AND n
				const uint8_t v = memory[PC+1];
				logAddrInst(PC, inst, v);
				std::clog << "AND " << std::dec << unsigned(v) << std::endl;
				break;
			}
	
			case 0xE9 : {	// JP (HL)
				logAddrInst(PC, inst);
				std::clog << "JP (HL)" << std::endl;
				break;
			}

			case 0xEB : { 	// EX DE,HL
				logAddrInst(PC, inst);
				std::clog << "EX DE,HL" << std::endl;
				break;
			}

			case 0xED :
				logInstED(state);
				break;

			case 0xF3 : {	// DI
				logAddrInst(PC, inst);
				std::clog << "DI" << std::endl;
				break;
			}
	
			case 0xFB : {	// EI
				logAddrInst(PC, inst);
				std::clog << "EI" << std::endl;
				break;
			}

/*	
	// IY instructions 
	
			case 0xFD : {
				const uint8_t inst2 = memory[PC+1];
				switch (inst2) {
					case 0x2A : {	// LD IY,(nn)
						const uint16_t addr = memory[PC+3] * 256U + memory[PC+2];
	#if LOG
						logAddrInst(PC, inst, inst2, memory[PC+2], memory[PC+3]);
						std::clog << "LD IY,(" << std::hex << std::setw(4) << ")" << std::endl;
	#endif
						IY = memory[addr] + 256 * memory[addr+1];
						PC += 4;
						break;
					}
	
					case 0xE1 : {	// POP IY
	#if LOG
						logAddrInst(PC, inst, inst2);
						std::clog << "POP IY" << std::endl;
	#endif
						IY = memory[SP++];
						IY += memory[SP++] * 256;
						PC += 2;
						break;
					}
					
					case 0xE5 : {	// PUSH IX
	#if LOG
						logAddrInst(PC, inst, inst2);
						std::clog << "PUSH IY" << std::endl;
	#endif
						memory[--SP] = (IY >> 8);
						memory[--SP] = IY & 0x00FF;
						PC += 2;
						break;
					}
	
					default:
						logAddrInst(PC, inst, inst2, memory[PC+2]);
						std::clog << " : Unknown IY instruction!" << std::endl;
						
						throw(std::string("Not emulated instruction"));
						break;
				
				}
				break;
			}
*/

			case 0xF9 : {	// LD SP,HL
				logAddrInst(PC, inst);
				std::clog << "LD SP,HL" << std::endl;
				break;
			}

			case 0xFE : { // CP n (v - A ?)
				const uint8_t v = memory[PC+1];
				logAddrInst(PC, inst, v);
				std::clog << "CP " << std::dec << unsigned(v) << std::endl;
				break;
			}

			default:
				std::clog << std::hex << std::setw(4) << std::setfill('0') << PC << "\t";
				std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+0]) << ' ';
				std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+1]) << ' ';
				std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+2]) << "\t\t\t";
				std::clog << " : Unknown instruction in " << __FILE__ << ": " << S__LINE__ << " - " << __PRETTY_FUNCTION__ << std::endl;
				break;
	
		}
	}

	void logInstED(const ZZ80State& state) const {
		const uint16_t PC = state.Z_Z80_STATE_MEMBER_PC;
		const uint8_t inst = memory[PC];
		const uint8_t inst2 = memory[PC+1];
		
		switch (inst2) {
			case 0xB0 : {	// LDIR
				logAddrInst(PC, inst, inst2);
				std::clog << "LDIR" << std::endl;
				break;
			}

			case 0x42 :
			case 0x52 :
			case 0x62 :
			case 0x72 : {	// SBC HL,ss
				logAddrInst(PC, inst, inst2);
				std::clog << "SBC HL," << ddName(inst >> 4) << std::endl;
				break;
			}

			case 0x43 :
			case 0x53 :
			case 0x63 :
			case 0x73 : {	// LD (nn),dd
				const uint16_t addr = memory[PC+3] * 256U + memory[PC+2];
				logAddrInst(PC, inst, inst2, memory[PC+2], memory[PC+3]);
				std::clog << "LD (" << std::hex << std::setw(4) << addr << ")," << ddName(inst >> 4) << std::endl;
				break;
			}
			
			case 0x44 : {	// NEG
				logAddrInst(PC, inst, inst2);
				std::clog << "NEG" << std::endl;
				break;
			}
			
			case 0x4B :
			case 0x5B :
			case 0x6B :
			case 0x7B : {	// LD dd,(nn)
				const uint16_t addr = memory[PC+3] * 256U + memory[PC+2];
				logAddrInst(PC, inst, inst2, memory[PC+2], memory[PC+3]);
				std::clog << "LD " << ddName(inst >> 4) << ",(" << std::hex << std::setw(4) << addr << ")" << std::endl;
				break;
			}

			default:
				std::clog << std::hex << std::setw(4) << std::setfill('0') << PC << "\t";
				std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+0]) << ' ';
				std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+1]) << ' ';
				std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+2]) << "\t\t\t";
				std::clog << " : Unknown instruction in " << __FILE__ << ": " << S__LINE__ << " - " << __PRETTY_FUNCTION__ << std::endl;
				break;
		}
	}
	
	void logAddrInst(const uint16_t addr, const uint8_t inst) const {
		std::clog << std::hex << std::setw(4) << std::setfill('0') << addr << "\t";
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst) << "\t\t\t\t";
	}

	void logAddrInst(const uint16_t addr, const uint8_t inst1, const uint8_t inst2) const {
		std::clog << std::hex << std::setw(4) << std::setfill('0') << addr << "\t";
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst1) << ' ';
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst2) << "\t\t\t";
	}

	void logAddrInst(const uint16_t addr, const uint8_t inst1, const uint8_t inst2, const uint8_t inst3) const {
		std::clog << std::hex << std::setw(4) << std::setfill('0') << addr << "\t";
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst1) << ' ';
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst2) << ' ';
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst3) << "\t\t";
	}
	
	void logAddrInst(const uint16_t addr, const uint8_t inst1, const uint8_t inst2, const uint8_t inst3, const uint8_t inst4) const {
		std::clog << std::hex << std::setw(4) << std::setfill('0') << addr << "\t";
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst1) << ' ';
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst2) << ' ';
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst3) << ' ';
		std::clog << std::setw(2) << std::setfill('0') << unsigned(inst4) << "\t\t";
	}
	
	void logState(const ZZ80State& state) const {
		std::clog << "CPU state" << std::endl;
		std::clog << "A:" << std::hex << int(state.Z_Z80_STATE_MEMBER_A) << "h\t\t";
		std::clog << "Flags: S:" << (state.Z_Z80_STATE_MEMBER_F & 0x80) << 				\
			" Z:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x40) <<							\
			" Y:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x20) <<							\
			" H:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x10) <<							\
			" X:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x08) <<							\
			" P:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x04) <<							\
			" N:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x02) <<							\
			" C:" << bool(state.Z_Z80_STATE_MEMBER_F & 0x01);
		std::clog << std::endl;		
	}
	
	const std::string rName(const uint8_t r) const {
		switch (r & 0x07) {
			case 0x0 : return "B"; break;
			case 0x1 : return "C"; break;
			case 0x2 : return "D"; break;
			case 0x3 : return "E"; break;
			case 0x4 : return "H"; break;
			case 0x5 : return "L"; break;
			case 0x6 : return "(HL)"; break;
			case 0x7 : return "A"; break;
			default:
				return "Bad register!";
		}
	}

	const std::string ddName(const uint8_t dd) const {
		switch (dd & 0x03) {
			case 0x0 : return "BC"; break;
			case 0x1 : return "DE"; break;
			case 0x2 : return "HL"; break;
			case 0x3 : return "SP"; break;
			default:
				return "Bad 'dd' register!";
		}
	}
	
	const std::string qqName(const uint8_t dd) const {
		switch (dd & 0x03) {
			case 0x0 : return "BC"; break;
			case 0x1 : return "DE"; break;
			case 0x2 : return "HL"; break;
			case 0x3 : return "AF"; break;
			default:
				return "Bad 'qq' register!";
		}
	}
	
	const std::string ccName(const uint8_t cc) const {
		switch (cc & 0x07) {
			case 0x0 : return "NZ"; break;
			case 0x1 : return "Z";  break;
			case 0x2 : return "NC"; break;
			case 0x3 : return "C";  break;
			case 0x4 : return "PO"; break;
			case 0x5 : return "PE"; break;
			case 0x6 : return "P";  break;
			case 0x7 : return "M";  break;
			default:
				return "Bad 'cc' register!";
		}
	}
	
	bool parity(const uint8_t N) {
		uint8_t y = N ^ (N >> 1);
		y = y ^ (y >> 2);
		y = y ^ (y >> 4);
		y = y ^ (y >> 8);
		y = y ^ (y >> 16);
		return (y & 1);
	}	
	
private:
	static constexpr uint16_t BIAS = 0xA800;			// 64k -> B000
	static constexpr uint16_t MEMORY_SIZE = 62;	// Ko
	
	Z80 cpu;
	uint8_t memory[MEMORY_SIZE * 1024];
};
