
all:
	gcc -o xmosusb    ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -lusb-1.0
	xmosusb --bin2hex ../dac8/DAC8STEREO.bin
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -lusb-1.0 -DDAC8STEREO -DDSP_CMD=0 -DBIN2HEX_CMD=0
	xmosusb --bin2hex ../dac8/DAC8PRO.bin
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -lusb-1.0 -DDAC8PRO -DDSP_CMD=0 -DBIN2HEX_CMD=0
	xmosusb --bin2hex ../dac8/DAC8PRO32.bin
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -lusb-1.0 -DDAC8PRO32 -DDSP_CMD=0 -DBIN2HEX_CMD=0
	xmosusb --bin2hex ../dac8/DAC8PRODSPEVAL.bin
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src -I. -I../dac8 -L. -lusb-1.0 -DDAC8PRODSPEVAL -DBIN2HEX_CMD=0
	copy dac8stereo.exe ..\dac8\utilities\windows
	copy dac8pro.exe    ..\dac8\utilities\windows
	copy dac8pro32.exe  ..\dac8\utilities\windows
	