#include <xs1.h>
#include <xclib.h>
#include <platform.h>
#include <flash.h>
#include <print.h>
#include "avb_flash_interface.h"
#include "avb_dfu.h"

#if 1
#define FLASH_ERROR() do { printstr("Error: line: "); printintln(__LINE__); __builtin_trap(); } while(0)
#else
#define FLASH_ERROR() do {} while(0)
#endif

static int flash_device_open = 0;
static fl_BootImageInfo factory_image;
static fl_BootImageInfo upgrade_image;
static int upgrade_image_valid = 0;

static int flash_page_size = 0;

int write_sswitch_reg_blind(unsigned coreid, unsigned reg, unsigned data);

int avb_dfu_device_reboot(void)
{
	unsigned int pllVal;
	unsigned int core_id = get_core_id();
#ifdef ARCH_G
	read_sswitch_reg(core_id, 6, x);
	write_sswitch_reg(core_id, 6, x);
#else
	read_sswitch_reg(core_id, 6, &pllVal);
	write_sswitch_reg_blind(core_id^0x8000, 6, pllVal);
	write_sswitch_reg_blind(core_id, 6, pllVal);
#endif
	return 0;
}

int avb_dfu_image_complete(void)
{
	if (fl_endWriteImage() != 0)
	{
		FLASH_ERROR();
		return 1;
	}
	else
	{
		return 0;
	}
}

int avb_dfu_data_block_write(unsigned char *data, int num_bytes)
{
	int i;
	int p = 0;

	if (flash_page_size == 0)
	{
		FLASH_ERROR();
		return 1;
	}

	for (i=0; i < num_bytes/flash_page_size; i++)
	{
		int result = fl_writeImagePage(&data[p]);

		if (result)
		{
			// Error
			FLASH_ERROR();
			return 1;
		}

		p += flash_page_size;
	}

	return 0;
}

/*
 * Sets the SPI ports of the flash, opens a connection to it, then prepares it to receive
 * a new image.
 */
int avb_dfu_init(void)
{
	int result;
    fl_BootImageInfo image;

    if (!flash_device_open)
    {
        if (flash_cmd_enable_ports() != 0)
        {
            // Something went wrong when trying to connect to the flash
        	FLASH_ERROR();
            return 1;
        }
        else
        {
        	flash_device_open = 1;
        }
    }

    flash_page_size = fl_getPageSize();

    // Check that the factory image is valid
    if (fl_getFactoryImage(&image) != 0)
    {
    	FLASH_ERROR();
        return 1;
    }
    factory_image = image;

    // Check if a valid upgrade image already exists
    if (fl_getNextBootImage(&image) == 0)
    {
        upgrade_image_valid = 1;
        upgrade_image = image;
    }

    do
    {
    	// If an upgrade image already exists, replace it with the new one
    	if (!upgrade_image_valid)
    	{
    		result = fl_startImageAdd(&factory_image, FLASH_MAX_UPGRADE_SIZE, 0);
    	}
    	else
    	{
    		result = fl_startImageReplace(&upgrade_image, FLASH_MAX_UPGRADE_SIZE);
    	}

    } while (result > 0);

	if (result < 0)
	{
		// Failed
		FLASH_ERROR();
		return 1;
	}

    return 0;
}

int avb_dfu_deinit(void)
{
	int result;
    if (!flash_device_open) return 0;

    result = flash_cmd_disable_ports();

    if (!result)
    {
    	flash_device_open = 0;
    	return 0;
    }
    else
    {
    	return 1;
    }
}
