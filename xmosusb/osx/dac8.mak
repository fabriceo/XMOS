
all:
	g++ -o xmosusb    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64
	./xmosusb --bin2hex ../dac8/DAC8STEREO.bin
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8STEREO
	./xmosusb --bin2hex ../dac8/DAC8PRO.bin
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO
	./xmosusb --bin2hex ../dac8/DAC8PRODSP.bin
	g++ -o dac8prodsp ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRODSP
	