all:	testlibusb xmosusb Makefile
	@echo done, to generate all dac8 upgrade utilities type : make dac8
	
xmos:	xmosusb Makefile
	@echo xmosusb done

dac8:	xmosusb Makefile dac8pro dac8stereo
	@echo dac8 done

#libusb is installed with brew and statically linked. no need for dylib file. update this folder according to your configuration
LIBUSBFOLDER = /usr/local/Cellar/libusb/1.0.27
LIBUSB = -I$(LIBUSBFOLDER)/include/libusb-1.0 $(LIBUSBFOLDER)/lib/libusb-1.0.a
FLAGS  = -Wl,-framework,IOKit -Wl,-framework,CoreFoundation -Wl,-framework,Security -arch x86_64
xmosusb:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling xmosusb
	g++ -o xmosusb ../src/xmosusb.cpp -I../src $(LIBUSB) $(FLAGS)
	cp xmosusb /usr/local/bin/xmosusb1026
	@echo xmosusb compiled sucessfully and copied in /usr/local/bin
	
testlibusb:	../src/testlibusb.c
	@echo compiling testlibusb
	gcc  -o testlibusb ../src/testlibusb.c $(LIBUSB) $(FLAGS) 
	@echo testlibusb compiled sucessfully
	
../dac8/dac8pro.bin.h:	../dac8/dac8pro.bin 
	./xmosusb --bin2hex ../dac8/dac8pro.bin
	@echo dac8pro.bin.h generated sucessfully
	
../dac8/dac8stereo.bin.h:	../dac8/dac8stereo.bin 
	./xmosusb --bin2hex ../dac8/dac8stereo.bin
	@echo dac8stereo.bin.h generated sucessfully

dac8pro: ../dac8/dac8pro.bin.h xmosusb
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src $(LIBUSB) $(FLAGS) -DDAC8PRO
	@echo dac8pro compiled sucessfully
	@cp dac8pro    ../dac8/utilities/osx

dac8stereo:	../dac8/dac8stereo.bin.h xmosusb
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src $(LIBUSB) $(FLAGS) -DDAC8STEREO 
	@echo dac8stereo compiled sucessfully
	@cp dac8stereo ../dac8/utilities/osx




