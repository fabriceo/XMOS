# requires sudo apt-get install libusb-1.0-0-dev
all:
	g++ -g -o xmosusb ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. `pkg-config --libs --cflags libusb-1.0`
