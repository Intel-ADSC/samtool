SAMTOOL is a simple command-line program used to read/write to these 
locations in an IA based system:
	- MMIO (memory-mapped IO)			- PCI
	- IO										- MSR

The tool does not require a driver.  Two libraries must be installed
prior to compile.

Required Files:
	samkit.c
	samkit.h
	samtool.c

Libaries:
	sudo apt-get install libpci-dev
	sudo apt-get install msr-tools

Compile:
	gcc -Wall -W -Werror -g samtool.c samkit.c -lpci -lm -o samtool

To Run:
	sudo ./samtool	?