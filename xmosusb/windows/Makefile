#windows version
#use mingw32-make or gmake
	
all:	xmosusb.exe testlibusb.exe Makefile dac8
	@echo xmosusb and testlibusb and dac8 uptodate
	
dac8:	xmosusb.exe Makefile dac8pro.exe dac8stereo.exe dac8prodspeval.exe
	@echo all dac8*.exe utilities done

#libusb 1.0.26 is mandatory to operate properly with winusb driver on windows
LIBUSB = -Imingw64/include/libusb-1.0 mingw64/lib/libusb-1.0.a 

xmosusb.exe:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling xmosusb.exe
	g++ -o xmosusb ../src/xmosusb.cpp -I../src $(LIBUSB)
	@echo xmosusb.exe compiled sucessfully

testlibusb.exe:	../src/testlibusb.c
	@echo compiling testlibusb.exe
	gcc -o testlibusb ../src/testlibusb.c -I../src $(LIBUSB)
	@echo testlibusb.exe compiled sucessfully
	
../dac8/dac8pro.bin.h:	../dac8/dac8pro.bin
	xmosusb --bin2hex ..\dac8\dac8pro.bin
	@echo dac8pro.bin.h generated sucessfully
		
../dac8/dac8stereo.bin.h:	../dac8/dac8stereo.bin
	xmosusb --bin2hex ..\dac8\dac8stereo.bin
	@echo dac8stereo.bin.h generated sucessfully

../dac8/dac8prodspeval.bin.h:	../dac8/dac8prodspeval.bin
	xmosusb --bin2hex ..\dac8\dac8prodspeval.bin
	@echo dac8prodspeval.bin.h generated sucessfully

dac8pro.exe: ../dac8/dac8pro.bin.h xmosusb.exe
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src $(LIBUSB) -DDAC8PRO -DDSP_CMD=0
	@echo dac8pro.exe compiled sucessfully
	@copy /Y dac8pro.exe    ..\dac8\utilities\windows

dac8stereo.exe:	../dac8/dac8stereo.bin.h xmosusb.exe
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src $(LIBUSB) -DDAC8STEREO -DDSP_CMD=0
	@echo dac8stereo.exe compiled sucessfully
	@copy /Y dac8stereo.exe ..\dac8\utilities\windows

dac8prodspeval.exe:	../dac8/dac8prodspeval.bin.h xmosusb.exe
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src $(LIBUSB) -DDAC8PRODSPEVAL
	@echo dac8prodspeval.exe compiled sucessfully
	@copy /Y dac8prodspeval.exe ..\dac8\utilities\windows

	