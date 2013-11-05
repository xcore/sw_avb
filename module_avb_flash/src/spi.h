#ifndef SPI_H
#define SPI_H

#ifdef __spi_conf_h_exists__
#include "spi_conf.h"
#endif

#define SPI_CMD_WRITE_ENABLE   0x06    /**< SPI command to enable write and erase operations */
#define SPI_CMD_WRITE_DISABLE  0x04
#define SPI_CMD_WRITE          0x02
#define SPI_CMD_READ           0x03
#define SPI_CMD_READSR         0x05
#define SPI_CMD_READID         0x9f

#ifndef SPI_CMD_ERASE
#define SPI_CMD_ERASE          0x20
#endif

#ifndef SPI_SECTOR_SIZE
#define SPI_SECTOR_SIZE        4096
#endif

#ifndef SPI_CLK_MHZ
#define SPI_CLK_MHZ              25
#endif

#ifdef __XC__
interface spi_interface {
    /** This function issues a single command without parameters to the SPI,
     * and reads up to 4 bytes status value from the device.
     *
     * \param cmd        command value - listed above
     *
     * \param returnBytes The number of bytes that are to be read from the
     *                    device after the command is issued. 0 means no bytes
     *                    will be read.
     *
     * \returns the read bytes, or zero if no bytes were requested. If multiple
     * bytes are requested, then the last byte read is in the least-significant
     * byte of the return value.
     */
    int command_status(int cmd, unsigned returnBytes);

    /** This function issues a single command with a 3-byte address parameter
     * and an optional data-set to be output to or input form the device.
     *
     * \param cmd        command value - listed above
     *
     * \param address    the address to send to the SPI device. Only the least
     *                   significant 24 bits are used.
     *
     * \param data       an array of data that contains either data to be written to
     *                   the device, or which is used to store that that is
     *                   read from the device.
     *
     * \param returnBytes If positive, then this is the number of bytes that
     *                    are to be read from the device, into ``data``. If
     *                    negative, then this is (minus) the number of bytes to
     *                    be written to the device from ``data``. 0 means no
     *                    bytes will be read or written.
     *
     */
    void command_address_status(int cmd, unsigned int address, unsigned char data[returnBytes], unsigned returnBytes);
};

[[distributable]]
void spi_task(server interface spi_interface i_spi);
#endif

#endif
