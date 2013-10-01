#include "avb.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_aecp.h"
#include <string.h>
#include <print.h>
#include "simple_printf.h"
#include "xccompat.h"
#include "avb_flash.h"

#define FLASH_SIZE (NUM_PAGES * PAGE_SIZE)
#define NUM_SECTORS (FLASH_SIZE / SECTOR_SIZE)

static unsigned write_address = 0;
static unsigned write_offset = 0;

static unsigned int sortbits(unsigned int bits)
{
  return( byterev(bitrev(bits)) );
}

static int fl_get_sector_address(int sectorNum)
{
  return SECTOR_SIZE * sectorNum;
}

/**
 * Returns the number of the first sector starting at or after the specified
 * address.
 * \return The number of sector or -1 if there is no such sector.
 */
static int fl_get_sector_at_or_after(unsigned address)
{
  unsigned sector;
  for (sector = 0; sector < NUM_SECTORS; sector++) {
    if (fl_get_sector_address(sector) >= address)
      return sector;
  }
  return -1;
}

static int fl_get_next_boot_image(fl_boot_image_info* boot_image_info)
{
  unsigned tmpbuf[6];
  unsigned last_address = boot_image_info->startAddress+boot_image_info->size;
  unsigned sector_num = fl_get_sector_at_or_after(last_address);
  if (sector_num < 0)
    return 1;
  while (sector_num < NUM_SECTORS) {
    unsigned sector_address = fl_get_sector_address(sector_num);
    spi_flash_read(sector_address, (char*)tmpbuf, 6 * sizeof(int));
    if (sortbits(tmpbuf[0]) == IMAGE_TAG_13) {
      boot_image_info->startAddress = sector_address;
      boot_image_info->size         = sortbits(tmpbuf[IMAGE_LENGTH_OFFSET_13]);
      boot_image_info->version      = sortbits(tmpbuf[IMAGE_VERSION_OFFSET_13]);
      boot_image_info->factory      = 0;
      return 0;
    }
    sector_num++;
  }
  return 1;
}

static int get_factory_image(fl_boot_image_info* boot_image_info)
{
  unsigned tmpbuf[9];
  spi_flash_read(0, (char*)tmpbuf, 4);
  unsigned start_addr = (sortbits(tmpbuf[0])+2)<<2; /* Normal case. */
  spi_flash_read(start_addr, (char*)tmpbuf, (6 + 3) * sizeof(int));
  unsigned *header = tmpbuf;
  if (sortbits(tmpbuf[0]) != IMAGE_TAG_13) {
    return 1;
  }
  boot_image_info->startAddress = start_addr;
  boot_image_info->size         = sortbits(header[IMAGE_LENGTH_OFFSET_13]);  /* Size is to next sector start. */
  boot_image_info->version      = sortbits(header[IMAGE_VERSION_OFFSET_13]);
  boot_image_info->factory      = 1;
  return 0;
}

static void write_and_update_address(unsigned char data[PAGE_SIZE]) {
    simple_printf("Wrote offset %d at %x \n", write_offset, write_address);
    spi_flash_write_small(write_address, data, PAGE_SIZE);
    write_address += PAGE_SIZE;
    write_offset += PAGE_SIZE;
}

static void erase_sectors(unsigned int image_size) {
    unsigned int sector_address = write_address;
    do {
      spi_flash_erase(sector_address, SECTOR_SIZE);
      simple_printf("Erased sector %x\n", sector_address);
      sector_address += SECTOR_SIZE;
    } while(sector_address < write_address + image_size);
}

int avb_write_upgrade_image_page(int address, unsigned char data[PAGE_SIZE]) {
  fl_boot_image_info image;

  if (address == 0) {
    write_address = 0;
    write_offset = 0;
    if (get_factory_image(&image) != 0) {
      printstrln("No factory image!");
      return 1;
    } else {
      if (fl_get_next_boot_image(&image) != 0) {
        // No upgrade image exists, add one
        printstrln("No upgrade");
        unsigned sectorNum = fl_get_sector_at_or_after(image.startAddress + image.size);
        write_address = fl_get_sector_address(sectorNum);

        erase_sectors(MAX_UPGRADE_IMAGE_SIZE);

        write_and_update_address(data);
      }
      else {
        // Replace the upgrade image
        printstrln("Upgrade exists");
        write_address = image.startAddress;

        erase_sectors(image.size);

        write_and_update_address(data);
      }
    }
  }
  else if (address == write_offset) {
    write_and_update_address(data);
  }

  return 0;
}