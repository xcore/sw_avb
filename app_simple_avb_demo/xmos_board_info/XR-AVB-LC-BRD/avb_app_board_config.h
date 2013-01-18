#include <platform.h>
// All the port defines for this board are set in the target description
// XN file so there is not much to do here

#define USING_XMOS_DEV_BOARD 1
#define PORT_SDATA_IN {PORT_SDATA_IN0, PORT_SDATA_IN1, PORT_SDATA_IN2, PORT_SDATA_IN3 }
#define PORT_SDATA_OUT {PORT_SDATA_OUT0, PORT_SDATA_OUT1, PORT_SDATA_OUT2, PORT_SDATA_OUT3 }

#define PORT_BUTTONS PORT_SHARED_IN
#define PORT_MUTE_LED_REMOTE PORT_SHARED_OUT

// These value are got from the ethernet board support module
#define ETHERNET_MII_INIT_full ETHERNET_DEFAULT_MII_INIT_full
#define ETHERNET_SMI_INIT ETHERNET_DEFAULT_SMI_INIT


