example for XMOS boards

the .xn files contains boards definitions and shall be added to the xTIMEcomposer environement
by using the "Install New Hardware" from the XTimeComposer Help menu !

then the filename (without .xn) shall be added in the Makefile on line "TARGET ="

in this example, 3 xn files are provided:
XU216_BASE.xn	: basic description for a core XU216 implementation
DIYINHK_DXIO.xn : pinout of the breadboard sold by DIYINHK
OKTODAC.xn		: pinout of the XMOS embded in the DAC8PRO

choose the example to run by selecting the proper SOURCES_DIR in the Makefile

example1 : it is a basic print hello world
example2

src
