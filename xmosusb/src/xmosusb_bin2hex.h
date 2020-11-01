#ifndef XMOSUSB_BIN2HEX_H
#define XMOSUSB_BIN2HEX_H


// used to convert a binary file like the atmel studio code generated for the front panel, into a c header file .h
int bin2hex(char *file){
    FILE* inFile = NULL;

    int bin_size = 0;
    unsigned int num_blocks = 0;
    unsigned int block_size = 4096;
    unsigned int remainder = 0;
    unsigned int sum;
    unsigned char block_data[0x40000];  // max 256ko

    inFile = fopen( file, "rb" );
    if( inFile == NULL ) {
      fprintf(stderr,"Error: Failed to open binary.\n");
      return -1;
    }

    /* Discover the size of the file. */
    if( 0 != fseek( inFile, 0, SEEK_END ) ) {
      fprintf(stderr,"Error: Failed to discover binary file size.\n");
      return -1;
    }

    bin_size = (int)ftell( inFile );

    if( 0 != fseek( inFile, 0, SEEK_SET ) ) {
      fprintf(stderr,"Error: Failed to input file pointer.\n");
     return -1;
    }

    num_blocks = bin_size/block_size;
    remainder = bin_size - (num_blocks * block_size);

    printf("... converting binary (%s) to .h file\n", file);

    for (int i = 0; i < num_blocks; i++) {
      memset(&block_data[i*block_size], 0x0, block_size);
      fread(&block_data[i*block_size], 1, block_size, inFile);
      }


    if (remainder) {
      memset(&block_data[num_blocks*block_size], 0x0, block_size);
      fread(&block_data[num_blocks*block_size], 1, remainder, inFile);
    }
     printf("loaded %d bytes\n",bin_size);
     fclose(inFile);

    FILE *outFile = NULL;

    outFile = fopen( strcat(file, ".h"), "w" );
    if( outFile == NULL ) {
      fprintf(stderr,"Error: Failed to open output .h file.\n");
      return -1;
    }

    bin_size += 3; bin_size &= ~3; // round to 4 bytes words
    num_blocks = (bin_size+63) / 64;
    bin_size /= 4; sum = 0;
    fprintf(outFile,"//#define BIN2C_SIZE %d\n",bin_size);
    fprintf(outFile,"//const unsigned int bin2c[BIN2C_SIZE] = { // hex in little-endian for lsb first\n");
    for (int i = 0; i < num_blocks; i++) {
        for (int j=0; (j<16) ;j++) {
            unsigned char * pch = &block_data[ i*64 + j*4 ];
            unsigned int* p = (unsigned int*)(pch);
            sum += *p;
            if (--bin_size)
                fprintf(outFile,"0x%X, ",*p);
            else
                { fprintf(outFile,"0x%X   // };  // checksum = 0x%X",*p, sum); break; }
        }
        fprintf(outFile,"\n");
    }
    fprintf(outFile,"\n");

    fclose(outFile);
    printf("checksum = 0x%X\n", sum);
    return 0;
  }

#endif //XMOSUSB_BIN2HEX_H

