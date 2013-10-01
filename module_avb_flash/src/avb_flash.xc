#include "avb_flash.h"
#include "flash_defaults.h"

void spi_flash_write_small(unsigned int address, unsigned char data[],int bytes) {
    spi_command_status(SPI_CMD_WRITE_ENABLE, 0);
    spi_command_address_status(SPI_CMD_WRITE,address,data,-(bytes));
    while(spi_command_status(SPI_CMD_READSR, 1) & 1) {
        ;
    }
    spi_command_status(SPI_CMD_WRITE_DISABLE, 0);
}

void spi_flash_erase(unsigned int address, int bytes) {
    char data[1];
    while (bytes > 0) {
        spi_command_status(SPI_CMD_WRITE_ENABLE, 0);
        spi_command_address_status(SPI_CMD_ERASE, address, data, 0);
        bytes -= SPI_SECTOR_SIZE;
        address += SPI_SECTOR_SIZE;
        while(spi_command_status(SPI_CMD_READSR, 1) & 1) {
            ;
        }
    }
    spi_command_status(SPI_CMD_WRITE_DISABLE, 0);
}

static int flashStartAddress = 0;

#define VALID    0x0F
#define INVALID  0x00

/*
 * Persistent data is stored in flash between addresses
 * FLASH_PERSISTENT_BASE and FLASH_PERSISTENT_BASE +
 * FLASH_PERSISTENT_SEGMENT_SIZE. Each data item is always a power of 2
 * long, and the last byte is a guard byte which is one of 0xFF (unused),
 * 0x0F (valid) or 0x00 (no longer valid).
 *
 * This function finds the first valid persistent data in flash. This is
 * probably the most recent state, unless the syste got switched off prior
 * to it being made invalid, in which case this data is still valid but
 * just one step old.
 */
int spi_flash_persistent_state_read(unsigned char data[]) {
    char guard[1];
    for(int i = flashStartAddress;
        i < flashStartAddress + FLASH_PERSISTENT_SEGMENT_SIZE; 
        i+= FLASH_PERSISTENT_SIZE + 1) {
        int index = (i&(FLASH_PERSISTENT_SEGMENT_SIZE-1));
        spi_flash_read(FLASH_PERSISTENT_BASE + index + FLASH_PERSISTENT_SIZE, guard, 1);
        if (guard[0] == VALID) {
            spi_flash_read(FLASH_PERSISTENT_BASE + index, data, FLASH_PERSISTENT_SIZE);
            flashStartAddress = index;
            return 1;
        }
    }
    return 0;
}

/*
 * This function writes persistent data by first finding valid persistent
 * data. If it cannot find any it will commence writing from the start
 * (default writeIndex is 0), otherwise writeIndex is set to be immediately
 * after the valid block. If the writeIndex is at the start of a sector,
 * then the sector is erased. The data is written, the guard byte for the
 * data is set to valid (in a separate write, that guarantees that the data
 * write has completed before the guard byte write is completed), and
 * finally the old guard byte is cleared.
 *
 * This process can be interrupted at the following places without disruption:
 *
 *   After erase - it will find the old block (there are always at least
 *   two sectors)
 *
 *   After write of data - the guard will not be written, so it will not
 *   find the next one valid
 *
 *   After the guard is written - a read will still find the old data,
 *   leading to this being erased again
 *
 *   After the old guard is cleared - this will now have progressed the
 *   state.
 *
 * If interrupt during erase, there is a chance that any guard is set to
 * 'valid'. This code cannot deal with it, but a full and large version
 * would put a CRC over the data to validate the guard.
 */
void spi_flash_persistent_state_write(unsigned char data[]) {
    char guard[1];
    int i, writeIndex = 0, clearIndex = FLASH_PERSISTENT_SEGMENT_SIZE - FLASH_PERSISTENT_SIZE;
    for(i = flashStartAddress;
        i < flashStartAddress + FLASH_PERSISTENT_SEGMENT_SIZE; 
        i+= FLASH_PERSISTENT_SIZE + 1) {
        int readIndex = (i&(FLASH_PERSISTENT_SEGMENT_SIZE-1));
        spi_flash_read(FLASH_PERSISTENT_BASE + readIndex + FLASH_PERSISTENT_SIZE, guard, 1);
        if (guard[0] == VALID) {
            clearIndex = readIndex;
            writeIndex = (readIndex+FLASH_PERSISTENT_SIZE+1)&(FLASH_PERSISTENT_SEGMENT_SIZE-1);
            break;
        }
    }
    if ((writeIndex & (FLASH_PERSISTENT_SECTOR_SIZE -1)) == 0) {
        spi_flash_erase(FLASH_PERSISTENT_BASE + writeIndex, FLASH_PERSISTENT_SECTOR_SIZE);
    }
    spi_flash_write_small(FLASH_PERSISTENT_BASE + writeIndex, data, 15);
    guard[0] = VALID;
    spi_flash_write_small(FLASH_PERSISTENT_BASE + writeIndex + FLASH_PERSISTENT_SIZE, guard, 1);
    guard[0] = INVALID;
    spi_flash_write_small(FLASH_PERSISTENT_BASE + clearIndex + FLASH_PERSISTENT_SIZE, guard, 1);
    flashStartAddress = writeIndex;
}
