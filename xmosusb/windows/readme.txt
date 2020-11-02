*** MinGW64 must be installed ***

download and launch the basic online installer :

https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/

select these options in the wizard:

	architecture 	x86_64
	
	Threads 		win32

once installed, enter in a terminal session from the Windows Start menu "MinGW-W64 projet / Run terminal"
a command prompt session will apear with proper "path" envirronement initialized.
type "cd xx" to move in this folder.
type "mingw32-make" to launch the compiler.
the xmosusb.exe file will be created in this same folder


