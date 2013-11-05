#include "avb_flash.h"
#include "flash_defaults.h"

void spi_flash_read(client interface spi_interface i_spi, unsigned int address, unsigned char data[], int bytes) {
  i_spi.command_address_status(SPI_CMD_READ, address, data, bytes);
}

void spi_flash_write_small(client interface spi_interface i_spi,
                           unsigned int address,
                           unsigned char data[],
                           int bytes) {
    i_spi.command_status(SPI_CMD_WRITE_ENABLE, 0);
    i_spi.command_address_status(SPI_CMD_WRITE,address,data,-(bytes));
    while(i_spi.command_status(SPI_CMD_READSR, 1) & 1) {
        ;
    }
    i_spi.command_status(SPI_CMD_WRITE_DISABLE, 0);
}

void spi_flash_erase(client interface spi_interface i_spi, unsigned int address, int bytes) {
    char data[1];
    while (bytes > 0) {
        i_spi.command_status(SPI_CMD_WRITE_ENABLE, 0);
        i_spi.command_address_status(SPI_CMD_ERASE, address, data, 0);
        bytes -= SPI_SECTOR_SIZE;
        address += SPI_SECTOR_SIZE;
        while(i_spi.command_status(SPI_CMD_READSR, 1) & 1) {
            ;
        }
    }
    i_spi.command_status(SPI_CMD_WRITE_DISABLE, 0);
}