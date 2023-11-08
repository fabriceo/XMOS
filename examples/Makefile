# The TARGET variable determines what target system the application is
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling
TARGET = OKTODAC

# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to
# the project name
APP_NAME = example

BUILD_FLAGS = -save-temps -Wall -fno-xta-info -O3 -fschedule -g  -report -fxscope -DXSCOPE #-fverbose-asm -march=xs2a 

# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
# XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to
# xcc for the final link (mapping) stage.
XCC_FLAGS		= $(BUILD_FLAGS)
XCC_CPP_FLAGS	= $(BUILD_FLAGS)

# The USED_MODULES variable lists other module used by the application.
USED_MODULES = lib_XCscheduler

#select where are the source files and the main() function for the choosen example to compile
SOURCE_DIRS = src/example3

INCLUDE_DIRS =

EXCLUDE_FILES =

# The VERBOSE variable, if set to 1, enables verbose output from the make system.
VERBOSE = 0

XMOS_MAKE_PATH ?= ../..
-include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common


