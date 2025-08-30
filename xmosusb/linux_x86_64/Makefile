# requires sudo apt-get install libusb-1.0-0-dev

all:	xmosusb testlibusb Makefile
	@echo xmosusb and testlibusb

LIBUSB = `pkg-config --libs --cflags libusb-1.0`

xmosusb:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling $@
	g++ -o $@ ../src/xmosusb.cpp -I../src $(LIBUSB)
	@echo $@ compiled sucessfully
	
testlibusb:	../src/testlibusb.c Makefile
	@echo compiling $@
	gcc  -o testlibusb ../src/testlibusb.c $(LIBUSB)
	@echo $@ compiled sucessfully
	
	