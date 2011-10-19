#include <print.h>
#include "tftp_app.h"
#include "avb_dfu.h"

// #define TFTP_DUMMY_TRANSFER

int tftp_app_transfer_begin(void)
{
	printstrln("TFTP DFU: Image transfer starting...");
#ifndef TFTP_DUMMY_TRANSFER
	return avb_dfu_init();
#else
	return 0;
#endif

}

int tftp_app_process_data_block(unsigned char *data, int num_bytes)
{
	/* num_bytes should always be 512 bytes apart from the final data block,
	 * which is always < 512 */
#ifndef TFTP_DUMMY_TRANSFER
	return avb_dfu_data_block_write(data, num_bytes);
#else
	return 0;
#endif

}

void tftp_app_transfer_complete(void)
{
	printstrln("TFTP DFU: Image transfer complete.");
#ifndef TFTP_DUMMY_TRANSFER
	avb_dfu_image_complete();

	avb_dfu_deinit();
#endif

	printstrln("TFTP DFU: Rebooting device...");
	avb_dfu_device_reboot();
}

void tftp_app_transfer_error(void)
{
	printstrln("TFTP DFU: Transfer error");
}
