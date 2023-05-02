Readme

DAC8 family firmware upgrade tool

for windows 10:
- uninstall any existing audio driver already used for the DAC8 (for example Thesycon drivers)
- install the generic WinUSB driver on the DAC8 or OKTO RESEARCH DFU (interface 3) with the opensource software Zadig (tested version 2.8)
- launch dac8pro.exe or dac8stereo.exe
REMARK : doesnt work with Windows7 due to libusb/winusb timeout limitation

for OSX , open a Terminal window and then type either ./dac8pro or ./dac8stereo
no requirement for libusb library has it is included (static version 1.0.26) in the executable file

for raspberry pi or linux, first install libusb with
	sudo apt-get install libusd-1.0-0-dev
the dev version has proven a better compatibility with the utility due to a proper visibility of the library in the include path.
then launch the utility with admin right or sudo command:
	sudo ./dac8pro or sudo ./dac8stereo


in general :
flash process takes place in 1, 2 or 3 steps depending on the existing firmware found in the DAC8
once firmware is flashed, the front panel firmware will be flashed (takes 40 seconds)
At the end of the process, a success message appears.



