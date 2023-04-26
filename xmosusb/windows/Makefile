#windows version
#use mingw32-make or gmake
#all:
#	gcc -o xmosusb ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -lusb-1.0
	
all:	xmosusb.exe Makefile
	@echo xmosusb uptodate, to generate all dac8 upgrade utilities type : gmake dac8
	
dac8:	xmosusb.exe Makefile dac8pro.exe dac8pro32.exe dac8stereo.exe dac8prodspeval.exe dacfabrice.exe
	@echo all dac8*.exe utilities done

xmosusb.exe:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../dac8/xmosusb_dac8.h
	@echo compiling xmosusb.exe
	g++ -o xmosusb ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64
	@echo xmosusb.exe compiled sucessfully
	
../dac8/dac8pro.bin.h:	../dac8/dac8pro.bin xmosusb.exe
	./xmosusb --bin2hex ../dac8/dac8pro.bin
	@echo dac8pro.bin.h generated sucessfully
	
../dac8/dac8pro32.bin.h:	../dac8/dac8pro32.bin xmosusb.exe
	./xmosusb --bin2hex ../dac8/dac8pro32.bin
	@echo dac8pro32.bin.h generated sucessfully
	
../dac8/dac8stereo.bin.h:	../dac8/dac8stereo.bin xmosusb.exe
	./xmosusb --bin2hex ../dac8/dac8stereo.bin
	@echo dac8stereo.bin.h generated sucessfully

../dac8/dac8prodspeval.bin.h:	../dac8/dac8prodspeval.bin xmosusb.exe
	./xmosusb --bin2hex ../dac8/dac8prodspeval.bin
	@echo dac8prodspeval.bin.h generated sucessfully
	
../dac8/dacfabrice.bin.h:	../dac8/dacfabrice.bin xmosusb.exe
	./xmosusb --bin2hex ../dac8/dacfabrice.bin
	@echo dacfabrice.bin.h generated sucessfully

dac8pro.exe: ../dac8/dac8pro.bin.h xmosusb.exe
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO -DDSP_CMD=0 -DBIN2HEX_CMD=0
	@echo dac8pro.exe compiled sucessfully
	@copy dac8pro.exe    ../dac8/utilities/windows

dac8pro32.exe: ../dac8/dac8pro32.bin.h xmosusb.exe
	g++ -o dac8pro32    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO32 -DDSP_CMD=0 -DBIN2HEX_CMD=0
	@echo dac8pro32.exe compiled sucessfully
	@copy dac8pro32.exe  ../dac8/utilities/windows

dac8stereo.exe:	../dac8/dac8stereo.bin.h xmosusb.exe
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8STEREO -DDSP_CMD=0 -DBIN2HEX_CMD=0
	@echo dac8stereo.exe compiled sucessfully
	@copy dac8stereo.exe ../dac8/utilities/windows

dac8prodspeval.exe:	../dac8/dac8prodspeval.bin.h xmosusb.exe
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRODSPEVAL -DBIN2HEX_CMD=0
	@echo dac8prodspeval.exe compiled sucessfully
	@copy dac8prodspeval.exe ../dac8/utilities/windows

dacfabrice.exe:	../dac8/dacfabrice.bin.h xmosusb.exe
	g++ -o dacfabrice ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDACFABRICE -DBIN2HEX_CMD=0
	@echo dacfabrice.exe compiled sucessfully
	@copy dacfabrice.exe ../dac8/utilities/windows


	