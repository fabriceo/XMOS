all:	xmosusb
	@echo xmosusb uptodate, to generate all dac8 upgrade utilities type : make dac8
	
dac8:	xmosusb dac8pro dac8pro32 dac8stereo dac8prodspeval dacfabrice
	@echo dac8 done

xmosusb:	../src/xmosusb.cpp ../src/xmosusb_samd.h ../src/xmosusb_dsp.h ../src/xmosusb_bin2hex.h ../dac8/xmosusb_dac8.h
	g++ -o xmosusb ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64
	@echo xmosusb compiled sucessfully
	
../dac8/dac8pro.bin.h:	../dac8/dac8pro.bin xmosusb
	./xmosusb --bin2hex ../dac8/dac8pro.bin
	@echo dac8pro.bin.h generated sucessfully
	
../dac8/dac8pro32.bin.h:	../dac8/dac8pro32.bin xmosusb
	./xmosusb --bin2hex ../dac8/dac8pro32.bin
	@echo dac8pro32.bin.h generated sucessfully
	
../dac8/dac8stereo.bin.h:	../dac8/dac8stereo.bin xmosusb
	./xmosusb --bin2hex ../dac8/dac8stereo.bin
	@echo dac8stereo.bin.h generated sucessfully

../dac8/dac8prodspeval.bin.h:	../dac8/dac8prodspeval.bin xmosusb
	./xmosusb --bin2hex ../dac8/dac8prodspeval.bin
	@echo dac8prodspeval.bin.h generated sucessfully
	
../dac8/dacfabrice.bin.h:	../dac8/dacfabrice.bin xmosusb
	./xmosusb --bin2hex ../dac8/dacfabrice.bin
	@echo dacfabrice.bin.h generated sucessfully

dac8pro: ../dac8/dac8pro.bin.h xmosusb
	g++ -o dac8pro    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO -DDSP_CMD=0 -DBIN2HEX_CMD=0
	@echo dac8pro compiled sucessfully
	@cp dac8pro    ../dac8/utilities/osx

dac8pro32: ../dac8/dac8pro32.bin.h xmosusb
	g++ -o dac8pro32    ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRO32 -DDSP_CMD=0 -DBIN2HEX_CMD=0
	@echo dac8pro32 compiled sucessfully
	@cp dac8pro32  ../dac8/utilities/osx

dac8stereo:	../dac8/dac8stereo.bin.h xmosusb
	g++ -o dac8stereo ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8STEREO -DDSP_CMD=0 -DBIN2HEX_CMD=0
	@echo dac8stereo compiled sucessfully
	@cp dac8stereo ../dac8/utilities/osx

dac8prodspeval:	../dac8/dac8prodspeval.bin.h xmosusb
	g++ -o dac8prodspeval ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDAC8PRODSPEVAL -DBIN2HEX_CMD=0
	@echo dac8prodspeval compiled sucessfully
	@cp dac8prodspeval ../dac8/utilities/osx

dacfabrice:	../dac8/dacfabrice.bin.h xmosusb
	g++ -o dacfabrice ../src/xmosusb.cpp -I../src -I. -I../dac8 libusb-1.0.0-x86_64.dylib -m64 -DDACFABRICE -DBIN2HEX_CMD=0
	@echo dacfabrice compiled sucessfully
	@cp dacfabrice ../dac8/utilities/osx


