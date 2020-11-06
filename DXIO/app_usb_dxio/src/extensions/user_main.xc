/*
 * user_main.xc
 *
 *  Created on: 30 mai 2020
 *      Author: Fabriceo
 */
#include <platform.h>
#include <xs1.h>
#include "../../lib_spi/api/spi.h"


on tile[1] : out port p_ss[1]          = { XS1_PORT_1C };
on tile[1] : out buffered port:32 p_sclk = XS1_PORT_1D;
on tile[1] : out buffered port:32 p_mosi = XS1_PORT_1M;
on tile[1] : in  buffered port:32 p_miso = XS1_PORT_1N ;

//extern int sigmadspdemo();  // extern declaration required for ".c" files

void user_task(client spi_master_if ispi) {

    //int res = sigmadspdemo();
    ispi.begin_transaction(0, 100, SPI_MODE_3);
    int val = ispi.transfer8(0x22);
    ispi.end_transaction(1000);

}
