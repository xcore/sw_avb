#ifndef AVB_DFU_H_
#define AVB_DFU_H_

/** A call to this function will cause a reset of the XCore(s).
 *
 *	The application can use this function to force the device to
 *	reboot and load the upgrade image, if valid.
 *
 *	\return		0 on success, but the device will probably have
 *				reset before this
 *
 **/
int avb_dfu_device_reboot(void);

/** Initialises the flash interface by enabling the SPI ports, opening
 * 	a connection to it and then preparing it to receive a new image.
 *
 *	The application must call and receive success return from this function
 *	before any call to avb_dfu_data_block_write().
 *
 * 	\return		0 on success, non-zero on failure
 **/
int avb_dfu_init(void);

/**	The application can write an upgrade image to flash by calling this
 * 	function for each subsequent block of image data.
 *
 * 	NOTE: the block size (num_bytes) must be >= to the flash page size,
 * 	apart from the very final block of the image, which may be less.
 *
 * 	\param data			A pointer to the data you wish to write to flash
 * 	\param num_bytes	The number of bytes to write (see note above)
 * 	\return				0 on success, non-zero on failure
 *
 **/
int avb_dfu_data_block_write(unsigned char *data, int num_bytes);

/**	The application must call this function when the last data block
 * 	of the image has been written to flash via avb_dfu_data_block_write().
 *
 * 	It ensures that the SPI device has written the last page of data to
 * 	its memory.
 *
 * 	\return 	0 on success, non-zero on failure.
 *
 **/
int avb_dfu_image_complete(void);

/**	Closes the connection to the flash and disables the SPI ports.
 *
 *	\return 	0 on success, non-zero on failure.
 *
 **/
int avb_dfu_deinit(void);


#endif /* AVB_DFU_H_ */
