// You will need to bring in some external libraries:
//	MUST BE LOGGED INTO INTERNET
//		sudo apt-get install libpci-dev
//		sudo apt-cache policy pciutils
//		sudo apt-get update
//		sudo apt-get install msr-tools
//	32 bit version needed:
//		sudo apt-get install libpci-dev:i386
//		sudo modprobe msr
//		sudo rdmsr 0x198  // tests the rdmsr library
//
// To Compile with another program using these routines (example.c for example):
// 		gcc -Wall -W -Werror -g example.c samkit.c -lpci -lm -o example
//
//	32 bit compile:
// 		gcc -m32 -Wall -W -Werror -g example.c samkit.c -lpci -lm -o example
//
// To Run:
// 		sudo modprobe msr
// 		sudo ./example
//
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

#include <sys/mman.h>
#include <pci/pci.h>    // ** Must use -lm compile option **
#include <fcntl.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>		// exit
#include <unistd.h>		// close

// Sam Crap Starts Here
#include <string.h>     // for strlen
#include <math.h>       // need for pow (exponent) ** Must use -lm compile option **
#include <sys/io.h>		// Permits access to IO Locations

//===========================================================
// Sam Routines
#include "samkit.h"   // My stuff

//===========================================================
// Defines
#define FATAL do { fprintf(stderr, "Error at line %d, file %s (%d) [%s]\n", \
  __LINE__, __FILE__, errno, strerror(errno)); exit(1); } while(0)
 
//#define MAP_SIZE 4086UL (this failed on address 0xFFFFFFF1 [but is what code pulled from inet had!])
#define MAP_SIZE 4096UL
#define MAP_MASK (MAP_SIZE - 1)



//===========================================================
//===========================================================
u8 SHFpcie_read_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u64 base_address)
{
	u64 memloc = base_address;
	memloc += bus * 0x100000;
	memloc += device * 0x8000;
	memloc += function * 1000;
	memloc += reg;

	return SHFmem_read_byte(memloc);
}

//===========================================================
//===========================================================
u16 SHFpcie_read_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u64 base_address)
{
	u64 memloc = base_address;
	memloc += bus * 0x100000;
	memloc += device * 0x8000;
	memloc += function * 1000;
	memloc += reg;

	return SHFmem_read_word(memloc);
}

//===========================================================
//===========================================================
u32 SHFpcie_read_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u64 base_address)
{
	u64 memloc = base_address;
	memloc += bus * 0x100000;
	memloc += device * 0x8000;
	memloc += function * 1000;
	memloc += reg;

	return SHFmem_read_dword(memloc);
}

//===========================================================
//===========================================================
void SHFpcie_write_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u8 u8_data, u64 base_address)
{
	u64 memloc = base_address;
	memloc += bus * 0x100000;
	memloc += device * 0x8000;
	memloc += function * 1000;
	memloc += reg;

	SHFmem_write_byte(memloc, u8_data);

return;
}


//===========================================================
//===========================================================
void SHFpcie_write_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u16 u16_data, u64 base_address)
{
	u64 memloc = base_address;
	memloc += bus * 0x100000;
	memloc += device * 0x8000;
	memloc += function * 1000;
	memloc += reg;

	SHFmem_write_word(memloc, u16_data);

return;
}


//===========================================================
//===========================================================
void SHFpcie_write_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u32 u32_data, u64 base_address)
{
	u64 memloc = base_address;
	memloc += bus * 0x100000;
	memloc += device * 0x8000;
	memloc += function * 1000;
	memloc += reg;

	SHFmem_write_dword(memloc, u32_data);

return;
}

u8 IOpci_read_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg)
{
	//0xCF8 - Address   0xCFC - Data (32 bit)
	//bit 31 enable
	//30-24  reserved
	//23-16  bus (0-255)
	//15-11  device (0-31)
	//10-8   function (0-7)
	//7-0    register

	unsigned long DWReg = 0;
	u32 command = 0x80000000;
	u32 data = 0;
	u8 offset = reg % 4;
	u8 retval = 0;

	//need to dword align and then pick the byte that you wanted
	DWReg = (reg / 4) * 4;

	command += bus * 10000;
	command += device * 1000;
	command += function * 100;
	command += DWReg;

	SHF_IO_write_dword(0xCF8, command);

	data = SHF_IO_read_dword(0xCFC);

	retval = (data >> (offset * 8)) & 0xFF;
	return retval;
}

u16 IOpci_read_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg)
{
	//0xCF8 - Address   0xCFC - Data (32 bit)
	//bit 31 enable
	//30-24  reserved
	//23-16  bus (0-255)
	//15-11  device (0-31)
	//10-8   function (0-7)
	//7-0    register

	unsigned long DWReg = 0;
	u32 command = 0x80000000;
	u32 data = 0;
	u8 offset = reg % 4;
	u16 retval = 0;

	//need to dword align and then pick the byte that you wanted
	DWReg = (reg / 4) * 4;

	command += bus * 10000;
	command += device * 1000;
	command += function * 100;
	command += DWReg;

	SHF_IO_write_dword(0xCF8, command);

	data = SHF_IO_read_dword(0xCFC);

	retval = (data >> (offset * 8)) & 0xFFFF;

	//Ok do we allow non-dword aligned PCI IO config cycles?
	//if we do it requires another read and stitching the data together
	if(offset > 2) //if 3 then need to grab bottom byte from next dword
	{
		//move to the next dword, still dword aligned
		command += 4;
		SHF_IO_write_dword(0xCF8, command);

		data = SHF_IO_read_dword(0xCFC);
		retval = retval | ((data & 0xFF) << 8);
	}

	return retval;
}

u32 IOpci_read_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg)
{
	//0xCF8 - Address   0xCFC - Data (32 bit)
	//bit 31 enable
	//30-24  reserved
	//23-16  bus (0-255)
	//15-11  device (0-31)
	//10-8   function (0-7)
	//7-0    register

	unsigned long DWReg = 0;
	u32 command = 0x80000000;
	u32 data = 0;
	u8 offset = reg % 4;
	u32 retval = 0;

	//need to dword align and then pick the byte that you wanted
	DWReg = (reg / 4) * 4;

	command += bus * 10000;
	command += device * 1000;
	command += function * 100;
	command += DWReg;

	SHF_IO_write_dword(0xCF8, command);

	data = SHF_IO_read_dword(0xCFC);

	retval = (data >> (offset * 8));

	//Ok do we allow non-dword aligned PCI IO config cycles?
	//if we do it requires another read and stitching the data together
	if(offset > 0) //if 3 then need to grab bottom byte from next dword
	{
		//move to the next dword, still dword aligned
		command += 4;
		SHF_IO_write_dword(0xCF8, command);

		data = SHF_IO_read_dword(0xCFC);
		retval = retval | (data << (8 * (4-offset)));
	}

	return retval;

}

//===========================================================
//===========================================================
u8 SHFpci_read_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg)
{
   struct pci_access *pacc = pci_alloc();
   struct pci_dev *dev;
   unsigned long am_range = 0;

   // Get rid of warnings.  I hate warnings
   //bus=bus;    device = device;    function = function;    reg = reg;

   pci_init(pacc);
   dev = pci_get_dev(pacc, 0x00, bus, device, function); if (dev == NULL) {printf("Error in pci_get_dev call\n");}

   am_range = (int)pci_read_byte(dev, reg);

   pci_cleanup(pacc);
   return am_range;
}

//===========================================================
//===========================================================
u32 SHFpci_read_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg)
{
   struct pci_access *pacc = pci_alloc();
   struct pci_dev *dev;
   unsigned long am_range;

   // Get rid of warnings.  I hate warnings
   //bus=bus;    device = device;    function = function;    reg = reg;

   pci_init(pacc);
   dev = pci_get_dev(pacc, 0x00, bus, device, function); if (dev == NULL) {printf("Error in pci_get_dev call\n");}

   // If user inputs a non-dword aligned value, align it!
   reg = (reg / 4) * 4;

   am_range = pci_read_long(dev, reg);

   pci_cleanup(pacc);
   return am_range;
}


//===========================================================
//===========================================================
u16 SHFpci_read_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg)
{
   struct pci_access *pacc = pci_alloc();
   struct pci_dev *dev;
   u16 am_range;

   // Get rid of warnings.  I hate warnings
   //bus=bus;    device = device;    function = function;    reg = reg;

   pci_init(pacc);
   dev = pci_get_dev(pacc, 0x00, bus, device, function); if (dev == NULL) {printf("Error in pci_get_dev call\n");}

   // If user inputs a non-dword aligned value, align it!
   reg = (reg / 2) * 2;

   am_range = pci_read_word(dev, reg);

   pci_cleanup(pacc);
   return am_range;
}


//===========================================================
//===========================================================
void SHFpci_write_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u8 u8_data)
{
	struct pci_access *pacc = pci_alloc();
	struct pci_dev *dev;

	pci_init(pacc);
	// dev = pci_get_dev(pacc, domain=0, bus,  device, function);
	dev =    pci_get_dev(pacc, 0x00,     bus,  device, function);
	if (dev == NULL) {
		printf("Failure in pci_get_dev routine; SHFpci_write_byte");
	}

	pci_write_byte(dev, reg, u8_data);
	pci_cleanup(pacc);

return;
}

void IOpci_write_byte(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u8 u8_data)
{
	//0xCF8 - Address   0xCFC - Data (32 bit)
	//bit 31 enable
	//30-24  reserved
	//23-16  bus (0-255)
	//15-11  device (0-31)
	//10-8   function (0-7)
	//7-0    register

	unsigned long DWReg = 0;
	u32 command = 0x80000000;
	u32 data = 0;
	u8 offset = reg % 4;
	u8 retval = 0;

	//need to dword align and then pick the byte that you wanted
	DWReg = (reg / 4) * 4;

	command += bus * 10000;
	command += device * 1000;
	command += function * 100;
	command += DWReg;

	SHF_IO_write_dword(0xCF8, command);

	data = SHF_IO_read_dword(0xCFC);

	retval = (data >> (offset * 8)) & 0xFF;
}



//===========================================================
//===========================================================
void SHFpci_write_word(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u16 u16_data)
{
	struct pci_access *pacc = pci_alloc();
	struct pci_dev *dev;

	pci_init(pacc);
	// dev = pci_get_dev(pacc, domain=0, bus,  device, function);
	dev =    pci_get_dev(pacc, 0x00,     bus,  device, function);
	if (dev == NULL) {
		printf("Failure in pci_get_dev routine; SHFpci_write_byte");
	}

	pci_write_word(dev, reg, u16_data);
	pci_cleanup(pacc);

return;
}


//===========================================================
//===========================================================
void SHFpci_write_dword(unsigned long bus, unsigned long device, unsigned long function, unsigned long reg, u32 u32_data)
{
	struct pci_access *pacc = pci_alloc();
	struct pci_dev *dev;

	pci_init(pacc);
	// dev = pci_get_dev(pacc, domain=0, bus,  device, function);
	dev =    pci_get_dev(pacc, 0x00,     bus,  device, function);
	if (dev == NULL) {
		printf("Failure in pci_get_dev routine; SHFpci_write_byte");
	}

	pci_write_long(dev, reg, u32_data);
	pci_cleanup(pacc);

return;
}


// *** BUG ***
// it looks like the "virt_addr = map_base + (target & MAP_MASK)" portion of code
// must be 16 byte aligned.  Getting crashes with anything that doesn't end in 0.
//===========================================================
//===========================================================
u8 SHFmem_read_byte(u64 passed_address)
{
	u8 u8_data;
	u64 aligned_address, offset;

	int fd; 
  	void *map_base, *virt_addr; 
	unsigned long read_result; 
	off_t target;

	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	
	target = (off_t)(aligned_address);

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);

//	read_result = *((unsigned char *) (virt_addr + offset));
	read_result = *((u8 *) (virt_addr + offset));
	u8_data = read_result;

	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);

return u8_data;
}


//===========================================================
//===========================================================
u16 SHFmem_read_word(u64 passed_address)
{
	u16 u16_data;
	u64 aligned_address, offset;

	int fd; 
  	void *map_base, *virt_addr; 
	unsigned long read_result; 
	off_t target;
	
	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	
	target = (off_t)(aligned_address);

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);
//	read_result = *((unsigned short *) (virt_addr + offset));
	read_result = *((u16 *) (virt_addr + offset));
	u16_data = read_result;

	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);

	return u16_data;
}


//===========================================================
//===========================================================
u32 SHFmem_read_dword(u64 passed_address)
{
	u32 u32_data;
	u16 bla;
	u64 aligned_address, offset;

	int fd; 
  	void *map_base, *virt_addr; 
	unsigned long read_result; 
	off_t target;
	
	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	
	target = (off_t)(aligned_address);

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);
//	read_result = *((unsigned long *) (virt_addr + offset));
	read_result = *((u32 *) (virt_addr + offset));
	u32_data = read_result;

	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);

	return u32_data;
}


//===========================================================
//===========================================================
u64 SHFmem_read_qword(u64 passed_address)
{
	u64 u64_data;
	u64 aligned_address, offset;

	int fd; 
  	void *map_base, *virt_addr; 
	unsigned long read_result; 
	off_t target;
	
	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	
	target = (off_t)(aligned_address);

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);
//	read_result = *((unsigned long *) (virt_addr + offset));
	read_result = *((u64 *) (virt_addr + offset));
	u64_data = read_result;
	u64_data = *((u64 *) (virt_addr + offset));


	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);

	return u64_data;
}


//===========================================================
//===========================================================
void SHFmem_write_byte  (u64 passed_address, u8 u8_data)
{
	int fd; 
  	void *map_base, *virt_addr; 
	off_t target;
	u64 aligned_address, offset;

	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	target = (off_t)(aligned_address);

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("SHFmem_write_byte:  Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);
	*((u8*)(virt_addr + offset)) = u8_data;

	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);

	return;
}


//===========================================================
//===========================================================
void SHFmem_write_word  (u64 passed_address, u16 u16_data)
{
	int fd; 
  	void *map_base, *virt_addr; 
	off_t target;
	u64 aligned_address, offset;
	
	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	target = (off_t)(aligned_address);
		
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("SHFmem_write_word:  Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);
	*((u16 *) (virt_addr+offset)) = u16_data;

	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);
	return;
}


//===========================================================
//===========================================================
void SHFmem_write_dword (u64 passed_address, u32 u32_data)
{
	int fd; 
  	void *map_base, *virt_addr; 
	off_t target;
	u64 aligned_address, offset;
	
	aligned_address = passed_address & 0xFFFFFFF0;
	offset = passed_address - aligned_address;	// 0x00 through 0x0F
	target = (off_t)(aligned_address);
		
	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	// Map one page
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
      printf("SHFmem_write_word:  Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);
	*((u32 *) (virt_addr+offset)) = u32_data;

	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);
	return;
}


// I/O Read Routines
//===========================================================
//===========================================================
u8 SHF_IO_read_byte(u64 passed_address)
	{
	u8 u8_data;
	ioperm(passed_address, 0x01, 0x01);
	u8_data = inb(passed_address);
	ioperm(passed_address, 0x01, 0x00);
	return u8_data;
	}

u16 SHF_IO_read_word(u64 passed_address)
	{
	u16 u16_data;
	ioperm(passed_address, 0x02, 0x01);
	u16_data = inw(passed_address);
	ioperm(passed_address, 0x02, 0x00);
	return u16_data;
	}

u32 SHF_IO_read_dword(u64 passed_address)
	{
	u32 u32_data;
	ioperm(passed_address, 0x04, 0x01);
	u32_data = inl(passed_address);
	ioperm(passed_address, 0x04, 0x00);
	return u32_data;
	}


// I/O Write Routines
//===========================================================
//===========================================================
void SHF_IO_write_byte(u64 passed_address, u8 u8_data)
	{
	ioperm(passed_address, 0x01, 0x01);
	outb(u8_data, passed_address);
	ioperm(passed_address, 0x01, 0x00);
	return;
	}

void SHF_IO_write_word(u64 passed_address, u16 u16_data)
	{
	ioperm(passed_address, 0x02, 0x01);
	outw(u16_data, passed_address);
	ioperm(passed_address, 0x02, 0x00);
	return;
	}

void SHF_IO_write_dword(u64 passed_address, u32 u32_data)
	{
	ioperm(passed_address, 0x04, 0x01);
	outl(u32_data, passed_address);
	ioperm(passed_address, 0x04, 0x00);
	return;
	}



// MSR Read Routine
//===========================================================
//===========================================================
// Posted by Alexander Weggerle (Intel) Wed, 02/08/2012 - 01:57 
// http://software.intel.com/en-us/forums/topic/280098
// NOTE:  YOU MUST RUN sudo modprobe msr before running these commands
// NOTE:  You MIGHT have to run sudo -s before running these commands

int SHF_rdmsr (int CPU_number, unsigned int MsrNum, unsigned long long *MsrVal) {
        char msrname[100];
        unsigned char MsrBuffer[8];
        int fh;
        off_t offset, fpos;

        /* Ok, use the /dev/CPU interface */
        #ifdef __ANDROID__
                snprintf (msrname,sizeof(msrname), "/dev/msr%d", CPU_number);
        #else
                snprintf (msrname,sizeof(msrname), "/dev/cpu/%d/msr", CPU_number);
        #endif

        fh = open (msrname, O_RDONLY);
        if (fh != -1) {
                offset = (off_t)MsrNum;
                fpos = lseek (fh, offset, SEEK_SET);
                if(fpos != offset) {
                        printf("seek %s to offset= 0x%x failed at %s %d\n",
                        msrname, MsrNum, __FILE__, __LINE__);
                        return -1;
                }
                read (fh, MsrBuffer, sizeof(MsrBuffer));
                if (MsrVal!=0) *MsrVal = (*(unsigned long long *)MsrBuffer);
                close (fh);
                return 0;
        } else {
                /* Something went wrong, just get out. */
                printf("Open of msr dev= '%s' failed at %s %d\n", msrname, __FILE__, __LINE__);
                printf("You may need to do (as root) 'sudo modprobe msr'\n");
                return -1;
        }
}


// MSR Write Commadn (DOES NOT APPEAR TO WORK CORRECTLY)
//===========================================================
//===========================================================
// This write MSR command does not seem to be working
// Looks very much like protection issue of some type (writes to MSR's not allowed)
// These write to MSR's are NOT working.  Instead, I'm calling:  		system(tempstr);	 where tempstr is the wrmsr 0xc3 0x11
int SHF_wrmsr (int CPU_number, unsigned int MsrNum, unsigned long long *MsrVal) {
        char msrname[100];
        unsigned char MsrBuffer[8];
        int fh;
        off_t offset, fpos;

        /* Ok, use the /dev/CPU interface */
        #ifdef __ANDROID__
                snprintf (msrname,sizeof(msrname), "/dev/msr%d", CPU_number);
        #else
                snprintf (msrname,sizeof(msrname), "/dev/cpu/%d/msr", CPU_number);
        #endif

        fh = open (msrname, O_RDWR | O_SYNC);
		fflush(stdout);

        if (fh != -1) {
                offset = (off_t)MsrNum;
                fpos = lseek (fh, offset, SEEK_SET);
                if(fpos != offset) {
                        printf("seek %s to offset= 0x%x failed at %s %d\n",
                        msrname, MsrNum, __FILE__, __LINE__);
                        return -1;
                }
                read (fh, MsrBuffer, sizeof(MsrBuffer));
                if (MsrVal!=0) 
					{
//					printf("*MsrVal: %llx\n", *MsrVal);  
					(*(unsigned long long *)MsrBuffer) = *MsrVal;

//					printf("\nLet's Read it Back:  "); 
//					printf("%llx\n", (*(unsigned long long *)MsrBuffer));
					// Sam, this proves that the WRITES are "working", just not permanent.
					}

				fflush(stdout);
                close (fh);
                return 0;
        } else {
                /* Something went wrong, just get out. */
                printf("Open of msr dev= '%s' failed at %s %d\n", msrname, __FILE__, __LINE__);
                printf("You may need to do (as root) 'sudo modprobe msr'\n");
                return -1;
        }
}

//===========================================================
//===========================================================
void SHFprint(u64 number, int min_length, int base, char *before, char *after)
// number - the input passed number
// min_length = how many chars minimum you want to display.
//		For example, number = 0x323.  min_length = 4.  Output = 0x0323
//		base.  You want hexidecimal (16)?  Base 10 (10)?  Octal (8)?
{
	u64 divisor;
	u64 digit;
	int i;

// I hate warnings
	//number = number;
	//min_length = min_length;
	//base = base;

	printf("%s", before);

	for (i=(min_length-1); i>=0 ;i--)
		{
		divisor = pow(base,i);
		digit = (number/divisor);
		number = number-(digit*divisor);
		printf("%X", (unsigned int)digit);
		}

	printf("%s", after);


	return;
}


//===========================================================
//===========================================================
u64 rdtsc(void)
{
	static u64 last;
	u32 lo, hi;
	u64 tsc;

	//last = last;

	if (sizeof(long) == sizeof(u64)) 
		{
	   	asm volatile("rdtsc" : "=a" (lo), "=d" (hi));
		return ((u64)(hi) << 32) | lo;
		}
	else 
		{
	   	asm volatile("rdtsc" : "=A" (tsc));
		return tsc;
		}
}


//===========================================================
//===========================================================
double Freq_Calc()
// From Todd Silver ITP Script
// Tested and works.  Note.  Does not handle HPET roll-over cause I'm in a hurry.  Deal with it.
{
	u64 start_time, end_time;
	unsigned long HPET_start_time, HPET_end_time, temp; 

	static double frequency = 0;
	// If run a second time during program, just exit with the last read frequency!

	if (frequency == 0)
		{
		HPET_start_time = Read_HPET();
		HPET_end_time = HPET_start_time + 0x44463F3; // 5 seconds
		start_time = rdtsc();
		temp=0x0;
		while (temp < HPET_end_time)     // Todd's way of waiting 5 seconds
			{
			temp = Read_HPET();
			}   
		end_time = rdtsc();
		frequency = (end_time - start_time)/5;
		}
	return frequency;
}


//===========================================================
//===========================================================
u64 Read_HPET()
// Note:  This is a 64 bit register, but I can't get my 64 bit Memory Read 
// Routine to work correctly.  Attempted fix onRoll-over seems to work
{
	u32 RCBA_Base,  HPET_Base;
	u8 u8ReturnData = 0x00;

	u64 HPET_Timer_Low = 0x00;
	static u32 LAST_HPET_Timer_Low = 0x00;
	static u64 HPET_Timer_Hi = 0x00;

	u64 HPET_Timer_Final;

	RCBA_Base = SHFpci_read_dword(0x00, 0x1F, 0x00, 0xF0);
	u8ReturnData = SHFmem_read_byte(RCBA_Base + 0x3404);

	// On some boards I needed to enable timer:  [7] = 1b.
	SHFmem_write_byte(((RCBA_Base + 0x3404) & (0xFFFFFFFC)), (u8ReturnData | 0x80));

/*
	// TEST CODE:
	u64 addressx;
	u64 datax;
	addressx = (u64)((RCBA_Base + 0x3404) & (0xFFFFFFFC));
	datax = (u64)(u8ReturnData | 0x80);
	printf("RCBA_Base:\t0x%X\n", RCBA_Base);
	printf("Address:\t0x%lX\n", addressx);
	printf("Data:\t0x%lX\n", datax);
*/	

	u8ReturnData = u8ReturnData & 0x03;
	switch (u8ReturnData)
		{
		case 0x00:
			HPET_Base = 0xFED00000;			
			break;
		case 0x01:
			HPET_Base = 0xFED01000;			
			break;
		case 0x02:
			HPET_Base = 0xFED02000;			
			break;
		case 0x03:
			HPET_Base = 0xFED03000;			
			break;
		default:
			HPET_Base = 0xFED00000;			
		}
	HPET_Timer_Low = SHFmem_read_dword (HPET_Base + 0xF0);

	// Trying to fix the overflow problem by being clever - Note that 
	// Generally HPET is going to be read to MEASURE stuff.  Can fix
	// by noting overflow.
	if (HPET_Timer_Low < LAST_HPET_Timer_Low)	// Overflow
		{
		HPET_Timer_Hi = HPET_Timer_Hi + 1;
		}
	LAST_HPET_Timer_Low = HPET_Timer_Low;

	HPET_Timer_Final = (HPET_Timer_Hi << 32) + HPET_Timer_Low;

	return HPET_Timer_Final;
}


//===========================================================
//===========================================================
double tsc_delay(u64 start_time, u64 end_time, char *units, double input_freq)
{
	u64 difference = end_time-start_time;
	double frequency;

	if (input_freq == 0)
		{
		frequency = Freq_Calc(); // This will be 3.2 for 3.2 GHz
		}
	else
		frequency = input_freq;
//	SHFprint(difference, 16, 10,"0x","  ");
//	printf("Freq:  %f", frequency);
//	printf("\n");


		if (strncmp(units, "clocks", 6) == 0)
			return difference;
		if (strncmp(units, "cycles", 6) == 0)
			return difference;

		if (strncmp(units, "sec", 3) == 0)
			return difference/frequency;

		if (strncmp(units, "ms", 2) == 0)
			return (difference*1000)/frequency;

		if (strncmp(units, "us", 4) == 0)
			return (difference*1000000)/frequency;

		if (strncmp(units, "ns", 2) == 0)
			return (difference*1000000000)/frequency;

	return -1;
}


//===========================================================
//===========================================================
double assembly_delay(u64 passed_address, char *units, u32 *read_result, double input_freq)
{
	u64 start_time = 0, end_time = 0;
	u64 difference;

	unsigned tsc_high, tsc_low;

	int fd; 
	void *map_base, *virt_addr; 
	unsigned long writeval = 0;
	off_t target;
	int access_type = 'w';
	static u32 read_data=0;

	static double frequency = 0;
	// If run a second time during program, just exit with the last read frequency!

	// I hate warnings:
	//passed_address = passed_address;
	//start_time = start_time;
	//end_time = end_time;
	//access_type = access_type;
	//writeval = writeval;
	//read_result = read_result;
	//read_data = read_data;

	// -------------------------------
	if (input_freq == 0)
		{
		frequency = Freq_Calc(); // This will be 3.2 for 3.2 GHz
		}
	else
		frequency = input_freq;

	// -------------------------------
	target = passed_address;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    
	/* Map one page */
	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	if(map_base == (void *) -1) 
		{
		printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}

	virt_addr = map_base + (target & MAP_MASK);

//  ---------------------------------------------------------
// The Ugliness
	//  ---------------------------------------------------------
	//	Read the TSC for START TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------
	start_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );

	read_data = *((unsigned long *) virt_addr);	// DATA READ HERE

	//  ---------------------------------------------------------
	//	Read the TSC for end TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	end_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );
//  ---------------------------------------------------------

	close(fd);

	// ------------------------------
	*read_result = read_data;
	difference = end_time-start_time;

	if (strncmp(units, "clocks", 6) == 0)
		return difference;
	if (strncmp(units, "cycles", 6) == 0)
		return difference;

	if (strncmp(units, "sec", 3) == 0)
		return difference/frequency;

	if (strncmp(units, "ms", 2) == 0)
		return (difference*1000)/frequency;

	if (strncmp(units, "us", 4) == 0)
		return (difference*1000000)/frequency;

	if (strncmp(units, "ns", 2) == 0)
		return (difference*1000000000)/frequency;

	return -1;
}


//===========================================================
//===========================================================
void neg_add_one(u32 *i)
{
	// Ignore this code.  I needed it to test some theories on Ref Passing.
	// I'll need it again some day....
	//
	// Pro-Tip.  Passing "&variable" doesn't work in GCC!!!
	u32 thedata;

	*i= -*i;
	thedata = *i+1;
	*i=thedata;
	// warnings...*sigh*
	//thedata = thedata;
}


//===========================================================
//===========================================================
double read_assembly_delay(u64 passed_address, char *units, double input_freq, u64 byte_length, u8 array1[], u8 size)
{
//	u64 start_time = 0, end_time = 0;
	unsigned tsc_high, tsc_low;
	unsigned long long int start_time = 0, end_time = 0;
	u64 difference;
	int fd; 
	void *map_base, *virt_addr; 
	unsigned long writeval = 0;
	off_t target;
	int access_type = 'w';
	static u32 read_data=0;
	static double frequency = 0;
	// If run a second time during program, just exit with the last read frequency!

	// I hate warnings:
	//passed_address = passed_address;
	//start_time = start_time;
	//end_time = end_time;
	//access_type = access_type;
	//writeval = writeval;
	//read_data = read_data;

/*
	// -------------------------------
	if (input_freq == 0.0)		
		{
		frequency = Freq_Calc(); // This will be 3.2 for 3.2 GHz
		}
	else
		frequency = input_freq;
	// -------------------------------
*/
	frequency = input_freq;
	target = passed_address;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);

   unsigned long map_size;
   unsigned long map_mask;
// #define MAP_SIZE 4096UL
// #define MAP_MASK (MAP_SIZE - 1) 

	map_size = ((byte_length + 0xFFF) & 0xFFFFF000);     // 4K min size and aligning
	map_mask = map_size -1;

//	SHFprint(map_size, 8, 0x10, "map_size ", "\n");
//	SHFprint(map_mask, 8, 0x10, "map_size ", "\n");
//  	exit(0);

/*  This did not work
	map_size = ((byte_length + 0xF) & 0xFFFFFFF0);     // 16 byte aligning
   map_mask = map_size-1;
*/
    
	// -------------------------------
	/* Map one page */
//	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~map_mask);
	if(map_base == (void *) -1) 
		{
		printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}
	virt_addr = map_base + (target & MAP_MASK);
	// -------------------------------

	u64 count = byte_length/size;			// size =1(byte) / 2(word) / 4(dword) =
	if (byte_length%size)
		count = count +1;	

	//  ---------------------------------------------------------
	//	Read the TSC for START TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	start_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );


	//  ---------------------------------------------------------
	//	DATA READ GOES HERE 
	// Each "Block" of this is 4K (0-0x1000)

	if (size == 1)  // byte
		{
		   asm ("cld;"
				  "rep movsb;"                         /* copy esi to edi */
			 	  "MFENCE;"
		   :                                         /* output (none) */
		   :"S"(virt_addr), "c"(count), "D"(array1)  /* input:   %0=virt_addr (%esi), %1=count (%ecx), %2=array1 (%edi) */
		   :"cc"		                                 /* clobbered register: %rax, %rbx, %rdx", carry-flag */
		   ); 
		}
	else if (size == 2) // word
		{
		   asm ("cld;"
				  "rep movsw;"                         /* copy esi to edi */
			 	  "MFENCE;"
		   :                                         /* output (none) */
		   :"S"(virt_addr), "c"(count), "D"(array1)  /* input:   %0=virt_addr (%esi), %1=count (%ecx), %2=array1 (%edi) */
		   :"cc"		                                 /* clobbered register: %rax, %rbx, %rdx", carry-flag */
		   ); 
		}
	else if (size == 4) // dword
		{
		   asm ("cld;"
				  "rep movsd;"                         /* copy esi to edi */
			 	  "MFENCE;"
		   :                                         /* output (none) */
		   :"S"(virt_addr), "c"(count), "D"(array1)  /* input:   %0=virt_addr (%esi), %1=count (%ecx), %2=array1 (%edi) */
		   :"cc"		                                 /* clobbered register: %rax, %rbx, %rdx", carry-flag */
		   ); 
		}

	//  ---------------------------------------------------------
	//	Read the TSC for end TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	end_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );

	//  ---------------------------------------------------------
	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);
	//  ---------------------------------------------------------

	//  ---------------------------------------------------------
	difference = end_time-start_time;

//		Debug
//   printf("start_time:\t\t%llx\n",start_time);
//   printf("end_time:\t\t%llx\n",end_time);

	if (strncmp(units, "clocks", 6) == 0)
		return difference;
	if (strncmp(units, "cycles", 6) == 0)
		return difference;
	if (strncmp(units, "sec", 3) == 0)
		return difference/frequency;
	if (strncmp(units, "ms", 2) == 0)
		return (difference*1000)/frequency;
	if (strncmp(units, "us", 4) == 0)
		return (difference*1000000)/frequency;
	if (strncmp(units, "ns", 2) == 0)
		return (difference*1000000000)/frequency;
	return -1;
}


//===========================================================
//===========================================================
double write_assembly_delay(u64 passed_address, char *units, double input_freq, u64 byte_length, u8 array1[], u8 size)
{
	unsigned tsc_high, tsc_low;
	u64 start_time = 0, end_time = 0;
	u64 difference;
	int fd; 
	void *map_base, *virt_addr; 
	unsigned long writeval = 0;
	off_t target;
	int access_type = 'w';
	static u32 read_data=0;
	static double frequency = 0;
	// If run a second time during program, just exit with the last read frequency!

	// I hate warnings:
	//passed_address = passed_address;
	//start_time = start_time;
	//end_time = end_time;
	//access_type = access_type;
	//writeval = writeval;
	//read_data = read_data;

/*
	// -------------------------------
	if (input_freq == 0.0)		
		{
		frequency = Freq_Calc(); // This will be 3.2 for 3.2 GHz
		}
	else
		frequency = input_freq;
	// -------------------------------
*/
	frequency = input_freq;
	target = passed_address;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);

   unsigned long map_size;
   unsigned long map_mask;
	map_size = ((byte_length + 0xFFF) & 0xFFFFF000);     // 4K min size and aligning
	map_mask = map_size -1;
    
	// -------------------------------
	/* Map one page */
//	map_base = mmap(0, MAP_SIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~MAP_MASK);
	map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~map_mask);
	if(map_base == (void *) -1) 
		{
		printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}
	virt_addr = map_base + (target & MAP_MASK);
	// -------------------------------


	u64 count = byte_length/size;			// size =1(byte) / 2(word) / 4(dword) =
	if (byte_length%size)
		count = count +1;	

	//  ---------------------------------------------------------
	//	Read the TSC for START TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	start_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );



	//  ---------------------------------------------------------
	//	DATA READ GOES HERE 
	// Each "Block" of this is 4K (0-0x1000)

	if (size == 1)  // byte
		{
		   asm ("cld;"
				  "rep movsb;"                         /* copy esi to edi */
			 	  "MFENCE;"
		   :                                         /* output (none) */
		   :"S"(array1), "c"(count), "D"(virt_addr)  /* input:   %0=array1 (%esi), %1=count (%ecx), %2=virt_addr (%edi) */
		   :"cc"		                                 /* clobbered register: %rax, %rbx, %rdx", carry-flag */
		   ); 
		}
	else if (size == 2) // word
		{
		   asm ("cld;"
				  "rep movsw;"                         /* copy esi to edi */
			 	  "MFENCE;"
		   :                                         /* output (none) */
		   :"S"(array1), "c"(count), "D"(virt_addr)  /* input:   %0=array1 (%esi), %1=count (%ecx), %2=virt_addr (%edi) */
		   :"cc"		                                 /* clobbered register: %rax, %rbx, %rdx", carry-flag */
		   ); 
		}
	else if (size == 4) // dword
		{
		   asm ("cld;"
				  "rep movsd;"                         /* copy esi to edi */
			 	  "MFENCE;"
		   :                                         /* output (none) */
		   :"S"(array1), "c"(count), "D"(virt_addr)  /* input:   %0=array1 (%esi), %1=count (%ecx), %2=virt_addr (%edi) */
		   :"cc"		                                 /* clobbered register: %rax, %rbx, %rdx", carry-flag */
		   ); 
		}

	//  ---------------------------------------------------------
	//	Read the TSC for end TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	end_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );

	//  ---------------------------------------------------------
	fflush(stdout);
	if(munmap(map_base, MAP_SIZE) == -1) FATAL;
	close(fd);
	//  ---------------------------------------------------------

	//  ---------------------------------------------------------
	difference = end_time-start_time;

	if (strncmp(units, "clocks", 6) == 0)
		return difference;
	if (strncmp(units, "cycles", 6) == 0)
		return difference;
	if (strncmp(units, "sec", 3) == 0)
		return difference/frequency;
	if (strncmp(units, "ms", 2) == 0)
		return (difference*1000)/frequency;
	if (strncmp(units, "us", 4) == 0)
		return (difference*1000000)/frequency;
	if (strncmp(units, "ns", 2) == 0)
		return (difference*1000000000)/frequency;
	return -1;
}

//===========================================================
//===========================================================
double block_read_assembly_delay_new(u64 passed_address, char *units, double input_freq, u64 number_4K_blocks, u8 array1[])
{
//	unsigned long array1[1024] __attribute__ ((aligned(64)));
	unsigned tsc_high, tsc_low;	
	u64 start_time = 0, end_time = 0;
	u64 difference;
	int fd; 
	void *map_base, *virt_addr; 
	//unsigned long writeval = 0;
	off_t target;
	int access_type = 'w';
	//static u32 read_data=0;
	static double frequency = 0;
	// If run a second time during program, just exit with the last read frequency!

	// I hate warnings:
	//passed_address = passed_address;
	//start_time = start_time;
	//end_time = end_time;
	//access_type = access_type;
	//writeval = writeval;
	//read_data = read_data;

/*
	// -------------------------------
	if (input_freq == 0.0)		
		{
		frequency = Freq_Calc(); // This will be 3.2 for 3.2 GHz
		}
	else
		frequency = input_freq;
	// -------------------------------
*/
	frequency = input_freq;
	target = passed_address;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);

// #define MAP_SIZE 4096UL
// #define MAP_MASK (MAP_SIZE - 1)
unsigned long map_size;
unsigned long map_mask;
map_size = number_4K_blocks * 0x1000;
map_mask = map_size-1;
    
	// -------------------------------
	/* Map one page */
	map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~map_mask);
	if(map_base == (void *) -1) 
		{
		printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}
	virt_addr = map_base + (target & map_mask);
	// -------------------------------


	u64 count = number_4K_blocks;

//		float TSC_delta;
//  	void *map_base, *virt_addr, *HPET_base, *HPET_virt_addr; 
//		unsigned long  HPET_start_time, HPET_end_time, HPET_delta; 
//		float HPET_resolution = 69.841279;  // (in ns)
//		float HPET_bandwidth, TSC_bandwidth;

/*
	//  ---------------------------------------------------------
	// Lets read first data to return (for testing)
	// This will show up on Logic Analyzer
	read_data = *((unsigned long *) virt_addr);	// DATA READ HERE
	array1[0] = read_data;
	//  ---------------------------------------------------------
*/

	//  ---------------------------------------------------------
	//	Read the TSC for START TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	start_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );

	//  ---------------------------------------------------------
	//	DATA READ GOES HERE 
	// Each "Block" of this is 4K (0-0x1000)

      asm ("movq %0, %%rax; "
	   "movq %1, %%rcx; "
		"loop3: ;"
//		"MOVNTDQA 0x0000(%%rax), %%xmm0;"		// %%rax = %0 = virt_addr (memory)
//		"MOVNTDQA 0x0010(%%rax), %%xmm1;"		// READ FROM MEM => XMM
//		"MOVNTDQA 0x0020(%%rax), %%xmm2;"
//		"MOVNTDQA 0x0030(%%rax), %%xmm3;"
//		"MOVDQA %%xmm0, 0x0000(%%rsi);"			// %%rsi = %2 = array1 (data)
//		"MOVDQA %%xmm1, 0x0010(%%rsi);"			// WRITE XMM => DATA
//		"MOVDQA %%xmm2, 0x0020(%%rsi);"
//		"MOVDQA %%xmm3, 0x0030(%%rsi);"
#include "code_block_read.h"
//		"MOVNTDQA 0x0fc0(%%rax), %%xmm0;"
//		"MOVNTDQA 0x0fd0(%%rax), %%xmm1;"
//		"MOVNTDQA 0x0fe0(%%rax), %%xmm2;"
//		"MOVNTDQA 0x0ff0(%%rax), %%xmm3;"
//		"MOVDQA %%xmm0, 0x0fc0(%%rsi);"
//		"MOVDQA %%xmm1, 0x0fd0(%%rsi);"
//		"MOVDQA %%xmm2, 0x0fe0(%%rsi);"
//		"MOVDQA %%xmm3, 0x0ff0(%%rsi);"
//		"LFENCE;"
		"MFENCE;"
		"ADD $0x1000, %%rax;"					// Remove the $, code crashes
		"ADD $0x1000, %%rsi;"					// Remove the $, code crashes
		"DEC %%ecx;"
		"JNE loop3;"

      :                                         /* output (none) */
      :"r"(virt_addr), "c"(count), "S"(array1)  /* input:   %0=virt_addr (%rax above), %1=count (%ecx %rcx above), %2=array1 (%esi) */
      :"%rax", "cc"										/* clobbered register: %rax, carry-flag */
      ); 

	//  ---------------------------------------------------------
	//	Read the TSC for END TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	end_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );


	//  ---------------------------------------------------------
	fflush(stdout);
	if(munmap(map_base, map_size) == -1) FATAL;
	close(fd);
	//  ---------------------------------------------------------

	//  ---------------------------------------------------------
	difference = end_time-start_time;

	if (strncmp(units, "clocks", 6) == 0)
		return difference;
	if (strncmp(units, "cycles", 6) == 0)
		return difference;
	if (strncmp(units, "sec", 3) == 0)
		return difference/frequency;
	if (strncmp(units, "ms", 2) == 0)
		return (difference*1000)/frequency;
	if (strncmp(units, "us", 4) == 0)
		return (difference*1000000)/frequency;
	if (strncmp(units, "ns", 2) == 0)
		return (difference*1000000000)/frequency;


	return -1;


	/*
   printf("HPET Delta\t\t%f usec\n",((HPET_delta * HPET_resolution)/1000));
   HPET_bandwidth = MAP_SIZE*count / ((HPET_delta * HPET_resolution)/1000);
   printf("BandWidth (HPET)\t%f MB/sec\n",HPET_bandwidth);

   TSC_bandwidth = (MAP_SIZE*count / TSC_delta);
   printf("BandWidth (TSC)\t\t%f MB/sec\n",TSC_bandwidth);
	*/
	//  ---------------------------------------------------------

}


//===========================================================
//===========================================================
double block_write_assembly_delay_new(u64 passed_address, char *units, double input_freq, u64 number_4K_blocks, u8 array1[])
{
//	unsigned long array1[1024] __attribute__ ((aligned(64)));
	unsigned tsc_high, tsc_low;
	u64 start_time = 0, end_time = 0;
	u64 difference;
	int fd; 
	void *map_base, *virt_addr; 
	//unsigned long writeval = 0;
	off_t target;
	//int access_type = 'w';
	//static u32 read_data=0;
	static double frequency = 0;
	// If run a second time during program, just exit with the last read frequency!



	// I hate warnings:
	//passed_address = passed_address;
	//start_time = start_time;
	//end_time = end_time;
	//access_type = access_type;
	//writeval = writeval;
	//read_data = read_data;

/*
	// -------------------------------
	if (input_freq == 0.0)		
		{
		frequency = Freq_Calc(); // This will be 3.2 for 3.2 GHz
		}
	else
		frequency = input_freq;
	// -------------------------------
*/
	frequency = input_freq;
	target = passed_address;

	if((fd = open("/dev/mem", O_RDWR | O_SYNC)) == -1) FATAL;
	fflush(stdout);
    

// #define MAP_SIZE 4096UL
// #define MAP_MASK (MAP_SIZE - 1)
unsigned long map_size;
unsigned long map_mask;
map_size = number_4K_blocks * 0x1000;
map_mask = map_size-1;
    
	// -------------------------------
	/* Map one page */
	map_base = mmap(0, map_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, target & ~map_mask);
	if(map_base == (void *) -1) 
		{
		printf("Address we tried to pass:\t0x%lX\n", target);
		FATAL;
		}
	virt_addr = map_base + (target & map_mask);
	// -------------------------------

	u64 count = number_4K_blocks;

//		float TSC_delta;
//  	void *map_base, *virt_addr, *HPET_base, *HPET_virt_addr; 
//		unsigned long  HPET_start_time, HPET_end_time, HPET_delta; 
//		float HPET_resolution = 69.841279;  // (in ns)
//		float HPET_bandwidth, TSC_bandwidth;

/*
	//  ---------------------------------------------------------
	// Lets read first data to return (for testing)
	// This will show up on Logic Analyzer
	read_data = *((unsigned long *) virt_addr);	// DATA READ HERE
	array1[0] = read_data;
	//  ---------------------------------------------------------
*/

	//  ---------------------------------------------------------
	//	Read the TSC for START TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	start_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );

	//  ---------------------------------------------------------
	//	DATA WRITE GOES HERE 
	// Each "Block" of this is 4K (0-0x1000)

      asm ("movq %0, %%rax; "
	   "movq %1, %%rcx; "
		"loop4: ;"
//		"MOVNTDQA 0x0000(%%rax), %%xmm0;"
//		"MOVNTDQA 0x0010(%%rax), %%xmm1;"
//		"MOVNTDQA 0x0020(%%rax), %%xmm2;"
//		"MOVNTDQA 0x0030(%%rax), %%xmm3;"
//		"MOVDQA %%xmm0, 0x0000(%%rsi);"
//		"MOVDQA %%xmm1, 0x0010(%%rsi);"
//		"MOVDQA %%xmm2, 0x0020(%%rsi);"
//		"MOVDQA %%xmm3, 0x0030(%%rsi);"
#include "code_block_read.h"
//		"MOVNTDQA 0x0fc0(%%rax), %%xmm0;"
//		"MOVNTDQA 0x0fd0(%%rax), %%xmm1;"
//		"MOVNTDQA 0x0fe0(%%rax), %%xmm2;"
//		"MOVNTDQA 0x0ff0(%%rax), %%xmm3;"
//		"MOVDQA %%xmm0, 0x0fc0(%%rsi);"
//		"MOVDQA %%xmm1, 0x0fd0(%%rsi);"
//		"MOVDQA %%xmm2, 0x0fe0(%%rsi);"
//		"MOVDQA %%xmm3, 0x0ff0(%%rsi);"
//		"LFENCE;"
		"MFENCE;"
		"ADD $0x1000, %%rax;"					// Remove the $, code crashes
		"ADD $0x1000, %%rsi;"					// Remove the $, code crashes
		"DEC %%ecx;"
		"JNE loop4;"

      :                                         /* output (none) */
      :"r"(array1), "c"(count), "S"(virt_addr)  /* input:   %0=array1 (%rax above), %1=count (%ecx  %rcx above), %2=virt_addr (%esi) */
      :"%rax", "cc"										/* clobbered register: %rax, %rsi, carry-flag */
      ); 

	//  ---------------------------------------------------------
	//	Read the TSC for end TIME
	asm ("LFENCE;");
   	asm volatile("rdtsc" : "=a" (tsc_low), "=d" (tsc_high) );
	asm ("LFENCE;");        //I get diff. answers if I move LFENCE's in & out
	//  ---------------------------------------------------------

	end_time = ( (unsigned long long)(tsc_low) | ((unsigned long long)(tsc_high) << 32)  );

	//  ---------------------------------------------------------
	fflush(stdout);
	if(munmap(map_base, map_size) == -1) FATAL;
	close(fd);
	//  ---------------------------------------------------------

	//  ---------------------------------------------------------
	difference = end_time-start_time;
	if (strncmp(units, "clocks", 6) == 0)
		return difference;
	if (strncmp(units, "cycles", 6) == 0)
		return difference;
	if (strncmp(units, "sec", 3) == 0)
		return difference/frequency;
	if (strncmp(units, "ms", 2) == 0)
		return (difference*1000)/frequency;
	if (strncmp(units, "us", 4) == 0)
		return (difference*1000000)/frequency;
	if (strncmp(units, "ns", 2) == 0)
		return (difference*1000000000)/frequency;

	return -1;
}

//===========================================================
//===========================================================
// These write to MSR's are NOT working.  Instead, I'm calling:  		system(tempstr);	 where tempstr is the wrmsr 0xc3 0x11
void SHF_wrmsr_new(u64 passed_address, u64 data)
{
	asm  volatile ("wrmsr;" 										// wiki.osdev.org/Inline_Assembly/Examples
         : 
         : "c"(passed_address), "A"(data)
			);	
	return;

/*
	unsigned int edx_bogus = 0;

	asm  volatile ("movq %0, %%rcx;"								// Address
						"movq %1, %%rax;"								// Low order of Data
						"movq $0, %%rdx;"								// High Order of Data
						"wrmsr;" 										// wiki.osdev.org/Inline_Assembly/Examples
         : 
         : "c"(passed_address), "A"(data) , "d"(edx_bogus)
			);	
	return;
*/
}

//===========================================================
//===========================================================
unsigned int PCI_Device_Found_and_Size(unsigned long bus, unsigned long device, unsigned long function)
	{
	// unsigned int register_size = 0;		// 0 = Not Found.  255 = Normal PCI.  4096 (4K) = contains extended PCI regs
	u16 devid = 0x01;
	u16 vendorid = 0x01;
	u8 capability_pointer = 0x01;
	u8 capability_pointer_addr = 0x34;

	devid = SHFpci_read_word(bus, device, function, 0x02);
	vendorid = SHFpci_read_word(bus, device, function, 0x00);

	if  ( ( (devid == 0x0000) && (vendorid == 0x0000)) || 
         ( (devid == 0xFFFF) && (vendorid == 0xFFFF)) )
		return 0x00;

	// Okay, let's go look for PCI Express Capability Pointer.  If we find it, then return 4096.  Else, return 255
	capability_pointer_addr =	SHFpci_read_byte(bus, device, function, 0x34);

	while (capability_pointer_addr != 0x00)
		{
		capability_pointer =	SHFpci_read_byte(bus, device, function, capability_pointer_addr);
		if (capability_pointer == 0x10)
			return 0x1000;
		else
			capability_pointer_addr = SHFpci_read_byte(bus, device, function, capability_pointer_addr+1);
		}
	return 0x100;

}

//===========================================================
//===========================================================
unsigned int PCIE_Device_Found_and_Size(unsigned long bus, unsigned long device, unsigned long function, u64 base_address)
	{
	// unsigned int register_size = 0;		// 0 = Not Found.  255 = Normal PCI.  4096 (4K) = contains extended PCI regs
	u16 devid = 0x01;
	u16 vendorid = 0x01;
	u8 capability_pointer = 0x01;
	u8 capability_pointer_addr = 0x34;

	devid = SHFpcie_read_word(bus, device, function, 0x02, base_address);
	vendorid = SHFpcie_read_word(bus, device, function, 0x00, base_address);

	if  ( ( (devid == 0x0000) && (vendorid == 0x0000)) ||
         ( (devid == 0xFFFF) && (vendorid == 0xFFFF)) )
		return 0x00;

	// If we find it, then return 4096.  Because, well...it's PCIExpress space so you get it in all its glory

	return 0x1000;




}




