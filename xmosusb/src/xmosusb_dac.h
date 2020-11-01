#ifndef XMOSUSB_DAC_H
#define XMOSUSB_DAC_H


/* ********
 *
 * Section related to getting information from OKTO Research DAC8pro & DAC8Stereo
 *
 ******* */


unsigned int dacstatus= 0;
unsigned int dacmute  = 0;
unsigned int dacunmute= 0;
unsigned int dacmode  = 0;

static const int tableFreq[8] = { 44100, 48000, 88200, 96000, 176400,192000, 352800,384000 };

// show some key information about the dac
void getDacStatus(){
    unsigned char data[64];
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
            VENDOR_GET_DEVICE_INFO, 0, 0, data, 64, 0);
    printf("DAC mode            = 0x%X ", data[0]);
    switch (data[0]& 7) {
    case 0: printf("pure usb"); break; case 2: printf("usb-aes"); break; case 3: printf("pure aes"); break;
    case 4: printf("pure rpi-usb"); break; case 6: printf("rpi-usb/aes"); break; case 7: printf("pure aes"); break; }
    if (data[0] & 8) printf(" dsp\n"); else printf("\n");
    int i2sfreq = tableFreq[data[1] & 7];
    printf("I2S Audio config    = 0x%2X (%d)\n", data[1], tableFreq[data[1] & 7]);
    printf("USB Audio config    = 0x%2X (%d)\n", data[2], tableFreq[data[2] & 7]);
    printf("DSP program number  = %d\n",    data[3]);
    printf("front panel version = %d\n",    data[4]);
    printf("Sound presence      = 0x%2X\n", data[5]);
    printf("trigger             = 0x%2X\n", data[6]);
    short vol= (signed char)data[7];
    if (vol>0)
        printf("usb volume          = muted (%ddB)\n", -vol-1 );
    else
        printf("usb volume          = %ddB\n", vol );
    vol= (signed char)data[8];
    if (vol>0)
        printf("front panel volume  = muted (%ddB)\n", -vol );
    else
        printf("front panel volume  = %ddB\n", vol );
    printf("xmos BCD version    = 0x%4X\n", (data[9]+(data[10]<<8)) );
    printf("usb vendor ID       = 0x%4X\n", (data[11]+(data[12]<<8)) );
    printf("usb product ID      = 0x%4X\n", (data[13]+(data[14]<<8)) );
    int maxFreq = (data[15]+(data[16]<<8)+(data[17]<<16)+(data[18]<<24));
    printf("maximum frequency   = %d\n",    maxFreq );
    int progress = (data[19]+(data[20]<<8)+(data[21]<<16)+(data[22]<<24));
    printf("front panel status  = %d ",    progress );
    printfwstatus(progress);
    printf("decimation factor   = %d\n",    data[23] );

    printf("maximum dsp tasks   = %d\n",    (data[24]) );
    int maxDsp = 100000000/i2sfreq/data[23];
    if (data[24]) {
        for (int i=0; i<= data[24]; i++)
            if (i != data[24])
                 printf("dsp %d: instructions = %d\n", i+1, (data[25+i+i]+(data[25+i+i+1]<<8)) );
            else {
                int maxInst = (data[25+i+i]+(data[25+i+i+1]<<8));
                printf("maxi   instructions = %d / %d = %d%%fs\n", maxInst , maxDsp, (int)(maxInst*100.0/(float)maxDsp) );  }
            }
    if (progress) fwprogress(0);    // display a simple text message about the dac status from fw perspective
}



void dac_printcmd() {
    fprintf(stderr, "--dacstatus\n");        // return the main registers providing informations on the dac and data stream status
    fprintf(stderr, "--dacmode  val\n");     // set the dac mode with the value given. front panel informed
    fprintf(stderr, "--dacmute\n");          // set the dac in mute till unmute
    fprintf(stderr, "--dacunmute\n");        // unmute the dac
}

int dac_testcmd(int argc, char **argv, int argi) {

    if(strcmp(argv[argi], "--dacstatus") == 0) {
        if (argv[argi+1])  param1 = atoi(argv[argi+1]);
        dacstatus = 1; }
    else
    if (strcmp(argv[argi], "--dacmode") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
        } else {
          fprintf(stderr, "No value specified for dacmode option\n");
          exit (-1); }
        dacmode = 1; }
    else
    if(strcmp(argv[argi], "--dacmute") == 0) {
            dacmute = 1; }
    else
    if(strcmp(argv[argi], "--dacunmute") == 0) {
            dacunmute = 1; }
    else return 0;
    return 1;
}


int dac_executecmd() {
    if (dacstatus) {
        getDacStatus();
    }
    else
    if (dacmode) {
        vendor_to_dev(VENDOR_SET_MODE,param1,0);
    }
    else
    if (dacmute) {
        vendor_to_dev(VENDOR_AUDIO_MUTE,0,0);
    }
    else
    if (dacunmute) {
        vendor_to_dev(VENDOR_AUDIO_UNMUTE,0,0);
    }
    else return 0;
    return 1;
}

#endif
