Source code for the XMOSUSB utility
available on github, licence creative comon share alike 4.0:
https://github.com/fabriceo/AVDSP
in the folder module_xmosusb

based on original code provided by XMOS (c)

folder src contains the source code of xmosusb.cpp and some .h files which are included at compilation

folders osx, windows and linux contains the Makefile and necessary libusb files.
to compile the utility, go in the osx or windows or linux folder and type "make".

for windows this requires GCC to be installed e.g. by installing mingw-w64 available at :
https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/
select the following options in the installation wizard : 
	architecture 	x86_64
	Threads 		win32
then a run-terminal option will apear in the window start menu for the option mingw.
this will launch a cmd.exe with proper "path" for accessing GCC and Make

for linux 
for raspberrypi / volumio, make sure you have gcc with following commands:
	sudo apt-get update 
	sudo apt-get install build-essential

for OSX this require the XCODE comand line utility

using xmosusb:
available commands depends on which file is include during compilation.

by default all 4 files are include:
	xmosusb_samd.h
	xmosusb_dsp.h
	xmosusb_dac.h
	xmosusb_bin2hex.h

to remove them, just include a compiler option like this in the Makefile:
	-DSAMD_CMD=0
	-DDSP_CMD=0
	-DDAC_CMD=0
	-DBIN2HEX_CMD=0

to print the list of supported commands type:
	xmosusb ?
	
typing xmosusb without option will scrutenize all usb device and print the list found.
a special display is made when a XMOS supported device is identified.

