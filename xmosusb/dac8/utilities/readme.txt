Readme

DAC8 family firmware upgrade tool

for windows:
- uninstall any existing audio driver already used for the DAC8 (for example Thesycon drivers)
- install a generic WinUSB driver on the OKTO RESEARCH DFU (interface 3) with the opensource software Zadig
- launch dac8pro.exe or dac8stereo.exe

for OSX , open a Terminal window and then type either ./dac8pro or ./dac8stereo

for raspberry pi or linux, first install libusb with
	sudo apt-get install libusd-1.0-0-dev
the dev version has proven a better compatibility with the utility due to a proper visibility in the include path.
then launch the utility with admin right or sudo command:
	sudo ./dac8pro or sudo ./dac8stereo


in general :
flash process takes place in 1, 2 or 3 steps depending on the existing firmware found in the DAC8
At the end of the process, a success message appears.
it is recommended to power cycle (off/on) the DAC8 after the whole upgrade process.

