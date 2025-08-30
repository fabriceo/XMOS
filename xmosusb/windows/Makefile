#windows version
#use mingw32-make or gmake
#
#build all
all:	xmosusb.exe testlibusb.exe Makefile
	@echo xmosusb and testlibusb

#build xmosusb and testlibusb only	

#libusb 1.0.26 is mandatory to operate properly with winusb driver on windows
# this is the path where it was installed for mingw64
# static library is used so no dll file required
LIBUSB = -Imingw64/include/libusb-1.0 mingw64/lib/libusb-1.0.a 

xmosusb.exe:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling $@
	g++ -o xmosusb ../src/xmosusb.cpp -I../src $(LIBUSB)
	@echo xmosusb.exe compiled sucessfully

xmostusb.exe:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling xmostusb.exe
	g++ -o xmostusb ../src/xmosusb.cpp -I../src -I../../../TUSBAUDIO -DTUSBAUDIOAPI -DTB_DEBUG=0
	@echo xmostusb.exe compiled sucessfully

testlibusb.exe:	../src/testlibusb.c
	@echo compiling testlibusb.exe
	gcc -o testlibusb ../src/testlibusb.c -I../src $(LIBUSB)
	@echo testlibusb.exe compiled sucessfully
	