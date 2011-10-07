#ifndef AVB_DFU_H_
#define AVB_DFU_H_

int avb_dfu_device_reboot(void);

int avb_dfu_init(void);

int avb_dfu_deinit(void);

int avb_dfu_data_block_write(unsigned char *data, int num_bytes);

int avb_dfu_image_complete(void);


#endif /* AVB_DFU_H_ */
