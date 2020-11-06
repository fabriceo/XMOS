#ifndef _USER_MAIN_H_
#define _USER_MAIN_H_

// this file is included by "customdefines.h" and become visible to "audio/main.xc"
#ifdef __XC__


#ifdef IAP_EA_NATIVE_TRANS
#include <xccompat.h>
void ea_protocol_demo(chanend c_ea_data);

#define USER_MAIN_CORES \
    on tile[1] : ea_protocol_demo(c_ea_data);

#endif /* IAP_EA_NATIVE_TRANS */

#include "../../lib_spi/api/spi.h"    // from module lib_spi folder api

// this defines is added just before the main() function defined in audio/main.xc (around line 477)
// can be used to declare prototypes used by USER_MAIN_CORES
#define USER_MAIN_PROTOTYPES \
        extern out port p_ss[1]; \
        extern out buffered port:32 p_sclk; \
        extern out buffered port:32 p_mosi; \
        extern in  buffered port:32 p_miso; \
        void user_task(client spi_master_if ispi); \


// this defines is used within the main() function defined in audio/main.xc, just before the main par statement
// can be used to declare interfaces for example
#define USER_MAIN_DECLARATIONS \
        spi_master_if ispi[1];


    // this defines is used at the very botom of the main par in audio/main.xc
#define USER_MAIN_CORES \
        on tile[1] : user_task(ispi[0]); \
        on tile[1] : spi_master(ispi, 1, p_sclk, p_mosi, p_miso , p_ss, 1, null); \
//[[distribute]] \

#endif // __XC__

#endif /* _USER_MAIN_H_ */
