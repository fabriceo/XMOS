all:	testlibusb xmosusb Makefile
	@echo done, to generate all dac8 upgrade utilities type : make dac8
	
dac8:	xmosusb Makefile dac8pro dac8stereo dac8prodspeval dacfabrice
	@echo dac8 done

#libusb is installed with brew and the statically linked. no need for dylib file or external dependency.
LIBUSB = -I/usr/local/Cellar/libusb/1.0.26/include/libusb-1.0 /usr/local/Cellar/libusb/1.0.26/lib/libusb-1.0.a -Wl,-framework,IOKit -Wl,-framework,CoreFoundation -Wl,-framework,Security

xmosusb:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling xmosusb
	g++ -o xmosusb ../src/xmosusb.cpp -I../src $(LIBUSB)
	@echo xmosusb compiled sucessfully
	
testlibusb:	../src/testlibusb.c
	@echo compiling testlibusb
	gcc  -o testlibusb ../src/testlibusb.c $(LIBUSB)
	@echo testlibusb compiled sucessfully
	
../dac8/dac8pro.bin.h:	../dac8/dac8pro.bin 
	./xmosusb --bin2hex ../dac8/dac8pro.bin
	@echo dac8pro.bin.h generated sucessfully
	
../dac8/dac8stereo.bin.h:	../dac8/dac8stereo.bin 
	./xmosusb --bin2hex ../dac8/dac8stereo.bin
	@echo dac8stereo.bin.h generated sucessfully

../dac8/dac8prodspeval.bin.h:	../dac8/dac8prodspeval.bin 
	./xmosusb --bin2hex ../dac8/dac8prodspeval.bin
	@echo dac8prodspeval.bin.h generated sucessfully

../dac8/dacfabrice.bin.h:	../dac8/dacfabrice.bin 
	./xmosusb --bin2hex ../dac8/dacfabrice.bin
	@echo dacfabrice.bin.h generated sucessfully

dac8pro: ../dac8/dac8pro.bin.h xmosusb
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src $(LIBUSB) -DDAC8PRO -DDSP_CMD=0
	@echo dac8pro compiled sucessfully
	@cp dac8pro    ../dac8/utilities/osx

dac8stereo:	../dac8/dac8stereo.bin.h xmosusb
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src $(LIBUSB) -DDAC8STEREO -DDSP_CMD=0
	@echo dac8stereo compiled sucessfully
	@cp dac8stereo ../dac8/utilities/osx

dac8prodspeval:	../dac8/dac8prodspeval.bin.h xmosusb
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src $(LIBUSB) -DDAC8PRODSPEVAL
	@echo dac8prodspeval compiled sucessfully
	@cp dac8prodspeval ../dac8/utilities/osx

dacfabrice:	../dac8/dacfabrice.bin.h xmosusb
	g++ -o dacfabrice ../src/xmosusb.cpp -I../src $(LIBUSB) -DDACFABRICE
	@echo dacfabrice compiled sucessfully




