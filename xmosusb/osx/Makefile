all:	testlibusb xmosusb Makefile
	@echo done, type password to copy xmosusb in /usr/local/bin
	sudo cp xmosusb /usr/local/bin
	which xmosusb
	
#libusb is installed with homebrew arm in the new opt/homebrew folder and is statically linked. no need for dylib file.
LIBUSBFOLDERARM = /opt/homebrew/Cellar/libusb/1.0.29
#libusb is installed with homebrew x86_64 in the former folder /usr/local and is statically linked.
LIBUSBFOLDERX86 = /usr/local/Cellar/libusb/1.0.29
LIBUSBARM = -I$(LIBUSBFOLDERARM)/include/libusb-1.0 $(LIBUSBFOLDERARM)/lib/libusb-1.0.a 
LIBUSBX86 = -I$(LIBUSBFOLDERX86)/include/libusb-1.0 $(LIBUSBFOLDERX86)/lib/libusb-1.0.a 
FLAGS  = -lobjc -Wl,-framework,IOKit -Wl,-framework,CoreFoundation -Wl,-framework,Security

OSXX86 = -target x86_64-apple-macos10.13 # tested successfully on imac 2010 intel with high sierra
OSXARM = -target arm64-apple-macos15 # so far all ARM silicon product support sequoia macos15


xmosusb_arm:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling $@
	@g++ -o $@ ../src/xmosusb.cpp -I../src $(LIBUSBARM) $(FLAGS) $(OSXARM)
	@echo $@ compiled sucessfully
	
xmosusb_x86:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../src/xmosusb_dac8.h
	@echo compiling $@
	@g++ -o $@ ../src/xmosusb.cpp -I../src $(LIBUSBX86) $(FLAGS) $(OSXX86)
	@echo $@ compiled sucessfully
	
xmosusb:	xmosusb_arm xmosusb_x86
	@lipo -create -output $@ xmosusb_x86 xmosusb_arm
	@echo dual architecture $@ combined sucessfully


testlibusb_arm:	../src/testlibusb.c
	@echo compiling $@
	@gcc  -o $@ ../src/testlibusb.c $(LIBUSBARM) $(FLAGS) $(OSXARM)
	@echo $@ compiled sucessfully

testlibusb_x86:	../src/testlibusb.c
	@echo compiling $@
	@gcc  -o $@ ../src/testlibusb.c $(LIBUSBX86) $(FLAGS) $(OSXX86)
	@echo $@ compiled sucessfully

testlibusb:	testlibusb_arm testlibusb_x86
	@lipo -create -output $@ testlibusb_x86 testlibusb_arm
	@echo dual architecture $@ combined sucessfully



