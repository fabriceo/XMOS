#ifndef XMOSUSB_DSP_H
#define XMOSUSB_DSP_H

/************
 *
 * Section related to DSP handling in XMOS
 *
 *************/

unsigned int dspload  = 0;
unsigned int dspprog  = 0;
unsigned int dspwrite = 0;
unsigned int dspread  = 0;
unsigned int dspreadmem = 0;
unsigned int dspreadheader = 0;
unsigned int readflash= 0;
unsigned int eraseflash=0;
unsigned int testvidpid = 0;


// send the dsp binary program to the dac ram memory

int vendor_dsp_load_page(unsigned int block_num, unsigned int size, unsigned char *data) {
    int result = libusb_control_transfer(devh, VENDOR_REQUEST_TO_DEV, VENDOR_LOAD_DSP, block_num, size, data, size, 0);
    return result;
}


// send the dsp binary program to the dac ram memory
int load_dsp_prog(char *file) {
  int i = 0;
  FILE* inFile = NULL;
  int bin_size = 0;
  unsigned int num_blocks = 0;
  unsigned int block_size = 64;
  unsigned char block_data[64];
  int result = 0;

  unsigned int timeout = 0;
  unsigned int blockCount = 0;

  inFile = fopen( file, "rb" );
  if( inFile == NULL ) {
    fprintf(stderr,"Error: Failed to open dsp program file.\n");
    return -1;
  }

  /* Discover the size of the image. */
  if( 0 != fseek( inFile, 0, SEEK_END ) ) {
    fprintf(stderr,"Error: Failed to discover dsp program file size.\n");
    return -1;
  }

  bin_size = (int)ftell( inFile );
  bin_size += 63;   //roundup
  bin_size &= ~63;

  if( 0 != fseek( inFile, 0, SEEK_SET ) ) {
    fprintf(stderr,"Error: Failed to input dsp file pointer.\n");
   return -1;
  }

  num_blocks = bin_size/block_size;

  printf("... loading opcodes (%s) to device (%dbytes)\n", file, bin_size);

  blockCount = 0;

  for (i = 0; i < num_blocks; i++) {
    memset(block_data, 0x0, block_size);
    fread(block_data, 1, block_size, inFile);
    result = vendor_dsp_load_page(blockCount, block_size, block_data);
    if (result <0) {
            fprintf(stderr,"Error: dsp_load_page returned an usb error %d at block %d\n",result,blockCount);
           return -1; }
    blockCount++;
    if ((blockCount & 63) == 0)
        printf("%dko\n", blockCount / 16);
  }

  result = vendor_dsp_load_page(0, 0, NULL);    // this is the formal command for ending the loading process
  if (0) {
          fprintf(stderr,"Error: dsp_load_page final step returned an error %d\n",result);
         return -1; }
   printf("... dsp load completed %d bytes\n",bin_size);

  return 0;
}


// read some data related to the dsp working area (parameters)
#define F28(x) ((double)(x)/(double)(1<<28))
#define F(x,y) ((double)(x)/(double)(1<<y))

void dspReadMem(int addr){
    unsigned data[16];
    unsigned char * p = (unsigned char*)&data[0];
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            VENDOR_READ_DSP_MEM, addr, 0, p, 64, 0);
    for (int i=0; i<16; i++) {
        int x = data[i];
        float f = (float)data[i];
        printf("0x%4X : %8X %d %f %f\n",i,x,x,F28(x),f);
    }
}

static const int dspTableFreq[] = { 8000, 16000, 24000, 32000, 44100, 48000, 88200, 96000, 176400,192000, 352800,384000, 705600, 768000 };

typedef struct dspHeader_s {    // 11 words
/* 0 */     int   head;
/* 1 */     int   totalLength;  // the total length of the dsp program (in 32 bits words), rounded to upper 8bytes
/* 2 */     int   dataSize;     // required data space for executing the dsp program (in 32 bits words)
/* 3 */     unsigned checkSum;  // basic calculated value representing the sum of all opcodes used in the program
/* 4 */     int   numCores;     // number of cores/tasks declared in the dsp program
/* 5 */     int   version;      // version of the encoder used MAJOR, MINOR,BUGFIX
/* 6 */     unsigned short   format;       // contains DSP_MANT used by encoder or 1 for float or 2 for double
/*   */     unsigned short   maxOpcode;    // last op code number used in this program (to check compatibility with runtime)
/* 7 */     int   freqMin;      // minimum frequency possible for this program, in raw format eg 44100
/* 8 */     int   freqMax;      // maximum frequency possible for this program, in raw format eg 192000
/* 9 */     unsigned usedInputs;   //bit mapping of all used inputs
/* 10 */    unsigned usedOutputs;  //bit mapping of all used outputs
        } dspHeader_t;


// read the header of the dsp program loaded in xmos ram memory
void dspReadHeader(){
    dspHeader_t header;
    unsigned char * p = (unsigned char*)&header;
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            VENDOR_READ_DSP_MEM, 2, 0, p, sizeof(dspHeader_t), 0);
    printf("total length = %d\n",header.totalLength);
    if (header.totalLength) {
        printf("data size    = %d\n",header.dataSize);
        printf("checksum     = 0x%X\n",header.checkSum);
        printf("num cores    = %d\n",header.numCores);
        printf("version      = %X\n",header.version);
        if (header.format)
             printf("encoded int    %d.%d\n",(32-header.format),header.format);
        else printf("encoded float\n");
        printf("freq min     = %d (%d)\n",header.freqMin,dspTableFreq[header.freqMin]);
        printf("freq max     = %d (%d)\n",header.freqMax,dspTableFreq[header.freqMax]);
    }
}

void dsp_printcmd() {
    fprintf(stderr, "--dspload  file\n");    // send the file binary content (dsp opcode) to the xmos dsp working memory
    fprintf(stderr, "--dspwrite slot\n");    // save xmos dsp memory content to flash slot N (1..15)
    fprintf(stderr, "--dspread  slot\n");    // load xmos dsp memory content with flash slot 1..15
    fprintf(stderr, "--dspreadmem addr\n");  // read 16 word of data in the dsp data area
    fprintf(stderr, "--dspheader\n");        // read dsp header of dsp program in memory to provide some info about it
    fprintf(stderr, "--dspprog  val\n");     // set the dsp program number in the front panel menu settings and load it from flash
    fprintf(stderr, "--flashread page\n");   // read 64 bytes of flash in data partition at adress page*64
    fprintf(stderr, "--flasherase sector\n");// erase a sector (4096bytes=64pages)  in data partition (starting 0)
    fprintf(stderr, "--testvidpid hex8\n");  // setup a new vid & pid in volatile memory and reboot the device
    fprintf(stderr, "--dspbasic file\n");    // read text file, and generate bin file with basic AVDSP code
}

int dsp_testcmd(int argc, char **argv, int argi) {

    if (strcmp(argv[argi], "--dspload") == 0) {
        if (argv[argi+1]) {
          filename = argv[argi+1];
        } else {
          fprintf(stderr, "No filename specified for dspload option\n");
          exit(-1); }
        dspload = 1; }
    else
    if (strcmp(argv[argi], "--dspreadmem") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
        } else {
          fprintf(stderr, "No value specified for dspreadmem option\n");
          exit(-1); }
        dspreadmem = 1; }
    else
    if (strcmp(argv[argi], "--dspheader") == 0) {
        dspreadheader = 1; }
    else
    if (strcmp(argv[argi], "--dspprog") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
        } else {
          fprintf(stderr, "No value specified for dspprog option\n");
          exit(-1); }
        dspprog = 1; }
    else
    if (strcmp(argv[argi], "--dspwrite") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
        } else {
          fprintf(stderr, "No value specified for dspwrite option\n");
          exit (-1); }
        dspwrite = 1; }
    else
    if (strcmp(argv[argi], "--dspread") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
        } else {
          fprintf(stderr, "No value specified for dspwrite option\n");
          exit (-1); }
        dspread = 1; }
    else
    if (strcmp(argv[argi], "--testvidpid") == 0) {
        if (argv[argi+1]) {
            param1 = strtoul(argv[argi+1], NULL, 16);
        }
        testvidpid = 1; }
    else
        if (strcmp(argv[argi], "--flashread") == 0) {
            if (argv[argi+1]) {
                param1 = atoi(argv[argi+1]);
            } else {
              fprintf(stderr, "No page specified for readflash option\n");
              exit( -1 ); }
            readflash = 1; }
        else
    if (strcmp(argv[argi], "--flasherase") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
        } else {
          fprintf(stderr, "No sector specified for eraseflash option\n");
          exit(-1); }
        eraseflash = 1; }
    else return 0;
    return 1;

}

int dsp_executecmd() {

    if (dspload) {
        //vendor_to_dev(VENDOR_AUDIO_MUTE,0,0); // can be an option, then user need to unmute
        vendor_to_dev(VENDOR_AUDIO_STOP,0,0);
        load_dsp_prog(filename);
        vendor_to_dev(VENDOR_AUDIO_START,0,0);
    }
    else
    if (dspwrite) {
        vendor_to_dev(VENDOR_AUDIO_STOP,0,0);
        data[0] = param1; // no need for VENDOR_OPEN_FLASH nor VENDOR_CLOSE_FLASH
        vendor_from_dev(VENDOR_WRITE_DSP_FLASH, param1, 0, data, 1);
        if (data[0]) printf("Write to flash error num %d\n",data[0]);
        vendor_to_dev(VENDOR_AUDIO_START,0,0);
    }
    else
    if (dspread) {
        vendor_to_dev(VENDOR_AUDIO_STOP, 0, 0);
        data[0] = param1; // no need for VENDOR_OPEN_FLASH nor VENDOR_CLOSE_FLASH
        vendor_from_dev(VENDOR_READ_DSP_FLASH, param1, 0, data, 4);
        int freq = (int)data[0] | (int)data[1]<<8 | (int)data[2]<<16 | (int)data[3]<<24;
        if (freq) printf("Read from flash return max frequency = %d\n",freq);
        vendor_to_dev(VENDOR_AUDIO_START, 0, 0);
    }
    else
    if (dspprog) {
        vendor_to_dev(VENDOR_SET_DSP_PROG, param1, 0);
    }
    else
    if (dspreadmem) {
        dspReadMem(param1);
    }
    else
    if (dspreadheader) {
        dspReadHeader();
    }
    else
    if (readflash) {
        vendor_to_dev(VENDOR_AUDIO_STOP, 0, 0);
        vendor_to_dev(VENDOR_OPEN_FLASH,0,0);
        vendor_from_dev(VENDOR_READ_FLASH, param1, 0, data, 64);
        for (int i = 0; i<4; i++) {
            printf("%6X : ",param1*64+i*16);
            for (int j=0; j<16; j++) {
                printf("%2X ",data[i*16+j]); }
            printf("\n"); }
        vendor_to_dev(VENDOR_CLOSE_FLASH,0,0);
        vendor_to_dev(VENDOR_AUDIO_START, 0, 0);
    }
    else
    if (eraseflash) {
        vendor_to_dev(VENDOR_AUDIO_STOP, 0, 0);
        vendor_to_dev(VENDOR_OPEN_FLASH,0,0);
        vendor_from_dev(VENDOR_ERASE_FLASH, param1, 1, data, 1);
        if (data[0]) printf("erase flash error num %d\n",data[0]);
        vendor_to_dev(VENDOR_CLOSE_FLASH,0,0);
        vendor_to_dev(VENDOR_AUDIO_START, 0, 0);
    } else
    if(testvidpid) {
        printf("testing vid pid 0x%X\n",param1);
        if (param1) {
            storeInt(0,param1);
            vendor_to_dev_data(VENDOR_TEST_VID_PID, 0, 0, data, 4); //VENDOR_RESET_DEVICE
            vendor_to_dev(VENDOR_RESET_DEVICE,0,0);
        } else
            vendor_from_dev(VENDOR_TEST_VID_PID, 0, 0, data, 4);
        param1 = loadInt(0);
        printf("vid pid = 0x%X\n",param1);
    } else return 0;
    return 1;
}

#endif // XMOSUSB_DSP_H
