# The TARGET variable determines what target system the application is
# compiled for. It either refers to an XN file in the source directories
# or a valid argument for the --target option when compiling
TARGET = DIYINHK_DXIO

# The APP_NAME variable determines the name of the final .xe file. It should
# not include the .xe postfix. If left blank the name will default to
# the project name
APP_NAME = test_dxio

# The USED_MODULES variable lists other module used by the application.
USED_MODULES =
EXCLUDE_FILES = 
# The flags passed to xcc when building the application
# You can also set the following to override flags for a particular language:
# XCC_XC_FLAGS, XCC_C_FLAGS, XCC_ASM_FLAGS, XCC_CPP_FLAGS
# If the variable XCC_MAP_FLAGS is set it overrides the flags passed to
# xcc for the final link (mapping) stage.
BUILD_FLAGS =  -O3 -march=xs2a
BUILD_FLAGS += -g -Xmapper --map -Xmapper MAPFILE -Wall -fcomment-asm -report -save-temps  
BUILD_FLAGS += -DQUAD_SPI_FLASH=1 -lquadflash -DFLASH_MAX_UPGRADE_SIZE=128*1024 

# The VERBOSE variable, if set to 1, enables verbose output from the make system.
VERBOSE = 0

XCC_FLAGS = $(BUILD_FLAGS) -fxscope -DXSCOPE -DPRINTF

FINISH_Default = #$(call XFLASH_FACTORY,$(1)) 
XFLASH_FACTORY = pwd && xflash --verbose --boot-partition-size 0xC0000 --factory $(1).xe #--no-reset-on-write  && xgdb --batch-silent --ex connect --ex q #--no-compression

XMOS_MAKE_PATH ?= ../..
-include $(XMOS_MAKE_PATH)/xcommon/module_xcommon/build/Makefile.common
