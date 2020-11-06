#include <xs1.h>

#include <assert.h>
#include "devicedefines.h"
#include <platform.h>
#include "gpio_access.h"
#include "i2c_shared.h"
//#include "print.h"
#include "dsd_support.h"

on tile[0] : out port p_gpio = XS1_PORT_8C;

#ifndef IAP
/* If IAP not enabled, i2c ports not declared - still needs for DAC config */
on tile [0] : struct r_i2c r_i2c = {XS1_PORT_4A};
#else
extern struct r_i2c r_i2c;
#endif


void wait_us(int microseconds)
{
    timer t;
    unsigned time;

    t :> time;
    t when timerafter(time + (microseconds * 100)) :> void;
}

void AudioHwInit(chanend ?c_codec)
{
    p_gpio_peek();
    p_gpio_out(0);

    /* Init the i2c module */
    i2c_shared_master_init(r_i2c);

    xprintf("AudioHWInit\n");

}

/* Configures the external audio hardware for the required sample frequency.
 * See gpio.h for I2C helper functions and gpio access
 */
void AudioHwConfig(unsigned samFreq, unsigned mClk, chanend ?c_codec, unsigned dsdMode,
    unsigned sampRes_DAC, unsigned sampRes_ADC)
{
    xprintf("AudioHwConfig samFreq=%d, DAC=%d, dsd=%d, ",samFreq,sampRes_DAC,dsdMode);


    /* Put ADC and DAC into reset */
	set_gpio(P_GPIO_ADC_RST_N, 0);
	set_gpio(P_GPIO_DAC_RST_N, 0);

    /* Set master clock select appropriately */

    if (mClk == MCLK_441)
    {
        set_gpio(P_GPIO_MCLK_24M, 0);
        wait_us(10);
        set_gpio(P_GPIO_MCLK_22M, 1);
        xprintf("22MHZ\n");
    }
    else
    {
        set_gpio(P_GPIO_MCLK_22M, 0);
        wait_us(10);
        set_gpio(P_GPIO_MCLK_24M, 1);
        xprintf("24MHZ\n");
    }

    /* Allow MCLK to settle */
    wait_us(20000); // 20ms

    if((dsdMode == DSD_MODE_NATIVE) || (dsdMode == DSD_MODE_DOP))
    {
        /* Enable DSD 8ch out mode on mux */
        set_gpio(P_GPIO_DSD_MODE, 1);

        /* DAC out out reset, note ADC left in reset in for DSD mode */
        set_gpio(P_GPIO_DAC_RST_N, 1);

    }
    else
    {
        /* dsdMode == 0 */
        set_gpio(P_GPIO_DSD_MODE, 0);

        /* Take ADC out of reset */
        set_gpio(P_GPIO_ADC_RST_N, 1);

#ifdef CODEC_MASTER
        /* Allow some time for clocks from ADC to become stable */
        wait_us(500);
#endif

        set_gpio(P_GPIO_DAC_RST_N, 1);//De-assert DAC reset

        wait_us(500);
    }
    return;
}
//:
