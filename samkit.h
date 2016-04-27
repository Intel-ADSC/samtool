// Version 1.0 - 10/01/2014: Initial Release
//
//===========================================================
/*
 * This file contains HW Access routines.  Part of samtool, examole, template, and answer.
 * samtool.c: Simple command-line program to read/write MMIO, io, pci, and msr addresses
 *
 *  Copyright (C) 2015, Intel Corporation
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see <http://www.gnu.org/licenses/>.
 */
// ==========================================================
// Sam Routines
//===========================================================
// PCI Read Routines
u8 SHFpci_read_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg);
u16 SHFpci_read_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg);
u32 SHFpci_read_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg);

u8 IOpci_read_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg);
u16 IOpci_read_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg);
u32 IOpci_read_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg);

// PCI Write Routines
void SHFpci_write_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u8 u8_data);
void SHFpci_write_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u16 u16_data);
void SHFpci_write_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u32 u32_data);

void IOpci_write_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u8 u8_data);
void IOpci_write_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u16 u16_data);
void IOpci_write_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u32 u32_data);

//PCIe Read Routines
u8 SHFpcie_read_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u64 base_address);
u16 SHFpcie_read_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u64 base_address);
u32 SHFpcie_read_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u64 base_address);

// PCIe Write Routines
void SHFpcie_write_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u8 u8_data, u64 base_address);
void SHFpcie_write_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u16 u16_data, u64 base_address);
void SHFpcie_write_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u32 u32_data, u64 base_address);

//===========================================================
// Memory Read Routines
// Note:  I've only managed to get these Memory routines to work with MMIO addresses.
// They do NOT work with DRAM Memory addresses.
// I've managed to access LAN Card Memory addresses, but only if I first execute the commands:
//   sudo rmmod modname (found from using lsmod command - usually e1000e).
// Sorries, but it's the best I have at this point.
u8  SHFmem_read_byte  (u64 passed_address);
u16 SHFmem_read_word  (u64 passed_address);
u32 SHFmem_read_dword (u64 passed_address);
u64 SHFmem_read_qword (u64 passed_address);  

// Memory Write Routines
void SHFmem_write_byte  (u64 passed_address, u8 u8_data);
void SHFmem_write_word  (u64 passed_address, u16 u16_data);
void SHFmem_write_dword (u64 passed_address, u32 u32_data);

//===========================================================
// I/O Read Routines
u8 SHF_IO_read_byte(u64 passed_address);
u16 SHF_IO_read_word(u64 passed_address);
u32 SHF_IO_read_dword(u64 passed_address);

// I/O Write Routines
void SHF_IO_write_byte(u64 passed_address, u8 u8_data);
void SHF_IO_write_word(u64 passed_address, u16 u16_data);
void SHF_IO_write_dword(u64 passed_address, u32 u32_data);

//===========================================================
// MSR Read Routine
int SHF_rdmsr(int CPU_number, unsigned int MsrNum, unsigned long long *MsrVal);
// NOTE:  YOU MUST RUN sudo modprobe msr before running these commands
// NOTE:  You MIGHT have to run sudo -s before running thes commands

// MSR Write Routine
int SHF_wrmsr (int CPU_number, unsigned int MsrNum, unsigned long long *MsrVal);
// This write MSR command does not seem to be working
// Leaving it here for now......but is...erroneous.

//===========================================================
u64 Read_HPET();
// Read from the South Bridge's HPET timer.

//===========================================================
u64 rdtsc(void);
// Read the CPU's Time Stamp Counter

//===========================================================
double Freq_Calc();
// Calculates the CPU frequency via the HPET and TSC.

//===========================================================
double tsc_delay(u64 start_time, u64 end_time, char *units, double input_freq);
// This routine takes two values previously read by the "rdtsc"
// routine and finds the time delta (uses CPU Frequency)
// Note, the char *units can be:
//	"clocks"
//	"cycles"
//	"sec"
//	"ms"
//	"us"
//	"ns"
//
// Note, if input_freq is PASSED, then don't need to run the frequency test, drastically
// speeding things up!

//===========================================================
void SHFprint(u64 number, int min_length, int base, char *before, char *after);
// This prints out the value of number.
// 	min_length gives the minimum number of digits (=0x08 for DWORD, for example)
//	base is the base (base 16, base 10, etc)
// 	*before and *after are strings to print before and after the given number

//===========================================================
double assembly_delay(u64 passed_address, char *units, u32 *read_result, double input_freq);
// This routine reads from the passed address, and uses the timestamp counter
// to time how long it takes.
// What makes this routine UNIQUE is that it's written in assembly.  Theoretically this
// is as fast as a MEMORY read can happen.
// Note, the char *units can be:
//	"clocks"
//	"cycles"
//	"sec"
//	"ms"
//	"us"
//	"ns"
//
// read_result = the read data.
// Note, if input_freq is PASSED, then don't need to run the frequency test, drastically
// speeding things up!

//===========================================================
void neg_add_one(u32 *i);
//float Xassembly_delay(u64 passed_address, char *units, u32 *read_result, float input_freq);
// Just test routines - ignore

//===========================================================
double read_assembly_delay(u64 passed_address, char *units, double input_freq, u64 byte_length, u8 array1[], u8 size); 

//===========================================================
double write_assembly_delay(u64 passed_address, char *units, double input_freq, u64 byte_length, u8 array1[], u8 size); 

//===========================================================
double block_read_assembly_delay_new(u64 passed_address, char *units, double input_freq, u64 number_4K_blocks, u8 array1[]);
// This routine reads from the passed address, and uses the timestamp counter
// to time how long it takes.
// What makes this routine UNIQUE is that it's written in assembly.  Theoretically this
// is as fast as a MEMORY read can happen.
// Note, the char *units can be:
//	"clocks"
//	"cycles"
//	"sec"
//	"ms"
//	"us"
//	"ns"
//
// array1[] - Array of data passed in/out (used for block transfers)
// Note, if input_freq is PASSED, then don't need to run the frequency test, drastically
// speeding things up!

//===========================================================
double block_write_assembly_delay_new(u64 passed_address, char *units, double input_freq, u64 number_4K_blocks, u8 array1[]);
// This routine reads from the passed address, and uses the timestamp counter
// to time how long it takes.
// What makes this routine UNIQUE is that it's written in assembly.  Theoretically this
// is as fast as a MEMORY read can happen.
// Note, the char *units can be:
//	"clocks"
//	"cycles"
//	"sec"
//	"ms"
//	"us"
//	"ns"
//
// array1[] - Array of data passed in/out (used for block transfers)
// Note, if input_freq is PASSED, then don't need to run the frequency test, drastically
// speeding things up!

//===========================================================
void SHF_wrmsr_new(u64 passed_address, u64 data);

//===========================================================
unsigned int PCI_Device_Found_and_Size(unsigned long bus, unsigned long device, unsigned long function);
unsigned int PCIE_Device_Found_and_Size(unsigned long bus, unsigned long device, unsigned long function, u64 base_address);


