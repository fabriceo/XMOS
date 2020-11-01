#ifndef XMOSUSB_OKTO_H
#define XMOSUSB_OKTO_H


void show_fp_status(){
    printf("Front panel firmware upgrade evaluation...\n");
static int oldprogress;
int dashed = 0;
while(1) {
    libusb_control_transfer(devh, VENDOR_REQUEST_FROM_DEV,
        VENDOR_GET_DEVICE_INFO, 0, 0, data, 64, 0);

    int progress = data[19]+(data[20]<<8)+(data[21]<<16)+(data[22]<<24);
    if (progress != oldprogress) {
        oldprogress = progress;
        if (progress <= 0) {
            if (dashed) printf("\n");
            printfwstatus(progress);
            switch (-progress) {
            case fw_running     :
            case fw_error       :
            case fw_faulty      : exit(-1); }
        }
        else {
            int num = progress / 1024;
            for (int i=0; i<num; i++) printf("#");
            printf("\r");
            dashed = 1;
        }
    }
}
}


int search_okto_product(){

    printf("Searching our product on the USB bus ...");
    int r = find_xmos_device(0, 0); // if listdev = 1, this will print all devices found
    if (r < 0)  {
        fprintf(stderr, "\nCould not find a valid usb device\n\n");
        exit(-1); }

    int dac = 0;
    char * teststr = strstr(Product, "DAC8PRO");
    if (teststr == Product) {
        printf("\nfound DAC8PRO (%s) v%d.%02X\n",SerialNumber,BCDdevice>>8,BCDdevice & 0xFF);
        Product[7] = 0;
        dac = 1;
    } else {
        teststr = strstr(Product, "DAC8STEREO");
        if (teststr == Product) {
            printf("\nfound DAC8STEREO (%s) v%d.%02X\n",SerialNumber,BCDdevice>>8,BCDdevice & 0xFF);
            Product[10] = 0;
            dac = 2;
        }
    }
    if (dac == 0) { printf("product not supported with this tool.\n"); exit(-1); }

    return dac;
}

int execute_file(char * filename){
    FILE* inFile = NULL;

    printf("\nOKTO Research DAC8PRO and DAC8STEREO firmware upgrade tool.\n\n");

    int r = libusb_init(NULL);
    if (r < 0) {
      fprintf(stderr, "failed to initialise libusb...\n");
      exit(-1); }

    int result = search_okto_product();

    char * teststr = strstr(filename, Product);
    if (teststr != filename) {
        printf("file not compatible with %s\n",Product);
        exit(-1);
    }
#if 1
    inFile = fopen( filename, "rb" );

    if( inFile == NULL ) {
      fprintf(stderr,"Error: Failed to open input file or file not found.\n");
      return -1;
    }

    fclose(inFile);
#endif
    printf("Upgrading USB firmware, do not disconnect...\n");
    xmos_enterdfu(XMOS_DFU_IF);
    SLEEP(1);
    result = write_dfu_image(XMOS_DFU_IF, filename, 1);
    if (result >= 0) {
        xmos_resetdevice(XMOS_DFU_IF);
        printf("Restarting device, waiting usb enumeration...\n");
        SLEEP(1);
        for (int i=1; i<=10; i++) {
            printf("%d / 10 seconds\r",i);
            if ((result = find_xmos_device(0, 0)) >= 0) break;
            SLEEP(1);
        }
    }
    if (result >= 0) {
        printf("\nDevice upgraded successfully to v%d.%02X\n",BCDdevice>>8,BCDdevice & 0xFF);
        show_fp_status();
    } else {
        printf("\nProblem during upgrade. disconnect, unplug-plug power supply, reconnect and retry\n");
        exit(-1);
    }


    libusb_close(devh);
    libusb_exit(NULL);
    return 0;
}


#endif
