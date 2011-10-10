#include <xs1.h>
#include <xclib.h>
#include <platform.h>
#include <flash.h>
#include <print.h>
#include "avb_flash_interface.h"

#define settw(a,b) {__asm__ __volatile__("settw res[%0], %1": : "r" (a) , "r" (b));}
#define setc(a,b) {__asm__  __volatile__("setc res[%0], %1": : "r" (a) , "r" (b));}
#define setclk(a,b) {__asm__ __volatile__("setclk res[%0], %1": : "r" (a) , "r" (b));}
#define portin(a,b) {__asm__  __volatile__("in %0, res[%1]": "=r" (b) : "r" (a));}
#define portout(a,b) {__asm__  __volatile__("out res[%0], %1": : "r" (a) , "r" (b));}

extern fl_SPIPorts p_flash;

fl_DeviceSpec flash_devices[] = { FL_DEVICE_WINBOND_W25X40, FL_DEVICE_ATMEL_AT25DF041A };
// fl_DeviceSpec flash_devices[] = {FL_DEVICE_ATMEL_AT25DF041A};

int flash_cmd_enable_ports(void)
{
	int result = 0;
	setc(p_flash.spiMISO, XS1_SETC_INUSE_OFF);
	setc(p_flash.spiCLK, XS1_SETC_INUSE_OFF);
	setc(p_flash.spiMOSI, XS1_SETC_INUSE_OFF);
	setc(p_flash.spiSS, XS1_SETC_INUSE_OFF);

	setc(p_flash.spiMISO, XS1_SETC_INUSE_ON);
	setc(p_flash.spiCLK, XS1_SETC_INUSE_ON);
	setc(p_flash.spiMOSI, XS1_SETC_INUSE_ON);
	setc(p_flash.spiSS, XS1_SETC_INUSE_ON);

	setclk(p_flash.spiMISO, XS1_CLKBLK_REF);
	setclk(p_flash.spiCLK, XS1_CLKBLK_REF);
	setclk(p_flash.spiMOSI, XS1_CLKBLK_REF);
	setclk(p_flash.spiSS, XS1_CLKBLK_REF);

	setc(p_flash.spiMISO, XS1_SETC_BUF_BUFFERS);
	setc(p_flash.spiMOSI, XS1_SETC_BUF_BUFFERS);

	settw(p_flash.spiMISO, 8);
	settw(p_flash.spiMOSI, 8);

	result = fl_connectToDevice(&p_flash, flash_devices, sizeof(flash_devices) / sizeof(fl_DeviceSpec));
	// result = fl_connect(&p_flash);

	if (result) return 1;
	else return 0;
}

int flash_cmd_disable_ports(void)
{
	int result = fl_disconnect();

	setc(p_flash.spiMISO, XS1_SETC_INUSE_OFF);
	setc(p_flash.spiCLK, XS1_SETC_INUSE_OFF);
	setc(p_flash.spiMOSI, XS1_SETC_INUSE_OFF);
	setc(p_flash.spiSS, XS1_SETC_INUSE_OFF);

	return result;
}
