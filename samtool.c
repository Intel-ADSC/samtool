//===========================================================
// To Compile:
//	gcc -Wall -W -Werror -g samtool.c samkit.c -lpci -lm -o samtool
//	   with 64 bit version installed via:  sudo apt-get install libpci-dev
//
//	gcc -Wall -m32 -W -Werror -g samtool.c samkit.c -lpci -lm -o samtool
//	   with 32 bit version installed via:  sudo apt-get install libpci-dev:i386
//
// Statically
//	gcc -static -Wall -W -Werror -g samtool.c samkit.c -lpci -lm -lz -o samtool
//	   Ignore the getpwuid error 
// 
// To Run:
//   	sudo modprobe msr			- Only first time after boot.  Password is x.
//   	sudo ./samtool				- Password is x on my machine.
//
// To un-install LAN driver, opening up it's MMIO range for testing (GFX MMIO works fine):
//		sudo rmmod e1000e			- Only first time after boot.  Password is x.
//   	sudo ./samtool				- Password is x on my machine.
//
//	To run in debug mode:
//		sudo gdb ./samtool
//
//	Better Graphical debugger:
//		sudo apt-get install ddd	- run once.
//		sudo apt-get install xterm	- run once.
//		sudo ddd ./samtool
//
//===========================================================
//#define  DEBUG_STUFF
//===========================================================
/*
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
// Version Info
#define COPYRIGHT_STRING "Copyright 2015, Intel Corporation"
#define AUTHOR_STRING "Sam Fleming"
#define VERSION_STRING "Version 1.4  (02-06-2015)"
// ==========================================================
// Defines Being Used
#include <stdbool.h>		// For bool
#include <stdlib.h>		// abs()
#include <math.h>		   // need for pow (exponent) ** Must use -lm compile option **
#include <string.h>		// for strcpy
#include <stdio.h>		// strcpy
#include <ctype.h>      // string stuff
#include <pci/pci.h>    // ** Must use -lpci compile option **
#include <sys/io.h>		// Permits access to IO Locations
#include <unistd.h>
//===========================================================
// Sam Routines
#include "samkit.h"   // Header Files for routines in samkit.c that do all the heavy lifting
//===========================================================
// Local Variable Types 'n Stuff
	enum access_types { access_none,  Read, Write };
	enum access_size { size_none, Byte, Word, Dword, XBlock };
	enum high_level_command_types { hl_command_none, mem, io, msr, pci, help };
	enum low_level_command_types { ll_command_none,																						// 0
											 Memory_Read_Byte,  Memory_Read_Word,     Memory_Read_Dword, Memory_Read_XMM,    // 1-4     
											 Memory_Write_Byte, Memory_Write_Word,		Memory_Write_Dword,							// 5-7
											 Memory_Write_XMM,  Memory_Detailed_Help, 													// 8-9   

											 IO_Read_Byte,		  IO_Read_Word,		   IO_Read_Dword,           					// 10-12
											 IO_Write_Byte,	  IO_Write_Word,		   IO_Write_Dword,          					// 13-15
											 IO_Detailed_Help,        																			// 16

											 MSR_Read,				MSR_Write,	          MSR_Detailed_Help,       					// 17-19

											 PCI_Read_Byte,		PCI_Read_Word,		   PCI_Read_Dword,          					// 20-22
											 PCI_Write_Byte,		PCI_Write_Word,		PCI_Write_Dword,         					// 23-25
											 PCI_Dump_Device,		PCI_Dump_File,			PCI_Detailed_Help,       					// 26-28

											 Generic_Help  };																						// 29

	struct command
		{
		bool nosudox;											// Did user select nosudo override?
		bool helpx;												// Did user select "?"
		bool errorx;											// Did we detect error in parser (put up detailed ? info)
		enum high_level_command_types Command_Type;	// mem, io, msr, pci, 
		enum low_level_command_types Command_Final;	// the exact command
		unsigned long Address;				// Address (or register, for PCI)
		bool Address_Valid;					// Was a valid address passed?
		unsigned long int Bus;				// Bus Number (for PCI)
		bool Bus_Valid;						// Was a valid Bus Passed? (PCI Only)
		unsigned long int Device;			// Device Number (for PCI only)
		bool Device_Valid;					// Was a valid Device passed? (for PCI only)
		unsigned long int Function;		// Function Number (for PCI)
		bool Function_Valid;					// Was a valid Function passed? (for PCI only)
		long long unsigned int Data;		// Data (for writes)
		bool Data_Valid;						// Was valid data passed? (for writes only)
		enum access_types Access_Type;	// Read, Write
		enum access_size Size;				// Byte, Word, Dword, XBlock
		unsigned long int Length;			// # of bytes to transfer		
		char Filename[255];		 			// Filename (for PCI reg dump)
		unsigned int Filename_int;			// The argv[i] parameter
		double passed_frequency;			// passed frequency
		bool Display_Time;					// Does user want to display time
		};

//===========================================================
// Local Routines 
	void parse_everything(struct command *THE_Command, int argc,      char *argv[]);
	void Execute_Command( struct command *THE_Command, int copyargc,  char copyargv[20][255]);
	void Not_Done_Yet(    struct command *THE_Command, int copyargc,  char copyargv[20][255]);
	void Pretty_Output(   struct command *THE_Command, float result9, char *temp, u8 array11[], float frequency);


//===========================================================
//===========================================================
int main(int argc, char *argv[]) 
	{
	struct command THE_Command;
	int copyargc;
	char copyargv[20][255];
	int i, j;

//===========================================================
// Initialize Parser Data
//===========================================================
	THE_Command.nosudox = false;
	THE_Command.helpx = false;
	THE_Command.errorx = false;
	THE_Command.Command_Type = ll_command_none;				
	THE_Command.Command_Final = hl_command_none;	
	THE_Command.Address = 0;
	THE_Command.Address_Valid = false;
	THE_Command.Bus = 0;
	THE_Command.Bus_Valid = false;
	THE_Command.Device = 0;
	THE_Command.Device_Valid = false;
	THE_Command.Function = 0;
	THE_Command.Function_Valid = false;
	THE_Command.Data = 0;
	THE_Command.Data_Valid = false;
	THE_Command.Access_Type = access_none;				
	THE_Command.Size = size_none;	
	THE_Command.Length = 1;
	strcpy(THE_Command.Filename, "");
	THE_Command.passed_frequency = 0;
	THE_Command.Display_Time = 0;

//===========================================================
// Make Copies of argc, argv
//===========================================================
	copyargc = argc;
	for (i=0; i<argc; i++)
         strcpy(copyargv[i],argv[i]);

//===========================================================
// Capitalize everything in the passed parameters to make things easier:
//===========================================================
	for (i=0; i<argc; i++)
      {
      j = 0;
      while (argv[i][j])
         {
         argv[i][j]=toupper(argv[i][j]);
         j++;
         }
      }

//===========================================================
// Printing All the Parameters (Debug)
//===========================================================
/*
	printf("\n======================================================\n"); 
	SHFprint(argc,1,10, "You passed ", " Parameters:\n");
	int q;
	for (q=0; q < argc; q++)
		{
		SHFprint(q,1,10, "   argc[", "] = ");
		printf("%s\n", argv[q]);
		} 
	printf("\n");
// Seems to work fine!
*/

//===========================================================
// Let's Parse This Mutha
//===========================================================
	parse_everything(&THE_Command, argc, argv);

//===========================================================
// Execute the Commands
//===========================================================
	Execute_Command(&THE_Command, copyargc, copyargv);

//===========================================================
// Cleanup - Debug
//===========================================================

#ifdef DEBUG_STUFF
	SHFprint(argc,1,10, "You passed ", " Parameters:\n");
	printf("HL Command_Type : %i    { (0)hl_command_none, (1)mem, (2)io, (3)msr, (4)pci, (5)help }\n", THE_Command.Command_Type);
	printf("LL Command_Type : %i    { none, then freak'n gobload }\n", THE_Command.Command_Final);
	printf("helpx           : %X\n", (unsigned int)THE_Command.helpx);
	printf("nosudox         : %X\n", (unsigned int)THE_Command.nosudox);
	printf("errorx          : %X\n\n", (unsigned int)THE_Command.errorx);
	printf("Bus             : %X\n",   (unsigned int)THE_Command.Bus);
	printf("Bus_Valid       : %X\n",   (unsigned int)THE_Command.Bus_Valid);
	printf("Device          : %X\n",   (unsigned int)THE_Command.Device);
	printf("Device_Valid    : %X\n",   (unsigned int)THE_Command.Device_Valid);
	printf("Function        : %X\n",   (unsigned int)THE_Command.Function);
	printf("Function_Valid  : %X\n",   (unsigned int)THE_Command.Function_Valid);
	printf("Address         : %X\n",   (unsigned int)THE_Command.Address);
	printf("Address_Valid   : %X\n",   (unsigned int)THE_Command.Address_Valid);
	printf("Access_Type     : %i    { (0)access_none,  (1)Read, (2)Write }\n", THE_Command.Access_Type);
	printf("Data            : %llX\n", THE_Command.Data);
	printf("Data_Valid      : %X\n",   (unsigned int)THE_Command.Data_Valid);
	printf("Size            : %i    { (0)size_none, (1)Byte, (2)Word, (3)Dword, (4)XBlock } \n", THE_Command.Size);
	printf("Length          : %X\n",   (unsigned int)THE_Command.Length);
	printf("Filename        : %s\n",   THE_Command.Filename);
	printf("passed_frequency: %fGHz\n", THE_Command.passed_frequency);	
	printf("Display_Time    : %X\n",    THE_Command.Display_Time);

	printf("Parameters Just Passed: ");
	for (i=0; i < argc; i++)
		{
		printf("%s ", argv[i]);
		} 
	printf("\n");
#endif

// Get rid of annoying error messages	
//	char *tempchar;
//	argc = argc;
//	tempchar = argv[0];
//	tempchar = tempchar;

	return 0;
	}


//===========================================================
//===========================================================
void parse_everything(struct command *THE_Command, int argc, char *argv[])
	{
	int i = 0;
	char * pEnd;
	bool Write_Data_Found = false;
	bool Address_Found = false;
	unsigned long int temp;
//	unsigned long int Temp_Length = 1;


	if (argc <1)
			{
			THE_Command->Command_Type = help;
			THE_Command->Command_Final = Generic_Help;
			}

   while (argv[i])
      {
		// -----------------------------------------------------
		// HELP:  
		if (strncmp(argv[i], "?", 1) == 0)
			THE_Command->helpx = true;
		else if (strncmp(argv[i], "H", 1) == 0)
			THE_Command->helpx = true;
		else if (strncmp(argv[i], "HELP", 4) == 0)
			THE_Command->helpx = true;
		else if (strncmp(argv[i], "-HELP", 5) == 0)
			THE_Command->helpx = true;
		else if (strncmp(argv[i], "-H", 2) == 0)
			THE_Command->helpx = true;
		else if (strncmp(argv[i], "/?", 2) == 0)
			THE_Command->helpx = true;
		else if (strncmp(argv[i], "-?", 2) == 0)
			THE_Command->helpx = true;

		// -----------------------------------------------------
		// nosudo:  
		else if (strncmp(argv[i], "NOSUDO", 6) == 0)
			THE_Command->nosudox = true;

		// -----------------------------------------------------
		// FILENAME:  (Last parameter and pci already given)
		// Must move before the Address (was crashing on filenames starting with "A"-"F"!)
		// !Address_Found.  If BB:DD.F-R was previously passed, this parameter could be B D W (size) and not filename
		// !(strchr(argv[i], ':'))  Is necessary for sudo ./samtool pci 00:0x1F.00-04=0x0fff
		else if ( (i == (argc-1)) && (THE_Command->Command_Type == pci) && (!Address_Found) && !(strchr(argv[i], ':')) )
			{
			strcpy(THE_Command->Filename, argv[i]);			// Don't want all caps filename!
			THE_Command->Command_Final = PCI_Dump_File;
			THE_Command->Filename_int = i;
			}

		// -----------------------------------------------------
		// COMMAND_TYPE:  
		else if (strncmp(argv[i], "MEM", 3) == 0)
			THE_Command->Command_Type = mem;
		else if (strncmp(argv[i], "IO", 2) == 0)
			THE_Command->Command_Type = io;
		else if (strncmp(argv[i], "MSR", 3) == 0)
			THE_Command->Command_Type =msr;
		else if (strncmp(argv[i], "PCI", 3) == 0)
			THE_Command->Command_Type = pci;

		// -----------------------------------------------------
		// SIZE:
		else if (strncmp(argv[i], "B", 1) == 0)
			THE_Command->Size = Byte;
		else if (strncmp(argv[i], "BYTE", 4) == 0)
			THE_Command->Size = Byte;
		else if (strncmp(argv[i], "W", 1) == 0)
			THE_Command->Size = Word;
		else if (strncmp(argv[i], "WORD", 4) == 0)
			THE_Command->Size = Word;
		else if (strncmp(argv[i], "D", 1) == 0)		// DEBUG - 'd' COULD be first name of filename!
			THE_Command->Size = Dword;
		else if (strncmp(argv[i], "DWORD", 5) == 0)
			THE_Command->Size = Dword;
		else if (strncmp(argv[i], "X", 1) == 0)
			THE_Command->Size = XBlock;
		else if (strncmp(argv[i], "BLOCK", 5) == 0)
			THE_Command->Size = XBlock;
		else if (strncmp(argv[i], "XBLOCK", 5) == 0)
			THE_Command->Size = XBlock;


		// -----------------------------------------------------
		// passed_frequency
		// Look for "F=#.#" and passed_frequency
		else if ( (argv[i][0]=='F') && (argv[i][1]=='=') ) 			// Does string contain first two letters of "F="?
			{
			THE_Command->passed_frequency = strtod (&argv[i][2],&pEnd);
			THE_Command->Display_Time = true;
			}
		else if ( (argv[i][0]=='F') && (argv[i][1]=='\0') ) 			// Does string contain just one letter of "F"?
			THE_Command->Display_Time = true;

		// -----------------------------------------------------
		// PCI BUS:DEVICE.FUNCTION-REGISTER
		// Look for ":" and decode Bus:Device.Function  (possibly -register).
		else if (strchr(argv[i], ':'))			// Does string contain a ':'?
			{
			// NOTE:  I don't handle "h"'s	
			// 00:1F.0-40

			// It's gotta be PCI access with a ':'!
			THE_Command->Command_Type = pci;
//			THE_Command->Address_Valid = true;					// BB:DD.F IS the PCI Address!  This line needed since follow on 0x#### will be length.
																			// Deleted.   pci 00:0x1f.00 was causing device to not come up
			Address_Found = true;
			THE_Command->Bus = strtol (argv[i],&pEnd,0);
			THE_Command->Bus_Valid = true;

			if (strcmp(pEnd,"") !=0)
				{
//				printf("pEnd: %s\n",pEnd);
				pEnd = pEnd +1;
				THE_Command->Device = strtol (pEnd,&pEnd,0);
				THE_Command->Device_Valid = true;
				}

			if (strcmp(pEnd,"") !=0)
				{
//				printf("pEnd: %s\n",pEnd);
				pEnd = pEnd +1;
				THE_Command->Function = strtol (pEnd,&pEnd,0);
				THE_Command->Function_Valid = true;
				}

			if (strcmp(pEnd,"") !=0)
				{
//				printf("pEnd: %s\n",pEnd);
				pEnd = pEnd +1;
				THE_Command->Address = strtol (pEnd,NULL,0);
				THE_Command->Address_Valid = true;
				Address_Found = true;
				}

			// Write data included with Address:  ex:  mem 0xA000=0x34
			if ( strchr(argv[i], '=') )			// Does string contain a ':' and this argcv is not f=
				{
				THE_Command->Access_Type = Write;
				pEnd = strchr(argv[i], '=') +1;
				if (pEnd)
					{
//						THE_Command->Data = strtol (pEnd,&pEnd,0);
					THE_Command->Data = strtoul (pEnd,&pEnd,0);
					THE_Command->Data_Valid = true;
					Write_Data_Found = true;
					}				
				}

			}

		// -----------------------------------------------------
		// ADDRESS/DATA/LENGTH:
		else if ( isxdigit(argv[i][0])  )			// First letter number, can't be anything else
			{
			temp = strtol (argv[i],&pEnd,0);

			if (!Address_Found)
				{
				THE_Command->Address = temp;
				THE_Command->Address_Valid = true;
				Address_Found = true;

				// Write data included with Address:  ex:  mem 0xA000=0x34
				if ( strchr(argv[i], '=')  )			// Does string contain a ':' and this argcv is not f=
					{
					THE_Command->Access_Type = Write;
					pEnd = strchr(argv[i], '=') +1;
					if (pEnd)
						{
//						THE_Command->Data = strtol (pEnd,&pEnd,0);
						THE_Command->Data = strtoul (pEnd,&pEnd,0);
						THE_Command->Data_Valid = true;
						Write_Data_Found = true;
						}				
					}
				}
			else if ( (THE_Command->Access_Type == Write) && (!Write_Data_Found) )
				{
				THE_Command->Data = temp;
				THE_Command->Data_Valid = true;
				Write_Data_Found = true;
				}
			else 
				THE_Command->Length = temp;
			}

		// -----------------------------------------------------
		// WRITE_DATA: (when it's by itself  ex:  mem 0xA000 =0x300
		// Look for "=" and set the WRITE attribute, and yank out the data
		else if ( strchr(argv[i], '=')  )			// Does string contain a ':' and this argcv is not f=
			{
			THE_Command->Access_Type = Write;
			pEnd = strchr(argv[i], '=') +1;
			if (pEnd)
				{
//				THE_Command->Data = strtol (pEnd,&pEnd,0);
				THE_Command->Data = strtoul (pEnd,&pEnd,0);
				THE_Command->Data_Valid = true;
				Write_Data_Found = true;
				}				
			}

		// -----------------------------------------------------
		i++;
      }  // end of while


	// -----------------------------------------------------
	//  NOW we make final determination of THE_Command->Command_Final
	if (THE_Command->Command_Type == hl_command_none)		// The didn't enter a command
			THE_Command->Command_Final = Generic_Help;

	else if ( (THE_Command->helpx == true) && (THE_Command->Command_Type == mem) )
			THE_Command->Command_Final = Memory_Detailed_Help;

	else if ( (THE_Command->helpx == true) && (THE_Command->Command_Type == io) )
			THE_Command->Command_Final = IO_Detailed_Help;

	else if ( (THE_Command->helpx == true) && (THE_Command->Command_Type == msr) )
			THE_Command->Command_Final = MSR_Detailed_Help;

	else if ( (THE_Command->helpx == true) && (THE_Command->Command_Type == pci) )
			THE_Command->Command_Final = PCI_Detailed_Help;


	// --------------------- START MEMORY------------------------------------------------
	else if (THE_Command->Command_Type == mem)
		{
		// Valid Checks:  Address							Data (write only)   
		if ( (THE_Command->Address_Valid == false)                                           ||		// No Addres
			  ( (THE_Command->Access_Type == Write) && (THE_Command->Data_Valid == false) ) )			// Write, no Data
			{
			THE_Command->Command_Final = Memory_Detailed_Help;
			THE_Command->errorx = true;
			}			
		else
			{
			// Optional:      Access_Type (default r)   Size (default to byte if none)
			if (THE_Command->Access_Type == access_none) 	// Default to read if none passed
				THE_Command->Access_Type = Read;
			if ( (THE_Command->Size == size_none) && (THE_Command->Access_Type == Write) )	// Data can override if write
				{
				if (THE_Command->Data <= 0xFF)
					THE_Command->Size = Byte;
				else if (THE_Command->Data <= 0xFFFF)
					THE_Command->Size = Word;
				else if (THE_Command->Data <= 0xFFFFFFFF)
					THE_Command->Size = Dword;
				}
			if ( (THE_Command->Size == size_none) && (THE_Command->Access_Type == Read) )	// Default Byte for Reads
				THE_Command->Size = Byte;

			// let's calculate the Command_Final! (finally!)
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Byte) )
				THE_Command->Command_Final = Memory_Read_Byte;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Word) )
				THE_Command->Command_Final = Memory_Read_Word;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Dword) )
				THE_Command->Command_Final = Memory_Read_Dword;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == XBlock) )
				THE_Command->Command_Final = Memory_Read_XMM;

			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Byte) )
				THE_Command->Command_Final = Memory_Write_Byte;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Word) )
				THE_Command->Command_Final = Memory_Write_Word;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Dword) )
				THE_Command->Command_Final = Memory_Write_Dword;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == XBlock) )
				THE_Command->Command_Final = Memory_Write_XMM;
			}
		}

// --------------------- START IO--------------------------------------------------
	else if (THE_Command->Command_Type == io)
		{
		// Valid Checks:  Address							Data (write only)		(XMM)   
		if ( (THE_Command->Address_Valid == false) ||		// No Addres
			  (THE_Command->Size == XBlock) )					// XMM not allowed on IO
			{
			THE_Command->Command_Final = IO_Detailed_Help;
			THE_Command->errorx = true;
			}			
		else
			{
			// Optional:      Access_Type (default r)   Size (default to byte if none)
			if (THE_Command->Access_Type == access_none) 	// Default to read if none passed
				THE_Command->Access_Type = Read;
			if ( (THE_Command->Size == size_none) && (THE_Command->Access_Type == Write) )	// Data can override if write
				{
				if (THE_Command->Data <= 0xFF)
					THE_Command->Size = Byte;
				else if (THE_Command->Data <= 0xFFFF)
					THE_Command->Size = Word;
				else if (THE_Command->Data <= 0xFFFFFFFF)
					THE_Command->Size = Dword;
				}
			if ( (THE_Command->Size == size_none) && (THE_Command->Access_Type == Read) )	// Default Byte for Reads
				THE_Command->Size = Byte;

			// let's calculate the Command_Final! (finally!)
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Byte) )
				THE_Command->Command_Final = IO_Read_Byte;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Word) )
				THE_Command->Command_Final = IO_Read_Word;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Dword) )
				THE_Command->Command_Final = IO_Read_Dword;

			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Byte) )
				THE_Command->Command_Final = IO_Write_Byte;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Word) )
				THE_Command->Command_Final = IO_Write_Word;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Dword) )
				THE_Command->Command_Final = IO_Write_Dword;
			}
		}

// --------------------- START MSR--------------------------------------------------
	else if (THE_Command->Command_Type == msr)
		{
		// Valid Checks:  Address							Data (write only)   
		if ( (THE_Command->Address_Valid == false)                                           ||		// No Addres
			  ( (THE_Command->Access_Type == Write) && (THE_Command->Data_Valid == false) ) )			// Write, no Data
			{
			THE_Command->Command_Final = MSR_Detailed_Help;
			THE_Command->errorx = true;
			}			
		else
			{
			// Optional:      Access_Type (default r)   Size (default to byte if none)
			if (THE_Command->Access_Type == access_none) 	// Default to read if none passed
				THE_Command->Access_Type = Read;

			// let's calculate the Command_Final! (finally!)
			if (THE_Command->Access_Type == Read)
				THE_Command->Command_Final = MSR_Read;

			if (THE_Command->Access_Type == Write)
				THE_Command->Command_Final = MSR_Write;
			}
		}

	// ------------------ START PCI----------------------------------------------
	else if (THE_Command->Command_Type == pci)
		{
		// Valid Checks:  Filenane (don't need anything else		B:D.F (device)  	B:D.F-R (full)   
		if (strcmp(THE_Command->Filename,"") !=0)
			THE_Command->Command_Final = PCI_Dump_File;
		else if ( (THE_Command->Bus_Valid == true ) && (THE_Command->Device_Valid == true ) && (THE_Command->Function_Valid == true ) && (THE_Command->Address_Valid == false )   )
			THE_Command->Command_Final = PCI_Dump_Device;
		else if ( (THE_Command->Bus_Valid == false ) || (THE_Command->Device_Valid == false ) || (THE_Command->Function_Valid == false ) || (THE_Command->Address_Valid == false )   )
			{
			THE_Command->Command_Final = PCI_Detailed_Help;
			THE_Command->errorx = true;
			}
		else if (THE_Command->Size == XBlock)																			// XMM not allowed on PCI
			{
			THE_Command->Command_Final = PCI_Detailed_Help;
			THE_Command->errorx = true;
			}

		else
			{
			// Optional:      Access_Type (default r)   Size (default to byte if none)
			if (THE_Command->Access_Type == access_none) 	// Default to read if none passed
				THE_Command->Access_Type = Read;
			if ( (THE_Command->Size == size_none) && (THE_Command->Access_Type == Write) )	// Data can override if write
				{
				if (THE_Command->Data <= 0xFF)
					THE_Command->Size = Byte;
				else if (THE_Command->Data <= 0xFFFF)
					THE_Command->Size = Word;
				else if (THE_Command->Data <= 0xFFFFFFFF)
					THE_Command->Size = Dword;
				}
			if ( (THE_Command->Size == size_none) && (THE_Command->Access_Type == Read) )	// Default Byte for Reads
				THE_Command->Size = Byte;

			// let's calculate the Command_Final! (finally!)
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Byte) )
				THE_Command->Command_Final = PCI_Read_Byte;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Word) )
				THE_Command->Command_Final = PCI_Read_Word;
			if ( (THE_Command->Access_Type == Read) && (THE_Command->Size == Dword) )
				THE_Command->Command_Final = PCI_Read_Dword;

			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Byte) )
				THE_Command->Command_Final = PCI_Write_Byte;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Word) )
				THE_Command->Command_Final = PCI_Write_Word;
			if ( (THE_Command->Access_Type == Write) && (THE_Command->Size == Dword) )
				THE_Command->Command_Final = PCI_Write_Dword;
			}
		}
	}  // end of parse_everything.  What a Pain


//===========================================================
//===========================================================
void Execute_Command(struct command *THE_Command, int copyargc, char copyargv[20][255])
	{
	int q;
	float result9;
	float temp_result9;
 	char temp[10];
	u8 u8return_data6 = 0x00;
	u16 u16return_data6 = 0x00;
	u32 u32return_data6 = 0x00;
	float frequency9;
//	u8 array11[0x400000];		// 4MB array  (worked!)
// u8 array11[0x800000];		// 8MB array (Didn't work!)
	u8 *array11;


	unsigned long long ret = 0;
	char tempstr[100];
	unsigned int cx;
	unsigned int Found_Size;  // Device Not Found = 0x00.  = 255/4K otherwise.

	array11 =  malloc(0x40000000 * sizeof(u8));		// 1GB


// ----- Generic Help -----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if ( (THE_Command->Command_Final == Generic_Help) || 
        (THE_Command->Command_Final == ll_command_none)  )
		{
      printf("\n===================================================================================================\n"); 
		fprintf(stderr, "USAGE: sudo %s {mem/io/msr/pci} {address} {=data (for write)} {b/w/d/x} {length} {?} {f{=#.#}}\n"
			"  {mem/io/msr/pci}      - Register Type:   Memory, I/O, MSR, PCI\n"
			"  {address}             - Address:         0x######### for (mem, IO, MSR) - In Hexadecimal\n"
			"                                           BB:D.F-0x## for (PCI)\n"
			"                                           BB:D.F          (PCI Device Dump)\n"
			"  {=data (for writes)}  - Data to Write:   =0x####         (In Hexadecimal)\n"
			"  {b/w/d/x}             - Access Size:     Byte, Word, Dword, XMM\n"
			"  {length (total size)} - # of Bytes TOTAL                 (mem only)\n"
			"                                                           (# of 4K blocks for xmm)\n"
			"                                                           (max of 512MB = 0x20000000/0x20000 for xmm))\n"
			"  {?}                   - Extended Help                    (with {mem/io/msr/pci} )\n"
			"  {f{=#.#}}             - Measure Time.    Freq in GHz     (#.# Opt - Else tool calculates using HPET/TSC)\n\n"

			"EXAMPLE:  sudo %s mem 0xFFFFFFF0 d 0x10 f\n"
			"  Memory Read from 0xFFFFFFF0 (dword access).  Total of 0x10 bytes read.  Measure latency/performance\n"
			"                                                                          [BIOS Boot Vector]\n\n"
			"EXAMPLE:  sudo %s {mem/io/msr/pci} ?    Extended Help & Examples  \n\n",
			copyargv[0], copyargv[0], copyargv[0]);
      printf("---------------------------------------------------------------------------------------------------\n"); 
	
//		if (THE_Command->Command_Type == hl_command_none)
//	      printf("* ERRORS DETECTED!! *  Check parameters.  Use {mem/io/msr/pci} ? for more detailed help\n"); 

		printf("Parameters Just Passed: ");
		for (q=0; q < copyargc; q++)
			{
			printf("%s ", copyargv[q]);
			} 
      printf("\n\n"); 
		printf(VERSION_STRING); printf ("\n");
		printf(AUTHOR_STRING); printf ("\n");
		printf(COPYRIGHT_STRING); 
      printf("\n===================================================================================================\n\n"); 
		}

// ----- Memory Detailed Help ---------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------

	if (THE_Command->Command_Final == Memory_Detailed_Help)
		{
      printf("\n===================================================================================================\n"); 
		fprintf(stderr, "USAGE:\tsudo %s mem address {=data (for write)} {b/w/d/x} {length} {f{=#.#}}\n"
			"  {address}             - Address:         0x#########\n"
			"  {=data (for writes)}  - Data to Write:   =0x####             (Opt.  Only for Writes.  In Hexadecimal)\n"
			"  {b/w/d/x}             - Access Size:     Byte/Word/DWord/XMM (Opt.  Defaults to Byte)\n"
			"  {length (total size)} - # of Bytes TOTAL                     (Opt.  Defaults to Access Size)\n"
  			"                                                               (# of 4K blocks for xmm)\n"
			"                                                               (max of 512MB = 0x20000000 {0x20000 for xmm})\n"
			"  {f{=#.#}}             - Measure Time.    Freq in GHz         (#.# Optional.  Otherwise tool calculates)\n\n"


			"EXAMPLES:\n"
		   "  sudo %s mem 0xFED000F0 d 8 f            Mem. Rd.from 0xFED000F0.       Dword Reads of 8 bytes.  Time measured.\n"
		   "                                                                                Will calculate frequency.\n"
		   "                                                                                                            [HPET Timer]\n"
		   "  sudo %s mem 0xC0000 w 0x20              Mem. Rd.from 0xC0000           Word Reads of 0x20 bytes.\n"
         "                                                                                                              [VGA BIOS]\n"
		   "  sudo %s mem 0xA0000=0x1122 0x40         Mem. Wr.of 0x1122 to 0xA0000   Word Writes of 0x1122.  0x40 \n"
		   "                                                                                consecutive bytes\n"
         "                                                                                                               [VGA Mem]\n"
		   "  sudo %s mem 0x90000000=0x11 xmm 0x10 f  Mem. Wr.of 0x11 to 0x90000000  Block Write of 0x11.  (0x10*4k)=64KB \n"
		   "                                                                                consecutive bytes written\n"
		   "                                                                                using XMM instructions.(Calc Freq)\n"
		   "                                                                                                                 [MMIO]\n"
  			"  sudo %s mem 0x90000000 x 0x40 f=2.0     Mem. Rd.from 0x90000000        Block Read. (0x40*4k)=256KB consecutive\n"
  			"                                                                                bytes read using XMM instructions.\n"
  			"                                                                                Use 2.0Ghz for freq.\n"
  			"                                                                                                                 [MMIO]\n",
			copyargv[0], copyargv[0], copyargv[0], copyargv[0], copyargv[0], copyargv[0]);
      printf("---------------------------------------------------------------------------------------------------\n"); 
	
		if (THE_Command->errorx)
	      printf("* ERRORS DETECTED!! *  Check parameters.  \n"); 

		printf("Parameters Just Passed: ");
		for (q=0; q < copyargc; q++)
			{
			printf("%s ", copyargv[q]); 
			} 
      printf("\n===================================================================================================\n\n"); 
		}

// ----- IO Detailed Help -------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------

	if (THE_Command->Command_Final == IO_Detailed_Help)
		{
      printf("\n===================================================================================================\n"); 
		fprintf(stderr, "USAGE:\tsudo %s io address {=data (for write)} {b/w/d} \n"
			"   {address}             - Address:         0x#########\n"
			"   {=data (for writes)}  - Data to Write:   =0x####             (Optional.  Only for Writes.  In Hexadecimal)\n"
			"   {b/w/d}               - Access Size:     Byte/Word/DWord     (Optional.  Defaults to Byte)\n\n"

			"EXAMPLES:\n"
		   "   sudo %s io 0x80        IO Rd. from        0x80.   Byte Access (Default).  1 Byte Read (Default). [Port 0x80]\n"
		   "   sudo %s io 0x80=0xBA   IO Wr. of 0xBA to  0x80    Byte Access (defined by data).                 [Port 0x80]\n"
		   "   sudo %s io 0xCF8 d     IO Rd. from        0xCF8.  Dword Access.                         [PCI CONFIG_ADDRESS]\n",
			copyargv[0], copyargv[0], copyargv[0], copyargv[0]);
      printf("---------------------------------------------------------------------------------------------------\n"); 
	
		if (THE_Command->errorx)
	      printf("* ERRORS DETECTED!! *  Check parameters.  \n"); 

		printf("Parameters Just Passed: ");
		for (q=0; q < copyargc; q++)
			{
			printf("%s ", copyargv[q]);
			} 
      printf("\n===================================================================================================\n\n"); 
		}
	
// ----- MSR Detailed Help ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------

	if (THE_Command->Command_Final == MSR_Detailed_Help)
		{
      printf("\n===================================================================================================\n"); 
		fprintf(stderr, "USAGE:\tsudo %s msr address {=data (for write)} {nosudo}\n"
			"   {address}             - Address:         0x#########\n"
			"   {=data (for writes)}  - Data to Write:   =0x####     (Optional.  Only for Writes.  In Hexadecimal)\n"
			"   {nosudo}              - nosudo option                (Optional.  Omit 'sudo' from modprobe msr cmd)\n\n"

			"NOTE:\n"
		   "   'sudo modprobe msr' executed before the msr command is issued.\n\n"

			"EXAMPLES:\n"
		   "   sudo %s msr 0x10         MSR Rd. from        0x10                                       [Time Stamp Counter]\n"
		   "   sudo %s msr 0x10 nosudo  MSR Rd. from        0x10 (omit 'sudo' from 'modprobe msr' cmd) [Time Stamp Counter]\n"
		   "   sudo %s msr 0xC3=0x10    MSR Wr. of 0x10 to  0xC3                                       [Gen. Perf. Counter]\n",
			copyargv[0], copyargv[0], copyargv[0], copyargv[0]);
      printf("---------------------------------------------------------------------------------------------------\n"); 
	
		if (THE_Command->errorx)
	      printf("* ERRORS DETECTED!! *  Check parameters.  \n"); 

		printf("Parameters Just Passed: ");
		for (q=0; q < copyargc; q++)
			{
			printf("%s ", copyargv[q]);
			} 
      printf("\n===================================================================================================\n\n"); 
		}
	
	if (THE_Command->Command_Final == PCI_Detailed_Help)
		{
      printf("\n===================================================================================================\n"); 
		fprintf(stderr, "USAGE:\tsudo %s pci {BB:DD.F-{R}} {=data (for write)} {nosudo} {Filename} \n"
			"   {BB:DD.F-{R}}         - Bus:Device.Function-Register                  ({R} Optional. Dumps Whole BB:DD.F if missing)\n"
			"   {=data (for writes)}  - Data to Write =0x####                         (Optional.     Only for Writes.  In Hexadecimal)\n"
			"   {nosudo}              - nosudo option                                 (Optional.     Omit 'sudo' before lspci -xxxx cmd)\n"
			"   {Filename}            - Filename (MUST BE LAST PARAMETER IF PRESENT!) (Optional.     Dumps all PCI Regs to File)\n\n"

			"EXAMPLES:\n"
		   "  sudo %s pci 00:0x00.0x00-0x00 D   PCI Rd. from 00:00.00-00 (Dword)       4 Bytes Read (Default). [Dev ID]\n"
		   "  sudo %s pci 00:0x1F.00-04=0x0FFF  PCI Wr. of 0x0FFF to 00:0x1F.00-04     Word Access.       [PCI CMD Reg]\n"
		   "  sudo %s pci 00:0x1D.00            PCI Rd. from 00:0x1D.00                Entire PCI space read. [USB Cnt]\n"
		   "  sudo %s pci Registers.txt         PCI Rd. of Entire PCI Space.           Stored in filename, Registers.txt\n"
		   "  %s pci nosudo Registers.txt       PCI Rd. of Entire PCI Space. (no sudo) Stored in filename, Registers.txt\n",
			copyargv[0], copyargv[0], copyargv[0], copyargv[0], copyargv[0], copyargv[0]);
      printf("---------------------------------------------------------------------------------------------------\n"); 
	
		if (THE_Command->errorx)
	      printf("* ERRORS DETECTED!! *  Check parameters.  \n"); 

		printf("Parameters Just Passed: ");
		for (q=0; q < copyargc; q++)
			{
			printf("%s ", copyargv[q]);
			} 
      printf("\n===================================================================================================\n\n"); 
		}

// ----- Memory Read Byte -------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Read_Byte)
		{

		strcpy(temp, "us");

		if (THE_Command->Length < 1)		// If user didn't pick a length (in bytes), need to make sure at least a word
			THE_Command->Length = 1;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}
/*
		// This works perfectly
		address9 = 0xFFFFFFF0;
		result9 = assembly_delay(address9, temp, &u32return_data9, frequency9);
		SHFprint(address9, 8, 0x10,"Rd from 0x"," = ");
		SHFprint(u32return_data9, 8, 0x10,"0x"," (PCH SPI)\t");
		printf("Time Delay:  %f", result9);
		printf(" %s\n", temp);

		// This now works
		result9 = assembly_delay(THE_Command->Address, temp, &u32return_data9, frequency9);
		SHFprint(THE_Command->Address, 8, 0x10,"Rd from 0x"," = ");
		SHFprint(u32return_data9, 8, 0x10,"0x"," (PCH SPI)\t");
		printf("Time Delay:  %f", result9);
		printf(" %s\n", temp);

		// This now works
		result9 = read_byte_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11);
		SHFprint(THE_Command->Address, 8, 0x10,"Rd from 0x"," = ");

		SHFprint(array11[3], 2, 0x10,"0x","");
		for (i=2; i>=0 ;i--)
			SHFprint(array11[i], 2, 0x10,"","");
		printf(" (PCH SPI)\t");
		printf("Time Delay:  %f", result9);
		printf(" %s\n", temp);
*/
		result9 = read_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 1);
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
 		}

// ----- Memory Read Word -------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Read_Word)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 2)		// If user didn't pick a length (in bytes), need to make sure at least a word
			THE_Command->Length = 2;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}
		result9 = read_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 2);
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- Memory Read Dword ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Read_Dword)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 4)		// If user didn't pick a length (in bytes), need to make sure at least a dword
			THE_Command->Length = 4;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}
		result9 = read_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 4);
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- Memory Read XMM --------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Read_XMM)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 1)		// If user didn't pick a length (in 4K byte blocks), need to make sure at least x1
			THE_Command->Length = 1;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}

		result9 = block_read_assembly_delay_new(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11);
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- Memory Write Byte ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Write_Byte)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 1)		// If user didn't pick a length (in bytes), need to make sure at least a byte
			THE_Command->Length = 1;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}

		// Need to set array11 up with the write data
		q = 0;
		while (q<4096)
			{
			array11[q]=(THE_Command->Data & 0x000000FF);
			q = q+1;
			}

		temp_result9 = write_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 1);

		// The user wants to see the data read back, confirm read:
		result9 = read_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 1);
		Pretty_Output(THE_Command, temp_result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- Memory Write Word ------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Write_Word)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 2)		// If user didn't pick a length (in bytes), need to make sure at least a word
			THE_Command->Length = 2;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}

		// Need to set array11 up with the write data
		q = 0;
		while (q<4096)
			{
			array11[q]=   (THE_Command->Data & 0x000000FF);
			array11[q+1]=((THE_Command->Data & 0x0000FF00)>>8);
			q = q+2;
			}

		temp_result9 = write_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 2);

		// The user wants to see the data read back, confirm read:
		result9 = read_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 2);
		Pretty_Output(THE_Command, temp_result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- Memory Write Dword -----------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Write_Dword)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 4)		// If user didn't pick a length (in bytes), need to make sure at least a dword
			THE_Command->Length = 4;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}

		// Need to set array11 up with the write data
		q = 0;
		while (q<4096)
			{
			array11[q]=   (THE_Command->Data & 0x000000FF);
			array11[q+1]=((THE_Command->Data & 0x0000FF00)>>8);
			array11[q+2]=((THE_Command->Data & 0x00FF0000)>>16);
			array11[q+3]=((THE_Command->Data & 0xFF000000)>>24);
			q = q+4;
			}

		temp_result9 = write_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 4);

		// The user wants to see the data read back, confirm read:
		result9 = read_assembly_delay(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11, 4);
		Pretty_Output(THE_Command, temp_result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- Memory Write XMM -------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == Memory_Write_XMM)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 1)		// If user didn't pick a length (in 4K byte blocks), need to make sure at least x1
			THE_Command->Length = 1;

		if ( (THE_Command->passed_frequency == (double)0.0) && (THE_Command->Display_Time) )
			{
			printf("You didn't pass a system frequency via command line parameter 'f=#.##'\n");
			printf("Please wait five seconds. Using HPET and TSC to calculate System Frequency.\n");
			frequency9 = Freq_Calc(); // This will be 3.2 for 3.2 GHz
			printf("Calculated Frequency \t= %f GHz\n\n", frequency9/1000000000);
			THE_Command->passed_frequency = frequency9/1000000000;
			}

		q = 0;
		while (q<0x10000)
			{
			array11[q]=    (THE_Command->Data & 0x00000000000000FF);
			array11[q+1]= ((THE_Command->Data & 0x000000000000FF00)>>8);
			array11[q+2]= ((THE_Command->Data & 0x0000000000FF0000)>>16);
			array11[q+3]= ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+4]= ((THE_Command->Data & 0x000000FF00000000)>>32);
			array11[q+5]= ((THE_Command->Data & 0x0000FF0000000000)>>40);
			array11[q+6]= ((THE_Command->Data & 0x00FF000000000000)>>48);
			array11[q+7]= ((THE_Command->Data & 0xFF00000000000000)>>56);
			array11[q+8]= 0;  // ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+9]= 0;  // ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+10]=0; // ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+11]=0; // ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+12]=0; //((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+13]=0; // ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+14]=0; // ((THE_Command->Data & 0x00000000FF000000)>>24);
			array11[q+15]=0; //((THE_Command->Data & 0x00000000FF000000)>>24);
			q = q+0x10;		// XMM instructions send 16 bytes at a time (I can only handle ull, I'm afraid.
			}

		temp_result9 = block_write_assembly_delay_new(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11);

		// The user wants to see the data read back, confirm read:
		result9 = block_read_assembly_delay_new(THE_Command->Address, temp, THE_Command->passed_frequency*1000000000, THE_Command->Length, array11);
		Pretty_Output(THE_Command, temp_result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- IO Read Byte -----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == IO_Read_Byte)
		{
		THE_Command->Length = 1;
		u8return_data6 = SHF_IO_read_byte(THE_Command->Address);
		THE_Command->Display_Time = false;								// Don't care about time in IO commands
		array11[0]=u8return_data6;
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- IO Read Word -----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == IO_Read_Word)
		{
		THE_Command->Length = 2;
		u16return_data6 = SHF_IO_read_word(THE_Command->Address);
		THE_Command->Display_Time = false;								// Don't care about time in IO commands
		array11[0]=u16return_data6 & 0x00FF;
		array11[1]=(u16return_data6 & 0xFF00)>>8;
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
		}

// ----- IO Read Dword-----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == IO_Read_Dword)
		{
		THE_Command->Length = 4;
		u32return_data6 = SHF_IO_read_dword(THE_Command->Address);
		THE_Command->Display_Time = false;								// Don't care about time in IO commands
		array11[0]= u32return_data6 & 0x000000FF;
		array11[1]=(u32return_data6 & 0x0000FF00) >>8;
		array11[2]=(u32return_data6 & 0x00FF0000) >>16;
		array11[3]=(u32return_data6 & 0xFF000000) >>24;
		Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
//		SHFprint(u32return_data6, 8, 0x10,"U32return_data6 0x","\n");

		}

// ----- IO Write Byte ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == IO_Write_Byte)
		{
		THE_Command->Length = 1;
		THE_Command->Display_Time = false;								// Don't care about time in IO commands
		u8return_data6	= THE_Command->Data & 0x000000FF;
		SHF_IO_write_byte(THE_Command->Address, u8return_data6);
		printf("============================================================\n");
		printf("No follow-On reads for I/O Write Commands (Can cause unexpected behaviors!\n");
		printf("============================================================\n\n");
		}

// ----- IO Write Word ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == IO_Write_Word)
		{
		THE_Command->Length = 2;
		THE_Command->Display_Time = false;								// Don't care about time in IO commands
		u16return_data6	= THE_Command->Data & 0x0000FFFF;
		SHF_IO_write_word(THE_Command->Address, u16return_data6);
		printf("============================================================\n");
		printf("No follow-On reads for I/O Write Commands (Can cause unexpected behaviors!\n");
		printf("============================================================\n\n");
		}

// ----- IO Write Dword ---------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == IO_Write_Dword)
		{
		THE_Command->Length = 4;
		THE_Command->Display_Time = false;								// Don't care about time in IO commands
		u32return_data6	= THE_Command->Data & 0xFFFFFFFF;
		SHF_IO_write_dword(THE_Command->Address, u32return_data6);
		printf("============================================================\n");
		printf("No follow-On reads for I/O Write Commands (Can cause unexpected behaviors!\n");
		printf("============================================================\n\n");
		}

// ----- MSR Read ---------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == MSR_Read)
		{

		if (THE_Command->nosudox == true)
			system("modprobe msr");		
		else
			system("sudo modprobe msr");		

		SHF_rdmsr(0, THE_Command->Address, &ret); 
		printf("============================================================\n");
		printf("Return Data:  MSR(0x");
		SHFprint(THE_Command->Address, 2, 0x10,"",") = 0x");
		SHFprint(ret, 16, 0x10,"","\n");
		printf("============================================================\n\n");
		}

// ----- MSR Write --------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == MSR_Write)
		{
		if (THE_Command->nosudox == true)
			{
			cx = snprintf(tempstr, 100, "wrmsr 0x%x 0x%x", (unsigned int)THE_Command->Address, (unsigned int)THE_Command->Data);
			system("modprobe msr");		
			}
		else
			{
			cx = snprintf(tempstr, 100, "sudo wrmsr 0x%x 0x%x", (unsigned int)THE_Command->Address, (unsigned int)THE_Command->Data);
			system("sudo modprobe msr");		
			}

		strcat(tempstr, "\0");
		cx = cx;
//		printf("Final String: %s\n",tempstr);

		system(tempstr);		

		// Confirmation Read:
		printf("============================================================\n");
		SHF_rdmsr(0, THE_Command->Address, &ret); 
		printf("Confirmation Read:  MSR(0x");
		SHFprint(THE_Command->Address, 2, 0x10,"",") = 0x");
		SHFprint(ret, 2, 0x10,"","\n");
		printf("============================================================\n\n");

		}

// ----- PCI Read Byte ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Read_Byte)
		{
		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			THE_Command->Length = 1;
		   array11[0] = SHFpci_read_byte(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address);
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Read Word ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Read_Word)
		{
		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			THE_Command->Length = 2;
			u16return_data6 = SHFpci_read_word(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address);
		   array11[0] = u16return_data6 & 0x00FF;
		   array11[1] = (u16return_data6 & 0xFF00)>>8;
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Read Dword ---------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Read_Dword)
		{
		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			THE_Command->Length = 4;
			u32return_data6 = SHFpci_read_dword(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address);
			array11[0]= u32return_data6 & 0x000000FF;
			array11[1]=(u32return_data6 & 0x0000FF00) >>8;
			array11[2]=(u32return_data6 & 0x00FF0000) >>16;
			array11[3]=(u32return_data6 & 0xFF000000) >>24;
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Write Byte ---------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Write_Byte)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 1)		// If user didn't pick a length (in bytes), need to make sure at least a byte
			THE_Command->Length = 1;

		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			THE_Command->Length = 1;
			SHFpci_write_byte(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address, THE_Command->Data);
			// The user wants to see the data read back, confirm read:
		   array11[0] = SHFpci_read_byte(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address);
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Write Word ---------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Write_Word)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 2)		// If user didn't pick a length (in bytes), need to make sure at least a word
			THE_Command->Length = 2;

		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			THE_Command->Length = 2;
			SHFpci_write_word(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address, THE_Command->Data);
			// The user wants to see the data read back, confirm read:
			u16return_data6 = SHFpci_read_word(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address);
		   array11[0] = u16return_data6 & 0x00FF;
		   array11[1] = (u16return_data6 & 0xFF00)>>8;
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Write Dword --------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Write_Dword)
		{
		strcpy(temp, "us");

		if (THE_Command->Length < 4)		// If user didn't pick a length (in bytes), need to make sure at least a dword
			THE_Command->Length = 4;

		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			THE_Command->Length = 4;
			SHFpci_write_dword(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address, THE_Command->Data);
			// The user wants to see the data read back, confirm read:
			u32return_data6 = SHFpci_read_dword(THE_Command->Bus, THE_Command->Device, THE_Command->Function, THE_Command->Address);
			array11[0]= u32return_data6 & 0x000000FF;
			array11[1]=(u32return_data6 & 0x0000FF00) >>8;
			array11[2]=(u32return_data6 & 0x00FF0000) >>16;
			array11[3]=(u32return_data6 & 0xFF000000) >>24;
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Dump Device --------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Dump_Device)
		{
		Found_Size = PCI_Device_Found_and_Size(THE_Command->Bus, THE_Command->Device, THE_Command->Function);
/*
		SHFprint(THE_Command->Bus,      2, 0x10,"\nFound Size for ", ":");
		SHFprint(THE_Command->Device,  2, 0x10,"", ".");
		SHFprint(THE_Command->Function,2, 0x10,"", " = ");
		SHFprint(Found_Size, 4, 0x10,"0x", "\n");
*/
		if (Found_Size == 0x00) // Not Found
			{
			printf("============================================================\n");
			SHFprint(THE_Command->Bus,      2, 0x10,"Device at ", ":");
			SHFprint(THE_Command->Device,   2, 0x10,"", ".");
			SHFprint(THE_Command->Function, 1, 0x10,"", " Not Found!\n");
			printf("============================================================\n\n");
			}
		else
			{
			for (cx=0x00; cx<=(Found_Size-1); cx++)
			   array11[cx] = SHFpci_read_byte(THE_Command->Bus, THE_Command->Device, THE_Command->Function, (THE_Command->Address+cx));
			THE_Command->Length = Found_Size;
			THE_Command->Size = 1;
			THE_Command->Address = 0;
			Pretty_Output(THE_Command, result9, temp, array11, THE_Command->passed_frequency);
			}
		}

// ----- PCI Dump File ----------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------
	if (THE_Command->Command_Final == PCI_Dump_File)
		{
		// sudo lspci -xxxx > regdump.txt
		if (THE_Command->nosudox == true)
			cx = snprintf(tempstr, 100, "lspci -xxxx >  %s", copyargv[THE_Command->Filename_int]);
		else
			cx = snprintf(tempstr, 100, "sudo lspci -xxxx >  %s", copyargv[THE_Command->Filename_int]);

		strcat(tempstr, "\0");
		cx = cx;
//		printf("Final String: %s\n",tempstr);

		system(tempstr);		

		printf("============================================================\n");
		printf("Full PCI Register Dump saved into file %s\n", copyargv[THE_Command->Filename_int]);
		printf("============================================================\n\n");
		}


	free(array11);

	}	// End of Execute_Command()


//===========================================================
//===========================================================
void Not_Done_Yet(struct command *THE_Command, int copyargc, char copyargv[20][255])
	{
	int q;

   printf("\n===================================================================================================\n"); 
   printf("This routine not implemented Yet.  Sorries!  Contact Sam Fleming for latest Version (x6-6129)      \n"); 
	SHFprint(copyargc,1,10, "You passed ", " Parameters:\n");
	for (q=0; q < copyargc; q++)
		{
		printf("%s ", copyargv[q]);
		} 
	printf("\nHL Command_Type : %i    { (0)hl_command_none, (1)mem, (2)io, (3)msr, (4)pci, (5)help }\n", THE_Command->Command_Type);
	printf("LL Command_Type : %i    { Table that follows: }\n", THE_Command->Command_Final);

	printf(" (0)ll_command_none,                                                                          // 0  \n");
	printf(" (1)Memory_Read_Byte,    (2)Memory_Read_Word,    (3)Memory_Read_Dword,   (4)Memory_Read_XMM,  // 1-4\n");     
	printf(" (5)Memory_Write_Byte,   (6)Memory_Write_Word,   (7)Memory_Write_Dword,  (8)Memory_Write_XMM, // 5-8\n");     
	printf(" (9)Memory_Detailed_Help,                                                                     // 9   \n");     
	printf(" (10)IO_Read_Byte,       (11)IO_Read_Word,       (12)IO_Read_Dword,                           // 10-12\n");     
	printf(" (13)IO_Write_Byte,      (14)IO_Write_Word,      (15)IO_Write_Dword,                          // 13-15\n");     
	printf(" (16)IO_Detailed_Help,                                                                        // 16\n");     
	printf(" (17)MSR_Read,           (18)MSR_Write,          (19)MSR_Detailed_Help,                       // 17-19\n");     
	printf(" (20)PCI_Read_Byte,      (21)PCI_Read_Word,      (22)PCI_Read_Dword,                          // 20-22\n");     
	printf(" (23)PCI_Write_Byte,     (24)PCI_Write_Word,     (25)PCI_Write_Dword,                         // 23-25\n");     
	printf(" (26)PCI_Dump_Device,    (27)PCI_Dump_File,      (28)PCI_Detailed_Help,                       // 26-28\n");     
	printf(" (29)Generic_Help                                                                             // 29\n");     
	printf("\n");

	printf("helpx           : %X\n", (unsigned int)THE_Command->helpx);
	printf("errorx          : %X\n", (unsigned int)THE_Command->errorx);
	printf("\n");
	printf("Bus             : %X\n",   (unsigned int)THE_Command->Bus);
	printf("Bus_Valid       : %X\n",   (unsigned int)THE_Command->Bus_Valid);
	printf("Device          : %X\n",   (unsigned int)THE_Command->Device);
	printf("Device_Valid    : %X\n",   (unsigned int)THE_Command->Device_Valid);
	printf("Function        : %X\n",   (unsigned int)THE_Command->Function);
	printf("Function_Valid  : %X\n",   (unsigned int)THE_Command->Function_Valid);
	printf("Address         : %X\n",   (unsigned int)THE_Command->Address);
	printf("Address_Valid   : %X\n",   (unsigned int)THE_Command->Address_Valid);
	printf("\n");
	printf("Access_Type     : %i    { (0)access_none,  (1)Read, (2)Write }\n", THE_Command->Access_Type);
	printf("Data            : %X\n",   (unsigned int)THE_Command->Data);
	printf("Data_Valid      : %X\n",   (unsigned int)THE_Command->Data_Valid);
	printf("Size            : %i    { (0)size_none, (1)Byte, (2)Word, (3)Dword, (4)XBlock } \n", THE_Command->Size);
	printf("Length          : %X\n",   (unsigned int)THE_Command->Length);
	printf("Filename        : %s\n", THE_Command->Filename);
	}

//===========================================================
//===========================================================
void Pretty_Output(   struct command *THE_Command, float result9, char *temp, u8 array11[], float frequency)
	{
	unsigned long Start_Address;	
	unsigned long End_Address;	
   unsigned long int j=0;
   unsigned long length;
   bool dots_printed=false;
	unsigned long int i;
	unsigned int numb_bytes;

	printf("============================================================\n");
	if (THE_Command->Command_Type == mem)
		printf("Mem Address");
	else if (THE_Command->Command_Type == io)
		printf("IO  Address");
	else if (THE_Command->Command_Type == pci)
		printf("PCI Address");

	if ( (THE_Command->Access_Type == Read) || (THE_Command->Command_Final == PCI_Dump_Device) )
		printf("(Read) : ");
	else if (THE_Command->Access_Type == Write)
		printf("(Write): ");


	if (THE_Command->Command_Type == pci)
		{
		SHFprint(THE_Command->Bus,      2, 0x10,"", ":");
		SHFprint(THE_Command->Device,   2, 0x10,"", ".");
		SHFprint(THE_Command->Function, 1, 0x10,"", "-");
	if (THE_Command->Command_Final == PCI_Dump_Device)
		SHFprint(THE_Command->Address,  3, 0x10,"", "h   (Device) ");
	else
		SHFprint(THE_Command->Address,  3, 0x10,"", "h   Size: ");
		}
	else
		SHFprint(THE_Command->Address, 8, 0x10,"0x","   Size: ");


	if ( (THE_Command->Size == Byte) && ((THE_Command->Command_Final != PCI_Dump_Device)) )
		printf("BYTE  ");
	else if (THE_Command->Size == Word)
		printf("WORD  ");
	else if (THE_Command->Size == Dword)
		printf("DWORD ");
	else if (THE_Command->Size == XBlock)
		printf("XBLOCK");


	if (THE_Command->Size == XBlock)
		{
		SHFprint(THE_Command->Length, 4, 0x10,"  Length: 0x"," (4K blocks)=");
		SHFprint((THE_Command->Length*0x1000), 4, 0x10,"0x"," bytes\n");
		}

	else if (THE_Command->Command_Final == PCI_Dump_Device)
		printf("\n");
	else if ( THE_Command->Command_Type == pci) 
		printf("\n");
	else 
		SHFprint(THE_Command->Length, 4, 0x10,"  Length: 0x","\n");


	if (THE_Command->Size == Byte)
		i = 2;
	if (THE_Command->Size == Word)
		i = 4;
	if ( (THE_Command->Size == Dword) || (THE_Command->Size == XBlock) )
		i = 8;
	if (THE_Command->Access_Type == Write)
		{
		SHFprint(THE_Command->Data, i, 0x10,"\nData Write = 0x","\n");
		printf("Data Read Back:\n");
		}

	// Now things get interesting
	if (THE_Command->Size == XBlock)
		{
		printf("             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
		printf("             -----------------------------------------------\n");
		Start_Address = THE_Command->Address & 0xFFFFFFF0;
		End_Address = (THE_Command->Address + (THE_Command->Length*0x1000)-1) | 0x0000000F;
		for (i=Start_Address; i<Start_Address+0x40; i++)
			{
			if ( (i & 0x0000000F) == 0)
				SHFprint(i, 8, 0x10,"0x",":  ");
			SHFprint(array11[i-THE_Command->Address], 2, 0x10,""," ");
			if ( (i & 0x0000000F) == 0xF)
				printf("\n");
			}
		printf("...\n");

		for (i=End_Address-0x40+1; i<=End_Address; i++)
			{
			if ( (i & 0x0000000F) == 0)
				SHFprint(i, 8, 0x10,"0x",":  ");
			SHFprint(array11[i-THE_Command->Address], 2, 0x10,""," ");
			if ( (i & 0x0000000F) == 0xF)
				printf("\n");
			}
		}
	else		// Everything NOT block mode!
		{
		if (THE_Command->Command_Type == pci)
			{
			printf("        0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
			printf("        -----------------------------------------------\n");
			}
		else
			{
			printf("             0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F\n");
			printf("             -----------------------------------------------\n");
			}

		Start_Address = THE_Command->Address & 0xFFFFFFF0;
		End_Address = (THE_Command->Address + THE_Command->Length-1) | 0x0000000F;
      length = End_Address - Start_Address;

		for (i=Start_Address; i<=End_Address; i++)
			{
			// Don't want screen scrolling forever for long lengths;
			//  Will print first 0x100 bytes and last 0x100 bytes.
         if ( (length > 0x200) && ( (j > 0xFF) && (j < (length-0xFF)) ) )
            {
            if (dots_printed == false)
               {
               printf("...\n");
               dots_printed = true;
               }
            }
         else
            {
			   if ( (i & 0x0000000F) == 0)
				   {
				   if (THE_Command->Command_Type == pci)
					   SHFprint(i, 3, 0x10,"0x",":  ");
				   else
					   SHFprint(i, 8, 0x10,"0x",":  ");
				   }
			   if ( (i < THE_Command->Address) || (i > (THE_Command->Address + THE_Command->Length-1) ) )
				   printf("xx ");
			   else
				   SHFprint(array11[i-THE_Command->Address], 2, 0x10,""," ");
			   if ( (i & 0x0000000F) == 0xF)
				   printf("\n");
			   }
         j = j+1;
         }
		} // not xmm mode!

	if (THE_Command->Display_Time)
		{
		printf("\nFrequency:          %2.5fGHz", frequency);		
		printf("       Time: %.4f", result9);
		printf(" %s\n", temp);

		// Let's calculate BW
		if (THE_Command->Size == XBlock)
			numb_bytes = THE_Command->Length * 0x1000;
		else
			numb_bytes = THE_Command->Length;
		SHFprint(numb_bytes, 6, 0x10,"Bytes Transferred:  0x","");
		printf("         Bandwidth: %f", (double)(numb_bytes/result9));
		printf(" MB/sec\n");
		}

	if (THE_Command->Command_Type == io)
		{
		printf("\nIO Return Data: 0x");
		i=THE_Command->Length-1;
		while (i!=0)
			{
			SHFprint(array11[i], 2, 0x10,"","");
			i=i-1;
			}
		SHFprint(array11[0], 2, 0x10,"","\n");
		}

	printf("============================================================\n\n");
	}

/*
VERSION:
========
Version 1.1  (01-27-2015):
	- Various Bug Fixes.
	- Test Plan provided.
Version 1.x  (xx-xx-2015):
	- The embedded assembly TSC routines were only returning 4 bytes.  2GB transfer times were reported incorrectly!  Fixed.
	- Long memory dumps (b, d, w) were taking too long.  Now only displaying first 0x100 and last 0x100 bytes.
	- Transfer size bumped up to 512MB (biggest MMIO card I have available for testing).
Version 1.2  (01-30-2015):  Release to Bluecoat.
Version 1.3  (02-03-2015):
	- First official release on GitHub.
	- Added a very short readme
Version 1.4  (02-09-15)
	- Cleaned up Help Menus
	- Tested with Following Builds:
	(see samtool_testing.txt for more details)
		Ubuntu 14.04 (64-bit)
		Mint 17 (64 bit)
		Fedora 21 (64-bit)
		SUSE 13.2 (64-bit)
	

TO DO:
=====
*)  Need to add ability to pass THIRTY-TWO BYTES 0x112233445566778899AABBCCDDEEFF00 (and this is only 16!) for xmm instruction
*)  It would be VERY NICE to "assembl-ize" the RDMSR and PCI routines (requiring access to PCIEXBAR).  Would remove library dependancies
*)  I would make ALL the integers uul (length, address).
*)  Making 64 bit addresses will be important, but hard!
*)  PCIe 2.0 and 3.0 testing  (MB/Sec)
*)  CPUID (I would just try to dump the whole kit-n-kaboodle (to file).  For the love of all that is righteous - DECODE THE ASCII STRINGS!
*)  Read-Modify-Write ability?  Really handy for PCI registers.  Will necessitate adding "x" for bits to be read but not modified.

DEBUG:
======


*/




