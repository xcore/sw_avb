#include <platform.h>
// All the port defines for this board are set in the target description
// XN file so there is not much to do here

#define USING_XMOS_DEV_BOARD    1

// These value are got from the ethernet board support module
#define ETHERNET_MII_INIT_full  ETHERNET_DEFAULT_MII_INIT_full
#define ETHERNET_SMI_INIT       ETHERNET_DEFAULT_SMI_INIT

#define AVB_XA_SK_AUDIO_SLICE   1

#define PLL_TYPE_CS2100         1

// CODEC and PLL control ports
#define I2C_COMBINE_SCL_SDA     1

#define AVB_I2C_TILE            0
#define AVB_GPIO_TILE           0

// 1V0 AUDIO SLICE

#define PORT_I2C                on tile[AVB_I2C_TILE]:XS1_PORT_4F

// PLL frequency control
#define PORT_SYNC_OUT           on tile[0]:XS1_PORT_1P

// I2S ports
#define PORT_MCLK               on tile[0]:XS1_PORT_1E
#define PORT_SCLK               on tile[0]:XS1_PORT_1A
#define PORT_LRCLK              on tile[0]:XS1_PORT_1H

#define PORT_SDATA_OUT          {on tile[0]:XS1_PORT_1D,on tile[0]:XS1_PORT_1M}

#define PORT_SDATA_IN           {on tile[0]:XS1_PORT_1I,on tile[0]:XS1_PORT_1L}

#define PORT_BUTTONS            on tile[AVB_GPIO_TILE]:XS1_PORT_4C

#define PORT_MUTE_LED_REMOTE    on tile[AVB_GPIO_TILE]:XS1_PORT_4A

#define PORT_LEDS               on tile[AVB_GPIO_TILE]:XS1_PORT_4B

#define PORT_AUDIO_SHARED       on tile[0]: XS1_PORT_4E