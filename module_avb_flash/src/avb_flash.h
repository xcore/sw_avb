#ifndef FLASH_H
#define FLASH_H

#include "spi.h"

/** Struct describing a bootable image. */
typedef struct {
  unsigned startAddress; /**< The address of the start of the image. */
  unsigned size; /**< The size in bytes of the image. */
  unsigned version; /**< The image version. */
  int factory; /**< 1 if the image is the factory image, 0 otherwise. */
} fl_boot_image_info;

#define IMAGE_TAG_13 0x0FF51DE
#define IMAGE_HEADER_SIZE_13 44       //Size of header bytes(11 words)

#define IMAGE_TAG_OFFSET_13 0
#define PAGE_CRC_OFFSET_13 1
#define IMAGE_CRC_OFFSET_13 2
#define IMAGE_FEATURES_OFFSET_13 3
#define IMAGE_SIZE_OFFSET_13 4
#define IMAGE_VERSION_OFFSET_13 5
#define IMAGE_LENGTH_OFFSET_13 6
#define NUM_CORES_OFFSET_13 7


/** This function reads a block of data from the flash at the given
 * address.

 * \param address the address to send to the SPI device. Only the least
 *                significant 24 bits are used.
 *
 * \param data    an array of data to which data is to be written from
 *                the flash device.
 *
 * \param bytes   The number of bytes that are to be read from the device
 *                into ``data``.
 *
 */
void spi_flash_read(client interface spi_interface i_spi, unsigned int address, unsigned char data[], int bytes);

/** This function writes a small block of data to the flash at the given
 * address. "Small" means that all writes must happen in the same 256 byte
 * page. In other words, (address + data & 0xff) must equal (address &
 * 0xff). A call can be made to spiFlashWrite() to write arbitrary size
 * data to arbitrary places. A write to flash can only change bits form '1'
 * to '0'. A call can be made to spiFlashErase() to set a whole sector of
 * the flash to all '1'.

 * \param address the address to send to the SPI device. Only the least
 *                significant 24 bits are used.
 *
 * \param data    an array of data that contains the data to be written to
 *                the flash device.
 *
 * \param bytes   The number of bytes that are to be written to the device
 *                from ``data``.
 *
 */
void spi_flash_write_small(client interface spi_interface i_spi, unsigned int address, unsigned char data[],int bytes);

/** This function erases a block of data in the flash at the given address.
 * This will replace the block with all '1' bits. The address should be
 * aligned on a SPI_SECTOR_SIZE boundary, and the length should be a whole
 * number of SPI_SECTOR_SIZE bytes.
 *
 * \param address the address to send to the SPI device. Only the least
 *                significant 24 bits are used.
 *
 * \param bytes   The number of bytes that are to be erased.
 *
 */
void spi_flash_erase(client interface spi_interface i_spi, unsigned int address, int bytes);

#endif
