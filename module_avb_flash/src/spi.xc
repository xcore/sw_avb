#include <xs1.h>
#include <xclib.h>
#include "spi.h"
#include <platform.h>
#include <stdio.h>

on tile[0]: out port spiSS              = PORT_SPI_SS;
on tile[0]: buffered out port:32 spiCLK = PORT_SPI_CLK;
on tile[0]: buffered in  port:8 spiMISO = PORT_SPI_MISO;
on tile[0]: buffered out port:8 spiMOSI = PORT_SPI_MOSI;

on tile[0]: clock SPIclock = XS1_CLKBLK_1;

void spi_init() {
    spiSS <: ~0;
    spiCLK <: ~0;

    configure_clock_src(SPIclock, spiCLK);
    start_clock(SPIclock);

    configure_out_port(spiMOSI, SPIclock, 0);
    configure_in_port(spiMISO, SPIclock);
}

#if (SPI_CLK_MHZ == 25)
#define eightPulses(clk)     { clk <: 0xCCCCCCCC;} 
#elif (SPI_CLK_MHZ == 13)
#define eightPulses(clk)     { clk <: 0xF0F0F0F0; clk <: 0xF0F0F0F0;} 
#else
#error "Undefined SPI_CLK_MHZ speed  - must be one of 25 or 13"
#endif

static void spi_cmd(int cmd) {
    spiSS <: 0;
    clearbuf(spiMOSI);
    spiMOSI <: bitrev(cmd<<24);
    eightPulses(spiCLK);
    sync(spiCLK);
}

int spi_command_status(int cmd, int returnBytes) {
    int data = 0;
    spi_cmd(cmd);
    clearbuf(spiMISO);
    while(returnBytes--) {
        eightPulses(spiCLK);
        spiMISO :> >> data;
    }
    spiSS <: 1;
    return bitrev(data);
}

void spi_command_address_status(int cmd, unsigned int addr, unsigned char data[], int returnBytes) {
    spi_cmd(cmd);
    addr = bitrev(addr << 8);
    spiMOSI <: >> addr;
    eightPulses(spiCLK);
    spiMOSI <: >> addr;
    eightPulses(spiCLK);
    spiMOSI <: >> addr;
    eightPulses(spiCLK);
    for(int i = 0; i < -returnBytes; i++) {
        spiMOSI <: bitrev(data[i]<<24);
        eightPulses(spiCLK);
    }
    sync(spiCLK);
    clearbuf(spiMISO);
    for(int i = 0; i < returnBytes; i++) {
        int x;
        eightPulses(spiCLK);
        spiMISO :> x;
        data[i] = bitrev(x<<24);
    }
    spiSS <: 1;
}
