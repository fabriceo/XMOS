#ifndef XMOSUSB_DAC8_H
#define XMOSUSB_DAC8_H



const unsigned int target_firmware_bin[] = {
#if defined( DAC8STEREO )
#include "../dac8/dac8stereo.bin.h"
#elif defined( DAC8PRO )
#include "../dac8/dac8pro.bin.h"
#elif defined( DAC8PRO32 )
#error "no more DAC8PRO32"
#else
//#warning "NO FIRMWARE FILE INCLUDED IN TOOL"
#endif
};


const unsigned int firmware_141_bin[] = {
#if defined( DAC8PRO )
#include "../dac8/dac8pro_141.bin.h"
#elif defined(DAC8STEREO)
#include "../dac8/dac8stereo_141.bin.h"
#else
    0
#endif
};

/* ********
 *
 * Section related to getting information from DAC8pro & DAC8Stereo
 *
 ******* */

unsigned int dacstatus= 0;
unsigned int dacmute  = 0;
unsigned int dacunmute= 0;
unsigned int dacmode  = 0;

static const int tableFreq[8] = { 44100, 48000, 88200, 96000, 176400,192000, 352800,384000 };

// show some key information about the dac
void getDacStatus(){
    printf("printing dac status:\n");
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
    printf("front panel version = %d\n",    data[4]);
    printf("Sound presence      = 0x%2X\n", data[5]);
    printf("trigger             = 0x%2X\n", data[6]);
    short vol= (signed char)data[7];
    if (vol & 128)
        printf("usb volume          = muted (%ddB)\n", -(vol & 127) );
    else
        printf("usb volume          = %ddB\n", -vol );
    vol= (signed char)data[8];
    if (vol & 128)
        printf("front panel volume  = muted (%ddB)\n", -(vol & 127) );
    else
        printf("front panel volume  = %ddB\n", -vol );
    printf("xmos BCD version    = %d.%2X", data[10] & 0x7F,data[9]);
    if (data[10] & 0x80) printf("dsp capability\n"); else printf("\n");
    printf("usb vendor ID       = 0x%4X\n", (data[11]+(data[12]<<8)) );
    printf("usb product ID      = 0x%4X\n", (data[13]+(data[14]<<8)) );
    int maxFreq = loadInt(15);//(data[15]+(data[16]<<8)+(data[17]<<16)+(data[18]<<24));
    printf("maximum frequency   = %d\n",    maxFreq );
    int progress = loadInt(19); //(data[19]+(data[20]<<8)+(data[21]<<16)+(data[22]<<24));
    printf("front panel status  = %d ",    progress );
#if defined( SAMD_CMD ) && (SAMD_CMD > 0)
    printfwstatus(progress);
#endif
	//DSP related section
    int maxTask = data[24];
    printf("maximum dsp tasks   = %d\n",    (maxTask) );
    int maxDsp = 100000000/i2sfreq;
    if (maxTask) {
        printf("DSP program number  = %d\n",    data[3]);
        printf("decimation factor   = %d\n",    data[23] );
        for (int i=0; i<= maxTask; i++)
            if (i != maxTask)
                 printf("dsp %d: instructions = %d\n", i+1, loadShort(25+i+i));//(data[25+i+i]+(data[25+i+i+1]<<8)) );
            else {
                int maxInst = loadShort(25+i+i);//(data[25+i+i]+(data[25+i+i+1]<<8));
                printf("max    instructions = %d / %d = %d%%fs\n", maxInst , maxDsp, (int)(maxInst*100.0/(float)maxDsp) );  }
            }
#if defined( SAMD_CMD ) && (SAMD_CMD > 0)
    if (progress) fwprogress(0);    // display a simple text message about the dac status from fw perspective
#endif
}



void dac_printcmd() {
    fprintf(stderr, "--dacstatus            provide details on the DAC internal statuses\n");        // return the main registers providing informations on the dac and data stream status
    fprintf(stderr, "--dacmode  val         change de DAC mode 0 PureUSB, 1 USB/AES, 2 PureAES\n");     // set the dac mode with the value given. front panel informed
    fprintf(stderr, "--dacmute              force 0 on I2S bus to DAC, low level command, front panel not informed\n");          // set the dac in mute till unmute
    fprintf(stderr, "--dacunmute            restore normal mode of operation\n");        // unmute the dac
}

int dac_testcmd(int argc, char **argv, int argi) {

    if(strcmp(argv[argi], "--dacstatus") == 0) {
        if (argv[argi+1])  param1 = atoi(argv[argi+1]);
        dacstatus = 1; }
    else
    if (strcmp(argv[argi], "--dacmode") == 0) {
        if (argv[argi+1]) {
            param1 = atoi(argv[argi+1]);
			if (param1>2) {
				fprintf(stderr, "Bad value specified for dacmode option\n");
				exit (-1); }
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




void show_fp_status(){
    printf("Monitoring front panel firmware upgrade (up to 40 seconds):\n");
static int oldprogress;
int dashed = 0;
while(1) {
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
        VENDOR_GET_DEVICE_INFO, 0, 0, data, 64, 0);

    int progress = loadInt(19);//data[19]+(data[20]<<8)+(data[21]<<16)+(data[22]<<24);
    if (progress != oldprogress) {
        oldprogress = progress;
        if (progress <= 0) {
            if (dashed) printf("\n");
#if defined( SAMD_CMD ) && (SAMD_CMD > 0)
            printfwstatus(progress);
#endif
            switch (-progress) {
            case fw_running     :
            case fw_error       :
            case fw_faulty      : return; }
        }
        else {
            int num = progress / 1024;
            for (int i=0; i<num; i++) printf("#");
            if (num) {
				printf("\r");fflush(stdout);
				dashed = 1; }
        }
    }
}
}


int search_dac8_product(char * filename){

    printf("Searching OKTO Research product on the USB ports ...");
    int r = find_usb_device(deviceID, 0, 1);
#ifdef WINDOWS
	  if (BCDdevice >= 0x150) {
		printf("found DAC8 product v%d.%02X\n", BCDdevice>>8,BCDdevice & 0xFF);
		printf("BCD version >= 150 : Please use Thesycon DFU utility to upgrade XMOS firmware\n");
		exit(-1);
	  }
#endif
    if (r < 0)  {
            fprintf(stderr, "\nCould not find or access a valid DAC8 product\n"
                            "please try uninstalling any existing drivers.\n\n");
            waitKey();
            libusb_exit(NULL);
            exit(-1); }
    char * teststr  = strstr(Product, "DAC8");
    char * teststr2 = strstr(Product, "DACSTEREO"); //added 20230429
    if ((teststr == Product) || (teststr2 == Product)) {
        printf("found %s v%d.%02X\n",Product, BCDdevice>>8,BCDdevice & 0xFF);
        teststr = strstr(Product, "_");
        if (teststr) teststr[0] = 0;    //delete characters above _
    } else  {
        if (deviceID) printf("Device [%d] not found\n",deviceID);
        else printf("No compatible product found...\n");
        if (filename == NULL) find_usb_device(0, 0, 1);
        libusb_exit(NULL);
        waitKey();
        exit(-1); }

    return 1;
}

int execute_file(char * filename){
    FILE* inFile = NULL;

    printf("\nFirmware upgrade tool for DAC8 products.\n\n");
    int repeat = 0;
    int BCDprev = 0;
    int defaultfile = 0;


entry:
	if (repeat > 3) {
		printf("Upgrade failed %d times. Please power cycle the device and try once more.\n",repeat);
        libusb_exit(NULL);
        waitKey();
        exit(-1);
	}
    int r = libusb_init(NULL);
    if (r < 0) {
        fprintf(stderr, "Failed to initialise libusb...\n");
#if defined(__linux__)
        printf("Consider install libusb with sudo apt install libusb ,or libusb-1.0, or libusb-1.0-0-dev ...\n");
#endif

      waitKey();
      exit(-1); }

    int result = search_dac8_product(filename);

#ifdef WINDOWS
    if (BCDdevice >= 0x150) {
        printf("\nThis tool cannot be used to upgrade DAC8 version >= 1.50");
        printf("\nPlease use the new Thesycon DFU utility made for OKTO Research\n\n");
        libusb_exit(NULL);
        exit(-1);
    }
#endif
    if (repeat == 0) {

        BCDprev = BCDdevice;

        if (filename == NULL) {
            defaultfile = 1;
            filename = strcat(Product,".bin");
        }

        char * teststr = strstr(filename, Product);
        if (teststr == NULL) {
            printf("file %s not compatible with %s\n",filename,Product);
            waitKey();
            exit(-1); }
    }

    inFile = fopen( filename, "rb" );

    if( inFile == NULL ) {
        if (sizeof(target_firmware_bin)>1) {
			#if defined( DAC8PRO )
			char * test = strstr(Product, "DAC8PRO");
			#elif defined ( DAC8STEREO )
			char * test = strstr(Product, "DAC8STEREO");
			if (test != Product) test = strstr(Product, "DACSTEREO");   //added 20230429 to cope with some products in the field
			#else
			char * test = NULL;
			#endif
			if (test != Product) {
				printf("No embedded %s image file, please specify a binary file in the command line.\n",Product);
                libusb_exit(NULL);
                waitKey();
				exit(-1); }
            filename = NULL;    // will use the inmemory image
        } else {
            if (defaultfile) {
                printf("No file for upgrade, please specify a binary file in the command line.\n");
                libusb_exit(NULL);
                waitKey();
                exit(-1); }
            fprintf(stderr,"Error: Failed to open file %s or file not found.\n",filename);
            libusb_exit(NULL);
            waitKey();
            exit(-1);
        }
    }
    if (filename) {
        printf("Opening file %s\n", filename);
        fclose(inFile); }

    if (BCDdevice < 0x141) {
    
            printf("Upgrading USB firmware to intermediate version 1.41, do not disconnect...\n");
            xmos_enterdfu(XMOS_DFU_IF);
            SLEEP(1);
            result = write_dfu_image(XMOS_DFU_IF, NULL, 1, firmware_141_bin, sizeof(firmware_141_bin) );
            if (result >= 0) {
                xmos_resetdevice(XMOS_DFU_IF);
                int oldBCD = BCDdevice;
                if (devhopen>=0) libusb_close(devh);
                printf("Restarting device, waiting usb enumeration...\n");
				SLEEP(4);
                for (int i=1; i<=3; i++) {
                    result = find_usb_device(deviceID, 0, 1);
                    SLEEP(2);
                    if (result >=0) break; //&& (oldBCD != BCDdevice)) break;
                }
            }
            if (BCDdevice == 0x141) {
                printf("Preliminary upgrade to v1.41 done successfully.\n\n");
                BCDprev = 0x141;
                if (devhopen>=0) libusb_close(devh);
  				libusb_exit(NULL);
                goto entry;
            } else {
            	repeat ++;
            	printf("Retrying upgrade...\n\n");
            	libusb_exit(NULL);
            	goto entry;
            }
    }

    printf("Upgrading USB firmware, do not disconnect...\n");
    xmos_enterdfu(XMOS_DFU_IF);
    SLEEP(1);
    result = write_dfu_image(XMOS_DFU_IF, filename, 1, target_firmware_bin, sizeof(target_firmware_bin) );
    if (result >= 0) {
        xmos_resetdevice(XMOS_DFU_IF);
        int oldBCD = BCDdevice;
		char oldProduct[64];
		strncpy(oldProduct, Product, 64);
        if (devhopen>=0) libusb_close(devh);
        printf("Restarting device %s, waiting usb enumeration...\n", Product);
		SLEEP(4);
        for (int i=1; i<=3; i++) {
            result = find_usb_device(deviceID, 0, 1);
            SLEEP(2);
			//int test = strcmp(Product,oldProduct);
            if ((result >=0) || (BCDdevice>=0x0150)) break; //&& ((oldBCD != BCDdevice)||(test |= 0))) break;
            //if (result >=0) libusb_close(devh);
        }
    }
    if ((result >=0) || (BCDdevice>=0x0150)) {
        printf("Device version v%d.%02X\n",BCDdevice>>8,BCDdevice & 0xFF);
#if defined( WIN32 )
        if (BCDdevice > 0x141) {
            if (BCDdevice < 0x0150) show_fp_status();	//Bizarre isnt it >= instead
            else {
                printf("please now wait max 60 seconds for font-panel firmware upgrade... Do not power off!\n");
                for (int i=0; i< 60; i++) {
                    printf("#");fflush(stdout);
                    SLEEP(1);   }
                printf("\nUpgrade process completed. Press Volume knob to display new front panel menus.\n");
            }
        }
#else
        if (BCDdevice > 0x141) show_fp_status();
#endif

        if (BCDdevice > BCDprev) printf("Success.\n");
        waitKey();
    } else {
    	repeat ++;
        libusb_exit(NULL);
        goto entry;
    }

    if (devhopen>=0) libusb_close(devh);
    libusb_exit(NULL);
    return 0;
}

#endif
