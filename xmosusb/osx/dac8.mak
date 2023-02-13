
all:
	g++ -o xmosusb    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64
#	./xmosusb --bin2hex ../dac8/DAC8STEREO.bin
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8STEREO -DDSP_CMD=0 -DBIN2HEX_CMD=0
#	./xmosusb --bin2hex ../dac8/DAC8PRO.bin
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO -DDSP_CMD=0 -DBIN2HEX_CMD=0
#	./xmosusb --bin2hex ../dac8/DAC8PRO32.bin
	g++ -o dac8pro32    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO32 -DDSP_CMD=0 -DBIN2HEX_CMD=0
#	./xmosusb --bin2hex ../dac8/DAC8PRODSPEVAL.bin
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRODSPEVAL -DBIN2HEX_CMD=0
#	./xmosusb --bin2hex ../dac8/DACFABRICE.bin
	g++ -o dacfabrice ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDACFABRICE -DBIN2HEX_CMD=0
	@cp dac8stereo ../dac8/utilities/osx
	@cp dac8pro    ../dac8/utilities/osx
	@cp dac8pro32  ../dac8/utilities/osx
	@cp dac8prodspeval ../dac8/utilities/osx
	@cp dacfabrice ../dac8/utilities/osx

	