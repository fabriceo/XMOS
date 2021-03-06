#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libusb.h"
// document available at http://libusb.sourceforge.net/api-1.0/modules.html

// check compiler option. if none then all activated by default
#ifndef SAMD_CMD
#define SAMD_CMD  1      // thiw will include all commands related to SAMD
#endif
#ifndef DSP_CMD
#define DSP_CMD   1      // this will include all dsp and flash related feature
#endif
#ifndef DAC8_CMD
#define DAC8_CMD   1      // this will include commands related to status and dac modes
#endif
#ifndef BIN2HEX_CMD
#define BIN2HEX_CMD   1 // this will include commands related to creating hexfile from binary
#endif


#if defined(__APPLE__)
#define SLEEP(n) system("sleep " #n)
#elif defined(__linux__)
#define SLEEP(n) system("sleep " #n)
#else // aka windows :)
// REQUIRES MinGW64.
// installer available at :
// https://sourceforge.net/projects/mingw-w64/files/Toolchains%20targetting%20Win32/Personal%20Builds/mingw-builds/installer/
// select options : architecture x86_64  and   Threads win32
// then just launch a run-terminal from the windows start menu and mingW project
// and type mingw32-make
#include "windows.h"
#define SLEEP(n) Sleep(1000*n) //windows
#endif


/* the device's vendor and product id */
#define XMOS_VID 0x20b1

#define XMOS_L1_AUDIO2_PID          0x0002
#define XMOS_L2_AUDIO2_PID          0x0004
#define XMOS_XS1_U8_Proto 			0x0006
#define XMOS_SU1_AUDIO2_PID         0x0008
#define XMOS_U8_MFA_AUDIO2_PID      0x000A
#define XMOS_XK_AUDIO_216_MC_AB		0x000C
#define xCORE_Microphone_Array		0x0010
#define XMOS_USB_Audio				0x0020
#define XMOS_DXIO                   0x2009
#define XMOS_DAC8_INITIAL           0x2009
#define XMOS_XCORE_AUDIO_AUDIO2_PID 0x3066

unsigned short pidList[] = {XMOS_XCORE_AUDIO_AUDIO2_PID, 
						    XMOS_DXIO,
						    XMOS_DAC8_INITIAL,
                            XMOS_L1_AUDIO2_PID,
                            XMOS_L2_AUDIO2_PID,
                            XMOS_SU1_AUDIO2_PID, 
                            XMOS_U8_MFA_AUDIO2_PID,
							XMOS_USB_Audio,
							xCORE_Microphone_Array,
							XMOS_XS1_U8_Proto};

#define DFU_REQUEST_TO_DEV      0x21
#define DFU_REQUEST_FROM_DEV    0xA1

#define VENDOR_REQUEST_TO_DEV    (LIBUSB_ENDPOINT_OUT | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)
#define VENDOR_REQUEST_FROM_DEV  (LIBUSB_ENDPOINT_IN  | LIBUSB_REQUEST_TYPE_VENDOR | LIBUSB_RECIPIENT_DEVICE)

// Standard DFU requests (not really used as the DFU mode is now bridged to the Audio Control commands)

#define DFU_DETACH      0
#define DFU_DNLOAD      1
#define DFU_UPLOAD      2
#define DFU_GETSTATUS   3
#define DFU_CLRSTATUS   4
#define DFU_GETSTATE    5
#define DFU_ABORT       6

#define XMOS_DFU_RESETDEVICE   0xf0 //final command after downloading new image to imediately reboot
#define XMOS_DFU_REVERTFACTORY 0xf1 //not used
#define XMOS_DFU_RESETINTODFU  0xf2 // very first command to send for using DNLOAD and GET_STATUS
#define XMOS_DFU_RESETFROMDFU  0xf3 //final command to re-enter in the standard Audio mode without rebooting
#define XMOS_DFU_SELECTIMAGE   0xf4 //not used
#define XMOS_DFU_SAVESTATE     0xf5 //not used
#define XMOS_DFU_RESTORESTATE  0xf6 //not used
#define XMOS_DFU_DNLOAD        0xf7 //same as original DFU_DNLOAD
#define XMOS_DFU_GETSTATUS     0xf8 //same as original DFU_GETSTATUS


static libusb_device_handle *devh = NULL;   // current usb device found and opened
int devhopen = -1;

unsigned XMOS_DFU_IF  = 0;                  // interface number used by the DFU driver, valid once device is opened
unsigned int deviceID = 0;                  // device number selected by the user in the command line (usefull when many xmos device found)
char * deviceSerial;                        // serial number found in the command line typed by the user

int foundDevices;
int devicePid;

// helpers
unsigned char data[64];                     // global var used to exchange data between host-client

static inline void storeInt(int idx,int val){
    data[idx] = val ;
    data[idx+1] = val>>8;
    data[idx+2] = val>>16;
    data[idx+3] = val>>24;
}

static inline void storeShort(int idx,short val){
    data[idx] = val ;
    data[idx+1] = val>>8;
}

static inline int loadInt(int idx){
    int val = data[idx];
    val |= data[idx+1]<<8;
    val |= data[idx+2]<<16;
    val |= data[idx+3]<<24;
    return val;
}

static inline int loadShort(int idx){
    short val = data[idx];
    val |= data[idx+1]<<8;
    return val;
}

static char str64[64] = "";
static char Manufacturer[64] = "";
static char Product[64] = "";
static char SerialNumber[16] = "";
unsigned BCDdevice = 0;


static char waitKey(){
    fprintf(stderr,"Press ENTER to continue...\n");
    char ch;
    scanf("%c",&ch);
    return ch;
}

static int find_usb_device(unsigned int id, unsigned int list, unsigned int printmode) // list !=0 means printing device information
{
    libusb_device *dev;
    libusb_device *founddev;
    libusb_device **devs;
    int currentId = 0;
    char string[256];
    int result;
    XMOS_DFU_IF = 0;
    foundDevices = 0;
    devicePid  = 0;

    libusb_get_device_list(NULL, &devs);
    devh = NULL;
    founddev = NULL;
    int i = 0;
    while ((dev = devs[i++]) != NULL) 
    {
        struct libusb_device_descriptor desc;
        libusb_get_device_descriptor(dev, &desc); 

        int foundDev = 0;

        if (deviceSerial == NULL) {
            if(desc.idVendor == XMOS_VID) {
                for(int j = 0; j < sizeof(pidList)/sizeof(pidList[0]); j++) {
                    if(desc.idProduct == pidList[j] /* && !list */) {
                        BCDdevice = desc.bcdDevice;
                        foundDev = 1;
                        //printf("device identified\n");
                        break; // for loop
                    }
                }
            } // id = xmos_vid
        } else {
            // check if current device correspond to given serial number
            if ((devhopen = libusb_open(dev, &devh)) >= 0)  {
                libusb_config_descriptor *config_desc = NULL;
                libusb_get_active_config_descriptor(dev, &config_desc);
                if (config_desc != NULL) {
                    if (desc.iSerialNumber) {
                        result = libusb_get_string_descriptor_ascii(devh, desc.iSerialNumber, (unsigned char*)str64, sizeof(str64));
                        if (result > 0) {
                            result = strcmp( str64, deviceSerial );
                            if ( result == 0 ) {
                                id = currentId; // force the id to be used, as the user gave a firm serial number
                                foundDev = 1;
                            }
                        }
                    }
                    libusb_free_config_descriptor(config_desc);
                } // config_desc
            }
        }

        if ((list || foundDev)) {   // "list" flag will force displaying all devices, not only the one found

            if (foundDev) foundDevices++;
            if (printmode) {
                if (foundDev) printf("\n[%d] > ",currentId);
                else printf("\n      ");
                printf("VID %04X, PID %04X, BCD %04X", desc.idVendor, desc.idProduct, desc.bcdDevice); }

            if ((devhopen = libusb_open(dev, &devh)) >=0 )  {

                libusb_config_descriptor *config_desc = NULL;
                libusb_get_active_config_descriptor(dev, &config_desc);
                if (config_desc != NULL)  {

                    if (desc.iManufacturer) {
                        result = libusb_get_string_descriptor_ascii(devh, desc.iManufacturer, (unsigned char*)str64, sizeof(str64));

                        if (result > 0) {
                            if (printmode) printf(" : %s", str64);
                            if (currentId == id)
                                result = libusb_get_string_descriptor_ascii(devh, desc.iManufacturer, (unsigned char*)Manufacturer, sizeof(Manufacturer));
                    } }
                    if (desc.iProduct) {
                        result = libusb_get_string_descriptor_ascii(devh, desc.iProduct, (unsigned char*)str64, sizeof(str64));
                        if (result > 0) {
                            if (printmode) printf("  %s", str64);
                            if (currentId == id)
                                result = libusb_get_string_descriptor_ascii(devh, desc.iProduct, (unsigned char*)Product, sizeof(Product));
                    } }

                    if (desc.iSerialNumber) {
                        result = libusb_get_string_descriptor_ascii(devh, desc.iSerialNumber, (unsigned char*)str64, sizeof(str64));
                        if (result > 0) {
                            if (printmode) printf("  %s", str64);
                            if (currentId == id)
                                result = libusb_get_string_descriptor_ascii(devh, desc.iSerialNumber, (unsigned char*)SerialNumber, sizeof(SerialNumber));
                    } }
                    libusb_free_config_descriptor(config_desc);
                } // config_desc
                if (devhopen>=0) libusb_close(devh);
            }
        }
        if (foundDev) { // only for a valid device

            if ((currentId == id) || (devicePid == desc.idProduct))  { // for the device select, go deper and show interfaces

                if ((devhopen = libusb_open(dev, &devh)) >= 0)  {
                    founddev  = dev;
                    devicePid = desc.idProduct;
                    BCDdevice = desc.bcdDevice;

                    libusb_config_descriptor *config_desc = NULL;
                    libusb_get_active_config_descriptor(dev, &config_desc); 
                    if (config_desc != NULL)  {

                        if (printmode) printf("\n");
                        // for each interface
                        for (int j = 0; j < config_desc->bNumInterfaces; j++) {

                            const libusb_interface_descriptor *inter_desc = ((libusb_interface *)&config_desc->interface[j])->altsetting;

                            if (inter_desc->bInterfaceClass == 0xFE && // DFU class
                                inter_desc->bInterfaceSubClass == 0x1)  {
                               XMOS_DFU_IF = j;
                               if (printmode>1) {
                                   printf("      (%d)  usb DFU",j);
                                   if (j == 0) printf(" => REBOOT / power-cycle required");
                                   printf("\n");
                               }

                            }

                            else if (inter_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                                inter_desc->bInterfaceSubClass == 0x01)  {
                                if (printmode>1) printf("      (%d)  usb Audio Control\n",j); }

                            else if (inter_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                                inter_desc->bInterfaceSubClass == 0x02)  {
                                if (printmode>1) printf("      (%d)  usb Audio Streaming\n",j); }

                            else if (inter_desc->bInterfaceClass == LIBUSB_CLASS_AUDIO &&
                                inter_desc->bInterfaceSubClass == 0x03)  {
                                if (printmode>1) printf("      (%d)  usb Midi Streaming\n",j); }

                            else if (inter_desc->bInterfaceClass == LIBUSB_CLASS_HID &&
                                inter_desc->bInterfaceSubClass == 0x00)  {
                                if (printmode>1) printf("      (%d)  usb HID\n",j); }

                            else if (inter_desc->bInterfaceClass == LIBUSB_CLASS_COMM &&
                                inter_desc->bInterfaceSubClass == 0x00)  {
                                if (printmode>1) printf("      (%d)  usb Communication\n",j); }
                            else if (inter_desc->bInterfaceClass == LIBUSB_CLASS_DATA &&
                                inter_desc->bInterfaceSubClass == 0x00)  {
                                if (printmode>1) printf("      (%d)  usb CDC Serial\n",j); }
                            else if (printmode>1) printf("     (%d)  %X %X unknown interface\n",j,inter_desc->bInterfaceClass,inter_desc->bInterfaceSubClass);
                           }

                        libusb_free_config_descriptor(config_desc);
                    } else {
                        if (printmode) printf(" ? No access to descriptor\n"); }
                    if (devhopen>=0) libusb_close(devh);
                } // libusb_open
                else {
#if defined(__linux__)
                    printf("\nCommand to be executed with admin rights or sudo\n");
#elif defined( WIN32 )
	printf(" => Uninstall Audio Driver first !\n");
#endif
                }
                if (!list) break;  // device selected : leave the loop, device is opened
            } // if currentId == id
            currentId++;

        } // foundDev

    } // while 1

    if (founddev)  {
        devhopen = libusb_open(founddev, &devh);
        if (printmode) printf("\n");
    }

    libusb_free_device_list(devs, 1);

    return devh ? 0 : -1;   // if a device was found then the devh handler is not nul and the device is "opened" and XMOS_DFU_IF contains interface number
}


int xmos_resetdevice(unsigned int interface) {
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_RESETDEVICE, 0, interface, NULL, 0, 0);
  return 0;
}

int xmos_enterdfu(unsigned int interface) {
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_RESETINTODFU, 0, interface, NULL, 0, 0);
  return 0;
}

int xmos_leavedfu(unsigned int interface) {
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_RESETFROMDFU, 0, interface, NULL, 0, 0);
  return 0;
}


int dfu_getStatus(unsigned int interface, unsigned char *state, unsigned int *timeout,
                  unsigned char *nextState, unsigned char *strIndex) {
  unsigned int data[2];
  libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_GETSTATUS, 0, interface, (unsigned char *)data, 6, 0);
  
  *state = data[0] & 0xff;
  *timeout = (data[0] >> 8) & 0xffffff;
  *nextState = data[1] & 0xff;
  *strIndex = (data[1] >> 8) & 0xff;
  return 0;
}

int dfu_download(unsigned int interface, unsigned int block_num, unsigned int size, unsigned char *data) {
  //printf("... Downloading block number %d size %d\r", block_num, size);
    unsigned int numBytes = 0;
    numBytes = libusb_control_transfer(devh, DFU_REQUEST_TO_DEV, XMOS_DFU_DNLOAD, block_num, interface, data, size, 0);
    return numBytes;
}


char *filename = NULL;

int write_dfu_image(unsigned int interface, char *file, int printmode, const unsigned int *firm, int firmsize) {
  
  int i = 0;
  FILE* inFile = NULL;
  int image_size = 0;
  unsigned int num_blocks = 0;
  unsigned int block_size = 64;
  unsigned int remainder = 0;
  unsigned char block_data[256];

  unsigned char dfuState = 0;
  unsigned char nextDfuState = 0;
  unsigned int timeout = 0;
  unsigned char strIndex = 0;
  unsigned int dfuBlockCount = 0;

  if (file) {
      inFile = fopen( file, "rb" );
      if( inFile == NULL ) {
        fprintf(stderr,"Error: Failed to open input data file.\n");
        return -1;
      }

      /* Discover the size of the image. */
      if( 0 != fseek( inFile, 0, SEEK_END ) ) {
        fprintf(stderr,"Error: Failed to discover input data file size.\n");
        return -1;
      }

      image_size = (int)ftell( inFile );

      if( 0 != fseek( inFile, 0, SEEK_SET ) ) {
        fprintf(stderr,"Error: Failed to input file pointer.\n");
       return -1;
      }
  } else {
      if (firm && firmsize) image_size = firmsize;
      else return -1;
  }
  num_blocks = image_size/block_size;
  remainder  = image_size - (num_blocks * block_size);

  printf("Verifying image %dkB\n",(image_size+1023)/1024);

  dfuBlockCount = 0; 

  for (i = 0; i < num_blocks; i++) {
    memset(data, 0x0, block_size);
    if (file) fread(data, 1, block_size, inFile);
    else
        for (int j=0; j<16; j++) storeInt(j*4,firm[i*16+j]);
    if (i == 0) printf("Preparing flash memory\n");
    if (i==1) printf("Downloading data...\n");
    int numbytes = dfu_download(interface, dfuBlockCount, block_size, data);
    if (numbytes != 64) {
        printf("Error: dfudownload returned an error %d at block %d.\n",numbytes, dfuBlockCount);
       return -1; }
    dfu_getStatus(interface, &dfuState, &timeout, &nextDfuState, &strIndex);
    dfuBlockCount++;
    if (printmode == 0) {
        if ((dfuBlockCount & 127) == 0) { printf("%dko\r",dfuBlockCount >> 4); fflush(stdout); }
    } else if ((dfuBlockCount & 63) == 0) { printf("#");fflush(stdout); } // 1 # = 4ko
  }
  if (printmode) printf("\n");

  if (remainder) {
    memset(data, 0x0, block_size);
    if (file) fread(data, 1, remainder, inFile);
    else
        for (int j=0; j<((remainder+3)/4); j++) storeInt(j*4,firm[num_blocks*16+j]);
    dfu_download(interface, dfuBlockCount, block_size, data);
    dfu_getStatus(interface, &dfuState, &timeout, &nextDfuState, &strIndex);
  }
   printf("Transfered %d bytes completed.\n",image_size);

  dfu_download(interface, 0, 0, NULL);
  dfu_getStatus(interface, &dfuState, &timeout, &nextDfuState, &strIndex);

  printf("USB firmware upgrade done.\n");

  return 0;
}

#if 0   // not supported anymore, just kept for futur code example
int read_dfu_image(unsigned int interface, char *file) {
  FILE *outFile = NULL;
  unsigned int block_count = 0;
  unsigned int block_size = 64;
  unsigned char block_data[64];

  outFile = fopen( file, "wb" );
  if( outFile == NULL ) {
    fprintf(stderr,"Error: Failed to open output data file.\n");
    return -1;
  }

  printf("... Uploading image (%s) from device\n", file);

  while (1) {
    unsigned int numBytes = 0;
    numBytes = dfu_upload(interface, block_count, 64, block_data);
    if (numBytes == 0) 
      break;
    fwrite(block_data, 1, block_size, outFile);
    block_count++;
    printf(".");
  }
  printf("\n");

  fclose(outFile);
  return 0;
}
#endif

int vendor_from_dev(int cmd, unsigned value, unsigned index, unsigned char *data, unsigned int size ){

    int result = libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV, cmd, value, index, data, size, 0);
    return result;
}

int vendor_to_dev(int cmd, unsigned value, unsigned index){
    unsigned char dummy[64];
    int result = libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, cmd, value, index, dummy, 0, 0);
    return result;
}

int vendor_to_dev_data(int cmd, unsigned value, unsigned index, unsigned char *data, unsigned int size){
    int result = libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, cmd, value, index, data, size, 0);
    return result;
}

int testvendor(int noprint){

    data[0] = 0x22;
    data[1] = 0x33;
    data[2] = 0x44;
    data[3] = 0x55;
    // sending data
    libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, 0x48, 5, 0, data, 4, 0);
    // receiving data
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV, 0x49, 6, 0, data, 2, 0);
    if (noprint == 0) printf("data 0-1 = 0x%X 0x%X\n",data[0],data[1]);
    return data[0] == 0x22;
}


enum VendorCommands {
/*D2H*/    VENDOR_AUDIO_MUTE  = 0xA0,  // force the DAC to mute by sending Zeros to the DAC I2S lines
/*D2H*/    VENDOR_AUDIO_UNMUTE,        // restore the normal mode
/*D2H*/    VENDOR_AUDIO_STOP,          // stop any data flow between I2S, USB, SPDIF : ready for flash access or dsp program changes
/*D2H*/    VENDOR_AUDIO_START,         // restore the normal mode
/*D2H*/    VENDOR_AUDIO_HANDLER,       // stop the exchanges between USB and DAC and SPDIF receivers/transmitters and treat specific calls
/*D2H*/    VENDOR_AUDIO_FINISH,        // restart normal exchanges

           VENDOR_TEST_VID_PID,        // this write a couple of vid pid in non volatile memory so that this is forced in descriptor at next reboot
           VENDOR_RESET_DEVICE,

/*H2D*/    VENDOR_GET_DEVICE_INFO,     // return a set of bytes representing the DAC status and modes.
/*D2H*/    VENDOR_SET_MODE,            // force the DAC in the given Mode. Front panel is informed and might save this value in eeprom

           VENDOR_SAMD_DOWNLOAD,       // for samd devices, used to reflash embeded fw or to download a newone
};
enum VendorCommandsDsp {
/*D2H*/    VENDOR_SET_DSP_PROG = 0xB0, // force the DAC to switch to the given dsp program by loading it from flash. Front panel is informed and might save this value
/*D2H*/    VENDOR_LOAD_DSP,            // load a 64bytes page in the dsp program buffer area. page number in wValue, 0 = start, then end
/*D2H*/    VENDOR_WRITE_DSP_FLASH,     // write the dsp program being in dsp program buffer, to the FLASH location N, overwrting exsting one and erasing nexts if too large to fit
/*D2H*/    VENDOR_READ_DSP_FLASH,      // read dsp program from flash memory
           VENDOR_WRITE_DSP_MEM,       // read up to 16 words in the dsp data area.
           VENDOR_READ_DSP_MEM,        // write up to 16words in the DSP data area.

/*D2H*/    VENDOR_OPEN_FLASH,          // open the flash memory for read/write
/*D2H*/    VENDOR_CLOSE_FLASH,          // close flash accees
/*H2D*/    VENDOR_READ_FLASH,          // read 64 bytes of flash memory in data partition at addres wValue, wValue representing the page index, one page bieng 64bytes
/*D2H*/    VENDOR_WRITE_FLASH,         // write 64bytes of data in a 4096bytes buffer. then write a full sector with these 4096 bytes
/*H2D*/    VENDOR_ERASE_FLASH,         // erase sector n passed in wValue, sectorsize = 4096 bytes
/*D2H*/    VENDOR_ERASE_FLASH_ALL     // erase all data partition
};


unsigned int param1   = 0;                  // command line parameter


#if defined ( SAMD_CMD ) && ( SAMD_CMD > 0 )
#include "xmosusb_samd.h"
#endif
#if defined ( DSP_CMD ) && ( DSP_CMD > 0 )
#include "xmosusb_dsp.h"
#endif
#if defined ( BIN2HEX_CMD ) && ( BIN2HEX_CMD > 0 )
#include "xmosusb_bin2hex.h"
#endif
#if defined ( DAC8_CMD ) && ( DAC8_CMD > 0 )
#include "xmosusb_dac8.h"
#endif

/*********
 *
 * main program entry point to analyse commands and launch routines
 *
 *******/
#include <math.h>
#if defined ( __XC__)
int main_only_for_testing_compiler(int argc, char **argv) {
#else
int main(int argc, char **argv) {
#endif
#ifdef WIN32

#endif
  int r = 1;
  unsigned argi = 1;


  unsigned int xmosload = 0;
  unsigned int resetdevice = 0;
  unsigned int listdev  = 0;
  unsigned int enterdfu = 0;
  unsigned int leavedfu = 0;
  unsigned int modetest = 0;

  if (argc < 2) listdev = 1;
  else {
      if ( (strcmp(argv[1], "?") == 0) || (strcmp(argv[1], "-?") == 0)  || (strcmp(argv[1], "--?") == 0) ) {

        fprintf(stderr, "Available options (optional deviceID as first option):\n");
        fprintf(stderr, "--listdevices\n");      // list all the USB devices and point the one with xmos vendor ID
        fprintf(stderr, "--resetdevice\n");      // send a DFU command for reseting the device
        fprintf(stderr, "--xmosload file\n");    // load a new firmware into the xmos flash boot partition
#if defined ( BIN2HEX_CMD ) && ( BIN2HEX_CMD > 0 )
        fprintf(stderr, "--bin2hex  file\n");    // convert a binary file into a text file with hexadecimal presentation grouped by 4 bytes, for C/C++ include
#endif
#if defined( SAMD_CMD ) && ( SAMD_CMD > 0)
        samd_printcmd();
#endif
#if defined( DSP_CMD ) && ( DSP_CMD > 0)
        dsp_printcmd();
#endif
#if defined( DAC8_CMD ) && ( DAC8_CMD > 0)
        dac_printcmd();
#endif
        return -1; }
  }

  if (argc > 2) {

#if defined ( BIN2HEX_CMD ) && ( BIN2HEX_CMD > 0 )
      if (strcmp(argv[1], "--bin2hex") == 0) {
        if (argv[2]) {
            exit(bin2hex(argv[2])); }
      }
#endif
  }

  if (argc > 1) {
      // extract deviceID forced by user eventually
      if (strlen(argv[1]) == 1) {
          deviceID = atoi(argv[1]);
          if (argc > 2) argi=2;
      } else {
          if ( (argv[1][0] >= '0') && (argv[1][0] <= '9')) {
              deviceSerial = argv[1];
              printf("Action only for device %s\n",deviceSerial);
              if (argc > 2) argi=2;
          }
      }
  }
// interception for dac8stereo or dac8pro
#if defined( DAC8STEREO ) || defined( DAC8PRO ) || defined( DAC8PRODSPEVAL ) || defined( DAC8PRO32 )
      if (argc > argi) {
          char * testcmd = strstr( argv[argi], "-" );
          if (testcmd != argv[argi]) {
              exit(execute_file(argv[argi])); }
      } else {
          exit(execute_file(NULL));
      }
#endif

      // check if there is some argument, to avoid errors when accessing pointer ...
  if (argc > argi) {

      // if not a command then passing over to special program, considering parameter as a filename.
  if (strcmp(argv[argi], "--xmosload") == 0) {
    if (argv[argi+1]) {
      filename = argv[argi+1];
    } else {
      fprintf(stderr, "No filename specified for xmosload option\n");
      exit(-1); }
    xmosload = 1;
  } else

  if(strcmp(argv[argi], "--listdevices") == 0) {
          listdev = 1; }
  else
  if(strcmp(argv[argi], "--resetdevice") == 0) {
          resetdevice = 1; }
  else
  if(strcmp(argv[argi], "--test") == 0) {
          modetest = 1; }
  else


#if defined( SAMD_CMD ) && ( SAMD_CMD > 0)
  if (samd_testcmd(argc, argv, argi)) { }
  else
#endif


#if defined( DSP_CMD ) && ( DSP_CMD > 0)
  if (dsp_testcmd(argc, argv, argi)) { }
  else
#endif

#if defined( DAC8_CMD ) && ( DAC8_CMD > 0)
  if (dac_testcmd(argc, argv, argi)) { }
  else
#endif

  {
    fprintf(stderr, "Invalid option passed to usb application\n");
    exit(-1); }
  }

  // now program is really starting

  // opening lib usb
  r = libusb_init(NULL);
  if (r < 0) {
    fprintf(stderr, "ERROR : Failed to initialise libusb...\n");
    exit(-1); }

  // searching for usb device

  r = find_usb_device(deviceID, listdev, 2); // if listdev = 1, this will print all devices found
  if (r < 0)  {
      if(!listdev) {
          fprintf(stderr, "Could not find a valid usb device [%d]\n",deviceID);
          exit(-1); }
  }

   printf("\n");
   if(resetdevice)  {
      printf("Sending reboot command...\n");
      vendor_to_dev(VENDOR_RESET_DEVICE,0,0);
      printf("Device restarting, waiting usb re-enumeration...\n");
      int result;
      printf("#");fflush(stdout);
        SLEEP(2);
        for (int i=2; i<=20; i++) {
            printf("#");fflush(stdout);
            if ((result = find_usb_device(deviceID, 0, 1)) >= 0) break;
            SLEEP(1);
        }
        if (result <0) printf("\nDevice not connected after 10sec...\n");
    }
   else
   if(modetest)  {
      printf("test\n");
      testvendor(0);
      printf("done.\n");
  } else
  if (listdev == 0) {

      if (xmosload) {
          xmos_enterdfu(XMOS_DFU_IF);
          int result = write_dfu_image(XMOS_DFU_IF, filename, 0, NULL, 0);
          xmos_resetdevice(XMOS_DFU_IF);
          if (result >= 0) {
              int oldBCD = BCDdevice;
              char oldProduct[64];
              strncpy(oldProduct,Product,64);
              if (devhopen>=0) libusb_close(devh);
              SLEEP(2);
              printf("Restarting device, waiting usb enumeration...\n");
              for (int i=1; i<=20; i++) {
                  result = find_usb_device(deviceID, 0, 1);
                  SLEEP(1);
                  if (result >=0) break;
                  //if (result >=0) if (devhopen>=0) libusb_close(devh);
              }
              if (result >= 0) {
                  printf("\nDevice upgraded successfully to v%d.%02X\n",BCDdevice>>8,BCDdevice & 0xFF); }
              else printf("\nDevice not connected...\n");
          }
      }

#if defined( SAMD_CMD ) && ( SAMD_CMD > 0)
      else if (samd_executecmd()) { }
#endif

#if defined( DSP_CMD ) && ( DSP_CMD > 0)
      else if (dsp_executecmd()) { }
#endif

#if defined( DAC8_CMD ) && ( DAC8_CMD > 0)
      else if (dac_executecmd()) { }
#endif

  } // if (listdev == 0)
  libusb_exit(NULL);
  exit(1);
}
