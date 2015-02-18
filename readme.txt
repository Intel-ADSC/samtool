SAMTOOL is a simple command-line program used to read/write to hardware 
locations in an IA based system:
	- MMIO (memory-mapped IO)			- PCI
	- IO										- MSR
The tool does not require a driver.  

Static Executable included in download.  The tool can usually be run after the following command is executed:
	chmod 755 samtool


Alternatively, the program can be compiled.  Two libraries must be installed
prior to compile.

Libraries:
	sudo apt-get install libpci-dev
	sudo apt-get install msr-tools

Required Files:
	samkit.c
	samkit.h
	samtool.c
	code_block_read.h
	code_block_write.h

Compile:
	gcc -Wall -W -Werror -g samtool.c samkit.c -lpci -lm -o samtool


Run (help Example):
	sudo ./samtool ?