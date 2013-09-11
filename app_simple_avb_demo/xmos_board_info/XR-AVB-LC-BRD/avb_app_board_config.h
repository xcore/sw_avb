#include <platform.h>
// All the port defines for this board are set in the target description
// XN file so there is not much to do here

#define ENABLE_XSCOPE           1

#define USING_XMOS_DEV_BOARD    1
#define PORT_SDATA_IN           {PORT_SDATA_IN0, PORT_SDATA_IN1, PORT_SDATA_IN2, PORT_SDATA_IN3 }
#define PORT_SDATA_OUT          {PORT_SDATA_OUT0, PORT_SDATA_OUT1, PORT_SDATA_OUT2, PORT_SDATA_OUT3 }

#define PORT_BUTTONS            PORT_SHARED_IN
#define PORT_MUTE_LED_REMOTE    PORT_SHARED_OUT

// These value are got from the ethernet board support module
#define ETHERNET_MII_INIT_full  ETHERNET_DEFAULT_MII_INIT_full
#define ETHERNET_SMI_INIT       ETHERNET_DEFAULT_SMI_INIT

#define PLL_TYPE_CS2300         1
#define PLL_AUX_OUTPUT_IS_MCLK  0

#define I2C_COMBINE_SCL_SDA     0

#define AVB_GPIO_ENABLED        1
#define AVB_I2C_TILE            1
#define AVB_GPIO_TILE           1
#define AVB_AUDIO_TILE          0
#define AVB_CONTROL_TILE        1

/** Add sine wave synthesis from channels ``I2S_SYNTH_FROM*2`` upwards in the I2S component */
#define I2S_SYNTH_FROM          1
