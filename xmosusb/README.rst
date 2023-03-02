Source code for the XMOSUSB utility
licence creative comon share alike 4.0
https://github.com/fabriceo/XMOS
in the folder xmosusb

inspired from original code provided by XMOS.

folder src contains the source code of xmosusb.cpp and some .h files which are included at compilation

folders osx, windows, linux, rpi contain the Makefile and executable for respective OS.

to compile the utility, go in the osx or windows or linux or rpi folder and type "make".

for windows this requires GCC to be installed e.g. by installing mingw-w64 available at :
https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/
select the following options in the installation wizard : 

	architecture 	x86_64
	
	Threads 		win32
	
then a "run-terminal" option will apear in the window start menu for the option mingw.
this will launch a cmd.exe with proper "path" for accessing GCC and Make

for linux and for raspberrypi ( tested with volumio ), 
make sure you have gcc with following commands:

	sudo apt-get update 
	sudo apt-get install build-essential
	
and libusb devlopment (this provide all the right package information for the compiler)

	sudo apt-get install libusb-1.0-0-dev

for OSX this requires the XCODE comand line utility to be installed

using xmosusb:
available commands depends on which file is include during compilation.

by default all 4 files are included:

	xmosusb_samd.h
	xmosusb_dsp.h
	xmosusb_dac8.h
	xmosusb_bin2hex.h

to remove them, just include a compiler option like this in the Makefile:

	-DSAMD_CMD=0
	-DDSP_CMD=0
	-DDAC8_CMD=0
	-DBIN2HEX_CMD=0

to print the list of supported commands type:
	xmosusb -?
	
typing xmosusb without option will scrutenize all usb device and print the list found.

to apply commands on a specific device, add its number as the first parameter of the command line,
or the serial number, as a text string as it apears when the list of devices are shown.
example:
	xmosusb --testvidpid
or
	xmosusb 0 --testvidpid
or
	xmosusb 000022 -- testvidpid
	
* to get a status from dac8
	xmosusb --dacstatus
	
* to program a dac8 xmos firmware wit a binary file generate bys xmos xflash tool:
	xmosusb --xmosload firmware.bin

*** using DSP capabilities (if available and reported by dacstatus command) ***
remark : creating a dsp code is donce with the command dspcreate and associated command line values. see below
* to upload a dsp code in the xmos ram memory
	xmosusb --dspload file
* to write a dsp code from xmos ram to flash memory in the slot X (1 to 8).
	xmosusb --dspwrite X
* to read a dsp code from flash memory slot X to xmos ram
	xmosusb --dspread X
* to read some dsp code in xmos ram by page of 16 words from adresse X (starting at 0)
	xmosusb --dspreadmem X
