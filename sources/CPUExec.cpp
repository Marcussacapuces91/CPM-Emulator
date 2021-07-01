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

#include "Z80.h"


void Z80_Computer::CPUExec(const uint8_t inst) {
	switch (inst) {
		
/*		
		case 0x00 : { // NOP
#if LOG
			logAddrInst(PC, inst);
			std::clog << "NOP " << std::endl;
#endif
			++PC;
			break;
		}
*/			
		case 0x01 : 
		case 0x11 : 
		case 0x21 : 
		case 0x31 : {	// LD dd,nn
			const uint16_t nn = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "LD " << ddName(inst >> 4) << ',' << std::dec << unsigned(nn) << std::endl;
#endif
			switch (inst & 0x30) {
				case 0x00 :	BC = nn; break;
				case 0x10 :	DE = nn; break;
				case 0x20 :	HL = nn; break;
				case 0x30 :	SP = nn; break;
			}
			PC += 3;
			break;
		}
/*
		case 0x02 : 
		case 0x12 : {	// LD (rr),A
#if LOG
			logAddrInst(PC, inst);
			std::clog << "LD (" << ddName(inst >> 4) << "),A" << std::endl;
#endif
			memory[(inst == 0x02) ? BC : DE] = A;
			++PC;
			break;
		}
*/
		case 0x03 :
		case 0x13 : 
		case 0x23 : 
		case 0x33 : {	// INC ss
#if LOG
			logAddrInst(PC, inst);
			std::clog << "INC " << ddName(inst >> 4) << std::endl;
#endif
			switch (inst & 0x30) {
				case 0x00 :	++BC; break;
				case 0x10 :	++DE; break;
				case 0x20 :	++HL; break;
				case 0x30 :	++SP; break;
			}
			// no conditions bits affected
			++PC;
			break;
		}
/*		
		case 0x04 :
		case 0x0C :
		case 0x14 :
		case 0x1C :
		case 0x24 :
		case 0x2C :
		case 0x34 :
		case 0x3C : {	// INC r
#if LOG
			logAddrInst(PC, inst);
			std::clog << "INC " << rName(inst >> 3);
#endif
			uint8_t* r;
			switch (inst & 0x38) {
				case 0x00 : r = &B; break;
				case 0x08 : r = &C; break;
				case 0x10 : r = &D; break;
				case 0x18 : r = &E; break;
				case 0x20 : r = &H; break;
				case 0x28 : r = &L; break;
				case 0x30 : r = &memory[HL]; break;
				case 0x38 : r = &A; break;
			}
//			add(*r, 1);
						
			flags.H = ((*r & 0x0F) == 0x0F);
			flags.PV = (*r == 0x7F);
			++(*r);
			flags.S = (*r & 0x80);
			flags.Z = !(*r);
			flags.N = false;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(*r);
			logFlags();
			std::clog << std::endl;
#endif
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
#if LOG
			logAddrInst(PC, inst);
			std::clog << "DEC " << rName(inst >> 3);
#endif
			uint8_t* r;
			switch (inst & 0x38) {
				case 0x00 : r = &B; break;
				case 0x08 : r = &C; break;
				case 0x10 : r = &D; break;
				case 0x18 : r = &E; break;
				case 0x20 : r = &H; break;
				case 0x28 : r = &L; break;
				case 0x30 : r = &memory[HL]; break;
				case 0x38 : r = &A; break;
			}
			flags.H = !(*r & 0x0F);
			flags.PV = (*r == 0x80);
			--(*r);
			flags.S = (*r & 0x80);
			flags.Z = !(*r);
			flags.N = true;
			flags.C = false;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(*r);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
*/
		case 0x06 : 
		case 0x0E : 
		case 0x16 : 
		case 0x1E : 
		case 0x26 : 
		case 0x2E : 
		case 0x36 : 
		case 0x3E : {	// LD r,n & LD (HL),n
			const uint8_t n = memory[PC+1];
#if LOG
			logAddrInst(PC, inst, n);
			std::clog << "LD " << rName(inst >> 3) << ',' << std::dec << unsigned(n) << std::endl;
#endif
			switch (inst & 0x38) {
				case 0x00 : B = n; break;
				case 0x08 : C = n; break;
				case 0x10 : D = n; break;
				case 0x18 : E = n; break;
				case 0x20 : H = n; break;
				case 0x28 : L = n; break;
				case 0x30 : memory[HL] = n; break;
				case 0x38 : A = n; break;
			}
			PC += 2;
  			break;
		}
/*		
		case 0x07 : {	// RLCA
#if LOG
			logAddrInst(PC, inst);
			std::clog << "RLCA" << std::endl;
#endif
			const bool b7 = (A & 0x80);
			A = (A << 1) | b7;
			flags.H = false;
			flags.N = false;
			flags.C = b7;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
*/		
		case 0x09 :
		case 0x19 :
		case 0x29 :
		case 0x39 : {	// ADD HL,ss
#if LOG
			logAddrInst(PC, inst);
			std::clog << "ADD HL," << ddName(inst >> 4);
#endif
			const uint16_t* ss;
			switch (inst & 0x30) {
				case 0x00 : ss = &BC; break;
				case 0x10 : ss = &DE; break;
				case 0x20 : ss = &HL; break;
				case 0x30 : ss = &SP; break;
			}
			const unsigned lb = unsigned(HL & 0x0FFF) + unsigned(*ss & 0x0FFF);
			flags.H = (lb & 0x1000);
			const unsigned hb = unsigned(HL >> 12) + unsigned(*ss >> 12) + flags.H;
			flags.C = (hb & 0x10);
			flags.N = false;
			HL = (hb << 12) + (lb & 0xFFF);
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(HL);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
/*
		case 0x0A :
		case 0x1A : {	// LD A,(dd)
#if LOG
			logAddrInst(PC, inst);
			std::clog << "LD A,(" << ddName(inst >> 4) << ")" << std::endl;
#endif
			A = memory[(inst == 0x0A ? BC : DE)];
			++PC;
			break;
		}
*/		
		case 0x0B :
		case 0x1B : 
		case 0x2B : 
		case 0x3B : {	// DEC ss
#if LOG
			logAddrInst(PC, inst);
			std::clog << "DEC " << ddName(inst >> 4) << std::endl;
#endif
			switch (inst & 0x30) {
				case 0x00 :	--BC; break;
				case 0x10 :	--DE; break;
				case 0x20 :	--HL; break;
				case 0x30 :	--SP; break;
			}
			++PC;
			break;
		}
/*		
		case 0x0F : { 	// RRCA
#if LOG
			logAddrInst(PC, inst);
			std::clog << "RRCA";
#endif
			const bool b0 = (A & 0x01);
			A = (A >> 1) | (b0 << 7);
			flags.H = false;
			flags.N = false;
			flags.C = b0;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}

		case 0x17 : { 	// RLA
#if LOG
			logAddrInst(PC, inst);
			std::clog << "RLA";
#endif
			const bool b7 = (A & 0x80);
			A = (A << 1) | (flags.C);
			flags.H = false;
			flags.N = false;
			flags.C = b7;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
		
		case 0x1F : { 	// RRA
#if LOG
			logAddrInst(PC, inst);
			std::clog << "RRA ";
#endif
			const bool b0 = (A & 0x01);
			A = (A >> 1) | (flags.C << 7);
			flags.C = b0;
			flags.H = false;
			flags.N = false;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
		
		case 0x22 : {	// LD (addr), HL
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "LD (" << std::hex << std::setw(4) << std::setfill('0') << addr << "h),HL" << std::endl;
#endif
			memory[addr] = L;
			memory[addr+1] = H;
			PC += 3;
			break;
		}
*/			
		case 0x2A : {	// LD HL,(nn)
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "LD HL,(" << std::hex << std::setw(4) << std::setfill('0') << addr << "h)";
#endif
			HL = memory[addr] + 256U * memory[addr+1];
			PC += 3;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << HL << std::endl;
#endif			
			break;
		}
			
		case 0x32 : {	// LD (nn),A
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "LD (" << std::setw(4) << std::setfill('0') << addr << "h),A" << std::endl;
#endif
			memory[addr] = A;
			PC += 3;
			break;
		}

		case 0x3A : {	// LD A,(addr)
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "LD A,(" << std::setw(4) << std::setfill('0') << addr << "h)" << std::endl;
#endif
			A = memory[addr];
			PC += 3;
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
#if LOG
			logAddrInst(PC, inst);
			std::clog << "LD " << rName(inst >> 3) << ',' << rName(inst);
#endif
			const uint8_t* rp;
			switch (inst & 0x07) {
				case 0x00 : rp = &B; break;
				case 0x01 : rp = &C; break;
				case 0x02 : rp = &D; break;
				case 0x03 : rp = &E; break;
				case 0x04 : rp = &H; break;
				case 0x05 : rp = &L; break;
				case 0x06 : rp = memory + HL; break;
				case 0x07 : rp = &A; break;
			}
			switch (inst & 0x38) {
				case 0x00 :	B = *rp; break;
				case 0x08 :	C = *rp; break;
				case 0x10 :	D = *rp; break;
				case 0x18 :	E = *rp; break;
				case 0x20 :	H = *rp; break;
				case 0x28 :	L = *rp; break;
				case 0x30 : memory[HL] = *rp; break;
				case 0x38 :	A = *rp; break;
			}
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << unsigned(*rp) << std::endl;
#endif
			break;
		}
/*		
		case 0x80 :
		case 0x81 :
		case 0x82 :
		case 0x83 :
		case 0x84 :
		case 0x85 :
		case 0x86 :
		case 0x87 : {	// ADD A,r
#if LOG
			logAddrInst(PC, inst);
			std::clog << "ADD A," << rName(inst) << std::endl;
#endif
			const uint8_t* r;
			switch (inst & 0x07) {
				case 0x00 : r = &B; break;
				case 0x01 : r = &C; break;
				case 0x02 : r = &D; break;
				case 0x03 : r = &E; break;
				case 0x04 : r = &H; break;
				case 0x05 : r = &L; break;
				case 0x06 : r = &memory[HL]; break;
				case 0x07 : r = &A; break;
			}
			A = add(A, *r);
			++PC;
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
#if LOG
			logAddrInst(PC, inst);
			std::clog << "ADC A," << rName(inst) << std::endl;
#endif
			const uint8_t* r;
			switch (inst & 0x07) {
				case 0x00 : r = &B; break;
				case 0x01 : r = &C; break;
				case 0x02 : r = &D; break;
				case 0x03 : r = &E; break;
				case 0x04 : r = &H; break;
				case 0x05 : r = &L; break;
				case 0x06 : r = &memory[HL]; break;
				case 0x07 : r = &A; break;
			}
			A = add(A, *r, flags.C);
			++PC;
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
#if LOG
			logAddrInst(PC, inst);
			std::clog << "AND " << rName(inst);
#endif
			switch (inst & 0x07) {
				case 0x0 : A &= B; break;
				case 0x1 : A &= C; break;
				case 0x2 : A &= D; break;
				case 0x3 : A &= E; break;
				case 0x4 : A &= H; break;
				case 0x5 : A &= L; break;
				case 0x6 : A &= memory[HL]; break;
				case 0x7 : A &= A; break;
			}
			flags.S = (A & 0x80);
			flags.Z = (A == 0);
			flags.H = true;		// H is set (!)
			flags.PV = even(A);
			flags.N = false;
			flags.C = false;
			++PC;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
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
#if LOG
			logAddrInst(PC, inst);
			std::clog << "XOR " << rName(inst);
#endif
			switch (inst & 0x07) {
				case 0x0 : A ^= B; break;
				case 0x1 : A ^= C; break;
				case 0x2 : A ^= D; break;
				case 0x3 : A ^= E; break;
				case 0x4 : A ^= H; break;
				case 0x5 : A ^= L; break;
				case 0x6 : A ^= memory[HL]; break;
				case 0x7 : A ^= A; break;
			}
			flags.S = (A & 0x80);
			flags.Z = (A == 0);
			flags.H = false;
			flags.PV = even(A);
			flags.N = false;
			flags.C = false;
			PC++;			
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}

// 0xB0
*/		
		case 0xB0 :
		case 0xB1 :
		case 0xB2 :
		case 0xB3 :
		case 0xB4 :
		case 0xB5 :
		case 0xB6 :
		case 0xB7 : { 	// OR r ( A ^= r)
#if LOG
			logAddrInst(PC, inst);
			std::clog << "OR " << rName(inst);
#endif
			switch (inst & 0x07) {
				case 0x0 : A |= B; break;
				case 0x1 : A |= C; break;
				case 0x2 : A |= D; break;
				case 0x3 : A |= E; break;
				case 0x4 : A |= H; break;
				case 0x5 : A |= L; break;
				case 0x6 : A |= memory[HL]; break;
				case 0x7 : A |= A; break;
			}
			flags.S = (A & 0x80);
			flags.Z = !A;
			flags.H = false; 	// is reset
			flags.PV = even(A);
			flags.N = false;
			flags.C = false;
			PC++;			
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
/*		
		case 0xB8 :
		case 0xB9 :
		case 0xBA :
		case 0xBB :
		case 0xBC :
		case 0xBD :
		case 0xBE :
		case 0xBF : { 	// CP r ( A - r)
#if LOG
			logAddrInst(PC, inst);
			std::clog << "CP " << rName(inst);
#endif
			const uint8_t* r;
			switch (inst & 0x07) {
				case 0x0 : r = &B; break;
				case 0x1 : r = &C; break;
				case 0x2 : r = &D; break;
				case 0x3 : r = &E; break;
				case 0x4 : r = &H; break;
				case 0x5 : r = &L; break;
				case 0x6 : r = &memory[HL]; break;
				case 0x7 : r = &A; break;
			}
			flags.S = ((A - *r) & 0x80);
			flags.Z = (A == *r);
			flags.H = (int(A & 0x0F) - int(*r & 0x0F)) < 0;
			flags.PV = ((A - *r) > 0x7F) | ((A - *r) < -0x7E);
			flags.N = true;
			flags.C = (int(A) - int(*r)) < 0;
			PC++;			
#if LOG
			std::clog << "\t\t\t? "<< std::dec << int(*r);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
		
// 0xC0
		
		case 0xC0 :
		case 0xC8 :
		case 0xD0 :
		case 0xD8 :
		case 0xE0 :
		case 0xE8 :
		case 0xF0 :
		case 0xF8 : {	// RET cc
#if LOG
			logAddrInst(PC, inst);
			std::clog << "RET ";
			switch (inst & 0x38) {
				case 0x00 :  std::clog << "NZ"; break;
				case 0x08 :  std::clog << "Z";  break;
				case 0x10 :  std::clog << "NC"; break;
				case 0x18 :  std::clog << "C";  break;
				case 0x20 :  std::clog << "PO"; break;
				case 0x28 :  std::clog << "PE"; break;
				case 0x30 :  std::clog << "P";  break;
				case 0x38 :  std::clog << "M";  break;
			}
			std::clog << std::endl;
#endif
			switch (inst & 0x38) {
				case 0x00 :	if (!flags.Z) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x08 :	if (flags.Z) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x10 :	if (!flags.C) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x18 :	if (flags.C) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x20 :	if (!flags.PV) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x28 :	if (flags.PV) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x30 :	if (!flags.S) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
				case 0x38 :	if (flags.S) PC = memory[SP++] + 256 * memory[SP++]; else PC++; break;
			}
			break;
		}
*/		
		case 0xC1 :
		case 0xD1 :
		case 0xE1 :
		case 0xF1 : {	// POP qq
#if LOG
			logAddrInst(PC, inst);
			std::clog << "POP " << qqName(inst >> 4) << std::endl;
#endif
			switch (inst & 0x30) {
				case 0x00 :  C = memory[SP++]; B = memory[SP++]; break;
				case 0x10 :  E = memory[SP++]; D = memory[SP++]; break;
				case 0x20 :  L = memory[SP++]; H = memory[SP++]; break;
				case 0x30 :  FLAGS = memory[SP++]; A = memory[SP++]; break;
			}
			++PC;
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
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "JP ";
			switch (inst & 0x38) {
				case 0x00 :	std::clog << "NZ"; break;
				case 0x08 :	std::clog << "Z"; break;
				case 0x10 :	std::clog << "NC";  break;
				case 0x18 :	std::clog << "C";  break;
				case 0x20 :	std::clog << "PO"; break;
				case 0x28 :	std::clog << "PE"; break;
				case 0x30 :	std::clog << "P";  break;
				case 0x38 :	std::clog << "M";  break;
			}
			std::clog << ',' << std::setw(4) << addr << 'h' << std::endl;
#endif
			switch (inst & 0x38) {
				case 0x00 :	PC = (!flags.Z  ? addr : PC + 3); break;
				case 0x08 :	PC = (flags.Z   ? addr : PC + 3); break;
				case 0x10 :	PC = (!flags.C  ? addr : PC + 3); break;
				case 0x18 :	PC = (flags.C   ? addr : PC + 3); break;
				case 0x20 :	PC = (!flags.PV ? addr : PC + 3); break;
				case 0x28 :	PC = (flags.PV  ? addr : PC + 3); break;
				case 0x30 :	PC = (!flags.S  ? addr : PC + 3); break;
				case 0x38 :	PC = (flags.S   ? addr : PC + 3); break;
			}
			break;
		}

		case 0xC3 : { 	// JP direct
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "JP " << std::setw(4) << addr << 'h' << std::endl;
#endif
			PC = addr;
			break;
		}
/*			
		case 0xC4 :
		case 0xCC :
		case 0xD4 :
		case 0xDC :
		case 0xE4 :
		case 0xEC :
		case 0xF4 :
		case 0xFC : { 	// CALL cc,addr
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "CALL ";
			switch (inst & 0x38) {
				case 0x00 :	std::clog << "NZ"; break;
				case 0x08 :	std::clog << "Z";  break;
				case 0x10 :	std::clog << "NC"; break;
				case 0x18 :	std::clog << "C";  break;
				case 0x20 :	std::clog << "PO"; break;
				case 0x28 :	std::clog << "PE"; break;
				case 0x30 :	std::clog << "P";  break;
				case 0x38 :	std::clog << "M";  break;
			}
			std::clog << ',' << std::setw(4) << addr << 'h' << std::endl;
#endif
			switch (inst & 0x38) {
				case 0x00 :	if (!flags.Z) {	memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x08 :	if (flags.Z) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x10 :	if (!flags.C) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x18 :	if (flags.C) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x20 :	if (!flags.PV) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x28 :	if (flags.PV) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x30 :	if (!flags.S) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
				case 0x38 :	if (flags.S) { memory[--SP] = (PC+3) >> 8; memory[--SP] = (PC+3) & 0xFF; PC = addr; } else PC +=3; break;
			}
			break;
		}
*/			
		case 0xC5 :
		case 0xD5 :
		case 0xE5 :
		case 0xF5 : { 	// PUSH qq
#if LOG
			logAddrInst(PC, inst);
			std::clog << "PUSH " << qqName(inst >> 4) << std::endl;
#endif
			switch (inst & 0x30) {
				case 0x00 : memory[--SP] = B; memory[--SP] = C; break;
				case 0x10 : memory[--SP] = D; memory[--SP] = E; break;
				case 0x20 : memory[--SP] = H; memory[--SP] = L; break;
				case 0x30 : memory[--SP] = A; memory[--SP] = FLAGS; break;
			}
			++PC;
			break;
		}
/*		
		case 0xC6 : { // ADD A,n
			const uint8_t v = memory[PC+1];
#if LOG
			logAddrInst(PC, inst, v);
			std::clog << "ADD A," << std::dec << unsigned(v) << std::endl;
#endif
			A = add(A, v);
			PC += 2;
			break;
		}
*/		
		case 0xC9 : { 	// RET
#if LOG
			logAddrInst(PC, inst);
			std::clog << "RET" << std::endl;
#endif
			PC = memory[SP++];
			PC += 256U * memory[SP++];
			break;
		}

		case 0xCD : { 	// CALL nn
			const uint16_t addr = memory[PC+2] * 256U + memory[PC+1];
#if LOG
			logAddrInst(PC, inst, memory[PC+1], memory[PC+2]);
			std::clog << "CALL " << std::hex << std::setw(4) << addr << 'h' << std::endl;
#endif
			memory[--SP] = (PC+3) >> 8;		// hB return addr
			memory[--SP] = (PC+3) & 0xFF;	// lB return addr
			PC = addr;
			break;
		}
/*
// 0xD0

		case 0xD6 : { // SUB nn
			const uint8_t v = memory[PC+1];
#if LOG
			logAddrInst(PC, inst, v);
			std::clog << "SUB " << std::dec << unsigned(v);
#endif
			A = sub(A, v);
			PC += 2;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}

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

// 0xE0

		case 0xE6 : { // AND n
			const uint8_t v = memory[PC+1];
#if LOG
			logAddrInst(PC, inst, v);
			std::clog << "AND " << std::dec << unsigned(v);
#endif
			A &= v;
			flags.S = (A & 0x80);
			flags.Z = (A == 0);
			flags.H = true;
			flags.PV = even(A);
			flags.N = false;
			flags.C = false;
			PC += 2;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}

		case 0xE9 : {	// JP (HL)
#if LOG
			logAddrInst(PC, inst);
			std::clog << "JP (HL)" << std::endl;
#endif
			PC = HL;
			break;
		}
*/
		case 0xEB : { 	// EX DE,HL
#if LOG
			logAddrInst(PC, inst);
			std::clog << "EX DE,HL" << std::endl;
#endif
			const uint16_t r = DE;
			DE = HL;
			HL = r;
			++PC;
			break;
		}

// Extendeds instructions
		
		case 0xED : {	// Extended instructions (ED)
			const uint8_t inst2 = memory[PC+1];
			switch (inst2) {
				case 0xB0 : {	// LDIR
#if LOG
					logAddrInst(PC, inst, inst2);
					std::clog << "LDIR";
#endif
					do {
						memory[HL++] = memory[DE++];
						BC--;
					} while (BC);
					flags.H = false;
					flags.PV = true;
					flags.N = false;
					PC += 2;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
					break;
				}
/*				
				case 0x42 :
				case 0x52 :
				case 0x62 :
				case 0x72 : {	// SBC HL,ss
#if LOG
					logAddrInst(PC, inst, inst2);
					std::clog << "SBC HL," << ddName(inst >> 4);
#endif
					const uint16_t* ss;
					switch (inst2 & 0x30) {
						case 0x00 : ss = &BC; break;
						case 0x10 : ss = &DE; break;
						case 0x20 : ss = &HL; break;
						case 0x30 : ss = &SP; break;
					}
					const long v = long(HL) - long(*ss) - flags.C;
					const bool pv = ((HL & 0x8000) != (*ss & 0x8000)) && ((HL & 0x8000) != (v & 0x8000));
					
					HL = (v & 0xFFFF);
					flags.S = (HL & 0x8000);
					flags.Z = (HL == 0);
					flags.H = false; // n'imp.
					flags.PV = pv;
					flags.N = true;
					flags.C = (v & 0x10000);
					PC += 2;
#if LOG
			std::clog << "\t\t\t<- "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
					break;
				}

				case 0x43 :
				case 0x53 :
				case 0x63 :
				case 0x73 : {	// LD (nn),dd
					const uint16_t addr = memory[PC+3] * 256U + memory[PC+2];
#if LOG
					logAddrInst(PC, inst, inst2, memory[PC+2], memory[PC+3]);
					std::clog << "LD (" << std::hex << std::setw(4) << addr << ")," << ddName(inst >> 4) << std::endl;
#endif
					switch (inst2 & 0x30) {
						case 0x00 : memory[addr+1] = B; memory[addr] = C; break;
						case 0x10 : memory[addr+1] = D; memory[addr] = E; break;
						case 0x20 : memory[addr+1] = H; memory[addr] = L; break;
						case 0x30 : memory[addr+1] = (SP >> 8); memory[addr] = (SP & 0x00FF); break;
					}
					PC += 4;
					break;
				}
				
				case 0x44 : {	// NEG
#if LOG
					logAddrInst(PC, inst, inst2);
					std::clog << "NEG";
#endif					
					const unsigned i = -unsigned(A & 0x0F);
					const unsigned j = -unsigned(A);
					A = j;
					flags.S = (j & 0x80);
					flags.Z = (A == 0);
					flags.H = (i & 0x10);
					flags.PV = (A == 0x80);
					flags.N = true;
					flags.C = !A;
					PC += 2;
#if LOG
					std::clog << "\t\t\t<- "<< std::dec << int(A);
					logFlags();
					std::clog << std::endl;
#endif
					break;
				}
				
				case 0x4B :
				case 0x5B :
				case 0x6B :
				case 0x7B : {	// LD dd,(nn)
					const uint16_t addr = memory[PC+3] * 256U + memory[PC+2];
#if LOG
					logAddrInst(PC, inst, inst2, memory[PC+2], memory[PC+3]);
					std::clog << "LD " << ddName(inst >> 4) << ",(" << std::hex << std::setw(4) << addr << ")" << std::endl;
#endif
					switch (inst2 & 0x30) {
						case 0x00 : BC = memory[addr+1] * 256U + memory[addr]; break;
						case 0x10 : DE = memory[addr+1] * 256U + memory[addr]; break;
						case 0x20 : HL = memory[addr+1] * 256U + memory[addr]; break;
						case 0x30 : SP = memory[addr+1] * 256U + memory[addr]; break;
					}
					PC += 4;
					break;
				}
*/
				default:
					logAddrInst(PC, inst, inst2, memory[PC+2]);
					std::clog << " : Unknown extended instruction!" << std::endl;
					
					throw(std::string("Not emulated instruction"));
					break;
					
			}
			break;
		}
/*
// 0xF0

		case 0xF3 : {	// DI
#if LOG
			logAddrInst(PC, inst);
			std::clog << "DI\t\t\t\t@warning NO ACTION !" << std::endl;
#endif
			++PC;
			break;
		}

		case 0xFB : {	// EI
#if LOG
			logAddrInst(PC, inst);
			std::clog << "EI\t\t\t\t@warning NO ACTION !" << std::endl;
#endif
			++PC;
			break;
		}

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
#if LOG
			logAddrInst(PC, inst);
			std::clog << "LD SP,HL";
#endif
			SP = HL;
#if LOG
			std::clog << "\t\t\t<- "<< std::hex << unsigned(SP) << std::endl;
#endif					
			++PC;
			break;
		}
/*
		case 0xFE : { // CP n (v - A ?)
			const uint8_t v = memory[PC+1];
#if LOG
			logAddrInst(PC, inst, v);
			std::clog << "CP " << std::dec << unsigned(v);
#endif
			sub(A, v);
			PC += 2;
#if LOG
			std::clog << "\t\t\t? "<< std::dec << int(A);
			logFlags();
			std::clog << std::endl;
#endif
			break;
		}
*/
		default:
			std::clog << std::hex << std::setw(4) << std::setfill('0') << PC << "\t";
			std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+0]) << ' ';
			std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+1]) << ' ';
			std::clog << std::setw(2) << std::setfill('0') << unsigned(memory[PC+2]) << "\t\t\t";
			std::clog << " : Unknown instruction!" << std::endl;
			
			throw(std::string("Not emulated instruction"));
			break;

	}
}

void Z80_Computer::CPUExecExtended(const uint8_t inst) {
	switch (inst) {
	}
}

