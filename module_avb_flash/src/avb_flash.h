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
void spi_flash_read(int address, char data[],int bytes);
#define spi_flash_read(address,data,bytes) spi_command_address_status(SPI_CMD_READ,address,data,bytes)

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
void spi_flash_write_small(int address, char data[],int bytes);

/** This function writes a block of data to the flash at the given address.
 * A write to flash can only change bits form '1' to '0'. A spiFlashErase()
 * call can be made to set a whole sector of the flash to all '1'.
 *
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
void spi_flash_write(int address, char data[],int bytes);

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
void spi_flash_erase(int address, int bytes);

/** This function reads persistent data from flash memory. The size of hte
 * persistent data, the base address and the segment size are all compile
 * time constants (defined earlier). A not-so-small library can be made
 * that provides generic functionality.
 *
 * This function may take some time to complete - in the worst case it will
 * search the whole persistent segment for valid data. The time taken
 * depends on the persisten segment size and flash speed, but may well be
 * milliseconds. Note that the read position is cached, so subsequent calls
 * to this function will be fast.
 *
 * \param data array in which the persistent data is to be stored.
 *
 * \returns 0 if no valid data could be found. A factory default should be
 * used in this case.
 */
int spi_flash_persistent_state_read(char data[]);

/** This function writes persistent data to flash memory. The size of hte
 * persistent data, the base address and the segment size are all compile
 * time constants (defined earlier). If this function returns, then the
 * next call to spiFlashPersistentStateRead() will return the data that is
 * written in this call.
 *
 * This function may take some time to complete - it will wait for data to
 * be written, and it may have to erase a sector of data. The time taken
 * depends mostly on flash speed, but may well be tens of milliseconds when
 * a sector needs to be erased. A write operation may take a few hundred
 * microseconds, and if spiFlashPersistentStateRead() has not been called
 * then a full search of flash may be necessary.
 *
 * Many calls to this function will wear out the flash memory. A flash
 * memory may wear out after as little as 100,000 cycles, so if this
 * function is called every 200 us, and the segment is 8192 bytes for
 * 15-bytes of data, then the memory will wear out in (8192/(15+1)) *
 * 100,000 * 0.0002 seconds, which is about 3 hours. Calls to this function
 * should therefore be limited (for example to at most once every few
 * seconds), or the persistent segment should be made much larger.
 *
 * Note that there are hypothetical cases where valid data may be found if
 * the erase function is interrupted by a power failure. A CRC check shall
 * be performed on the data if this is an issue for the application.
 *
 * \param data array in which the persistent data to be written is stored.
 */
void spi_flash_persistent_state_write(char data[]);

#endif
