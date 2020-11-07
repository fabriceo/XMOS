# requires sudo apt-get install libusb-1.0-0-dev
all:
	g++ -g -o xmosusb ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. `pkg-config --libs --cflags libusb-1.0`
	./xmosusb --bin2hex ../dac8/DAC8STEREO.bin
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -DDAC8STEREO -DDSP_CMD=0 -DBIN2HEX_CMD=0 `pkg-config --libs --cflags libusb-1.0`
	./xmosusb --bin2hex ../dac8/DAC8PRO.bin
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -DDAC8PRO -DDSP_CMD=0 -DBIN2HEX_CMD=0 `pkg-config --libs --cflags libusb-1.0`
	./xmosusb --bin2hex ../dac8/DAC8PRODSPEVAL.bin
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -DDAC8PRODSPEVAL -DBIN2HEX_CMD=0 `pkg-config --libs --cflags libusb-1.0`
	
	
