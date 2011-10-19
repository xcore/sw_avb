#ifndef AVB_FLASH_INTERFACE_H_
#define AVB_FLASH_INTERFACE_H_

#ifndef FLASH_MAX_UPGRADE_SIZE
#define FLASH_MAX_UPGRADE_SIZE 128 * 1024 // 128K default
#endif

#ifndef AVB_DFU_TRAP_ON_FLASH_ERROR
#define AVB_DFU_TRAP_ON_FLASH_ERROR 0
#endif

/** Sets up the SPI ports and connects to the flash.
 *
 * 	\return 	0 on success, non-zero on failure.
 *
 **/
int flash_cmd_enable_ports(void);

/** Disables the SPI ports and disconnects the flash.
 *
 *	\return 	0 on success, non-zero on failure.
 *
 **/
int flash_cmd_disable_ports(void);

#endif /* AVB_FLASH_INTERFACE_H_ */
