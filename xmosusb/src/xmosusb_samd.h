#ifndef XMOSUSB_SAMD_H
#define XMOSUSB_SAMD_H

/* *******
 *
 * Section specific to handling SAMD devices from XMOS, via uart command & samba monitor
 *
 ********* */

// variables used by "main"
unsigned int samdload = 0;
unsigned int samdreflash = 0;
unsigned int samdbootloaderversion = 0;
unsigned int samdbootloader = 0;
unsigned int samdreset = 0;
unsigned int samderase = 0;


int samd_download(unsigned int value, unsigned int block_num, unsigned int size, unsigned char *data) {
    unsigned int numBytes = 0;
    numBytes = libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, VENDOR_SAMD_DOWNLOAD, value, block_num, data, size, 0);
    return numBytes;
}


static inline int getIntFromBuffer(unsigned char buf[], int idx) {
    return  buf[idx] | (buf[idx+1]<<8) | (buf[idx+2]<<16) | (buf[idx+3]<<24);
}

enum fwsteps_e { fw_not_started, fw_starting, fw_checking, fw_checkfailed, fw_obsolete, fw_running,
                 fw_startflashing, fw_bootloader, fw_waitbootloader, fw_version, fw_init, fw_erase,
                 fw_block, fw_completed, fw_error, fw_faulty };

// send many commands to download a new samd firmware by using 64bytes chunk.
int samdloadbinfile(unsigned int interface, char *file) {
  int i = 0;
  FILE* inFile = NULL;
  int image_size = 0;
  unsigned int num_blocks = 0;
  unsigned int block_size = 64;
  unsigned int remainder = 0;
  unsigned char block_data[64];

  unsigned int blockCount = 0;

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

  num_blocks = image_size / block_size;
  remainder  = image_size - (num_blocks * block_size);

  printf("Downloading image (%s) to samd processor (%dbytes)\n", file, image_size);

  blockCount = 0;

  for (i = 0; i < num_blocks; i++) {
    memset(block_data, 0x0, block_size);
    fread(block_data, 1, block_size, inFile);
    int numbytes = samd_download(fw_block, blockCount, block_size, block_data);
    if (numbytes != 64) {
        fprintf(stderr,"Error: samd_download returned an error at block %d : %d\n", i, numbytes);
       return -1; }

    blockCount++;
    if ((blockCount & 15) == 0) printf("%dko\n",blockCount >> 4);
  }

  if (remainder) {
    memset(block_data, 0x0, block_size);
    fread(block_data, 1, remainder, inFile);
    samd_download(fw_block, blockCount, remainder, block_data);
  }
   printf("downloaded %d bytes\n",image_size);

  return 0;
}


void printfwstatus(int status){
    if (status <= 0)
        switch (-status) {
        case fw_not_started : printf("Device not started (reseted)\n"); break;
        case fw_starting    : printf("Device starting (out of reset)\n"); break;
        case fw_checking    : printf("Checking front panel fw version\n"); break;
        case fw_checkfailed : printf("fw check failed\n"); break;
        case fw_obsolete    : printf("fw older than xmos embedded fw\n"); break;
        case fw_running     : printf("Device running ok\n"); break;
        case fw_startflashing  : printf("Front panel flashing started\n"); break;
        case fw_bootloader  : printf("Enterring bootloader\n"); break;
        case fw_waitbootloader  : printf("Waiting bootloader ack\n"); break;
        case fw_version     : printf("Getting bootloader version\n"); break;
        case fw_init        : printf("Bootloader ready\n"); break;
        case fw_erase       : printf("Erasing device flash\n"); break;
        case fw_completed   : printf("Front panel fw upgrade DONE\n"); break;
        case fw_error       : printf("Front panel fw upgrade  ERROR\n"); break;
        case fw_faulty      : printf("Front panel device probably FAULTY...\n"); break;
        default : printf("\n");
        }
    else {
        printf("downloaded %d bytes\n", status);
    }
}

int fwstatus;
// display the samd update progress (not finished, especially on xmos side)
void fwprogress(int exitstate){
    unsigned char data[64];
    int loop = 1;
    int i=0;
    while (loop) {
        libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
                VENDOR_GET_DEVICE_INFO, 0, 0, data, 64, 0);
        static int old = -100;
        int progress = data[19]+(data[20]<<8)+(data[21]<<16)+(data[22]<<24);
        if (progress <= 0) fwstatus = progress;
        if (progress != old) {
            old = progress;
            if (progress <= 0) {
                switch (-progress) {
                case fw_running     :
                case fw_error       :
                case fw_faulty      : printfwstatus(progress); loop=0; break;

                case fw_completed   :
                case fw_not_started :
                case fw_starting    :
                case fw_checking    :
                case fw_bootloader  :
                case fw_waitbootloader  :
                case fw_version     :
                case fw_init        :
                case fw_erase       : printfwstatus(progress);  break;
                }
                if (exitstate == -progress) loop = 0;
            } else
                printf("downloading %d bytes\n", progress);
            i=0;
        } else {
            printf("%d\r",i); i++;
        }
    }
    printf("\n");
}

void samd_printcmd() {
    fprintf(stderr, "--samdload file\n");    // NOT IMPLEMENTED load the front panel flash with a samd binary program and reboot
    fprintf(stderr, "--samdreflash\n");      // re-initiate flashing the front panel firmware with the binary file embeded in xmos source code.
    fprintf(stderr, "--samdbootloader\n");   // enters samd into bootloader mode, to prepare for erasing and downloading new fw
    fprintf(stderr, "--samdbootloaderversion\n");      // provide the text received when sending V# to the bootloader
    fprintf(stderr, "--samdreset\n");        // reinitialize dac application and cycle reset front panel
    fprintf(stderr, "--samderase\n");        // erase all samd flash
}

int samd_testcmd(int argc, char **argv, int argi) {
    if (strcmp(argv[argi], "--samdload") == 0) {
      if (argv[argi+1]) {
        filename = argv[argi+1];
      } else {
        fprintf(stderr, "No filename specified for samdload option\n");
        return -1; }
      samdload = 1;
    } else
    if (strcmp(argv[argi], "--samdreflash") == 0) {
      samdreflash = 1; }
    else
    if (strcmp(argv[argi], "--samdbootloader") == 0) {
        samdbootloader = 1; }
    else
    if (strcmp(argv[argi], "--samdbootloaderversion") == 0) {
        samdbootloaderversion = 1; }
    else
    if (strcmp(argv[argi], "--samdreset") == 0) {
        samdreset = 1; }
    else
    if (strcmp(argv[argi], "--samderase") == 0) {
        samderase = 1; }
    else return 0;
    return 1;
}

int samd_executecmd() {
    int res;
    if (samdload) {
          printf("muting dac\n");
          vendor_to_dev(VENDOR_AUDIO_MUTE,0,0);
          fwprogress(fw_init);  // wait for status running or init (bootloader)
          if (fwstatus != -fw_init) {
              printf("force bootload\n");
              res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_bootloader, 0, data, 4);
              fwprogress(fw_init); }
          printf("force erase\n");
          res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_erase, 0, data, 4);
          if (res == 4) {
              res = getIntFromBuffer(data, 0);
              if (res == -fw_erase) {
                  printf("get version\n");
                  res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_version, 0, data, 64);
                  if (res != 4) {
                      samdloadbinfile(XMOS_DFU_IF, filename);
                      printf("restarting\n");
                      res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_starting, 0, data, 4);
                      fwprogress(fw_running);
                  }
              }
          } else
              printf("cant erase flash ...\n");
          printf("unmuting dac\n");
          vendor_to_dev(VENDOR_AUDIO_UNMUTE,0,0);
      }
      else
      if (samdreflash) {
          vendor_to_dev(VENDOR_AUDIO_MUTE,0,0);
          printf("muting dac\n");
          res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_startflashing, 0, data, 4);
          fwprogress(fw_running);
          vendor_to_dev(VENDOR_AUDIO_UNMUTE,0,0);
          printf("unmuting dac\n");
      } else
      if (samdbootloaderversion) {
          int res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_version, 0, data, 64);
          if (res>4) {
              printf("received %d chars : \n%s",res, data);
              fwprogress(fw_init);
          } else
              fwprogress(fw_init);
      }
      else
      if (samdbootloader) {
          int res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_bootloader, 0, data, 4);
          fwprogress(fw_init);
      }
      else
      if (samdreset) {
          int res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_starting, 0, data, 4);
          fwprogress(fw_running);
      }
      else
      if (samderase) {
          int res = vendor_from_dev(VENDOR_SAMD_DOWNLOAD, fw_erase, 0, data, 4);
          fwprogress(fw_init);
      } else return 0;
    return 1;
}

#endif // XMOSUSB_SAMD_H
