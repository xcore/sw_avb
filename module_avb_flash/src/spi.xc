#include <xs1.h>
#include <xclib.h>
#include "spi.h"
#include <platform.h>
#include <stdio.h>

/*
static void spi_init() {
  spiSS <: ~0;
  spiCLK <: ~0;

  configure_clock_src(SPIclock, spiCLK);
  start_clock(SPIclock);

  configure_out_port(spiMOSI, SPIclock, 0);
  configure_in_port(spiMISO, SPIclock);
}
*/

#if (SPI_CLK_MHZ == 25)
#define eightPulses(clk)     { clk <: 0xCCCCCCCC;}
#elif (SPI_CLK_MHZ == 13)
#define eightPulses(clk)     { clk <: 0xF0F0F0F0; clk <: 0xF0F0F0F0;}
#else
#error "Undefined SPI_CLK_MHZ speed  - must be one of 25 or 13"
#endif

static void spi_cmd(int cmd, fl_SPIPorts &spi_ports) {
  spi_ports.spiSS <: 0;
  clearbuf(spi_ports.spiMOSI);
  spi_ports.spiMOSI <: bitrev(cmd<<24);
  eightPulses(spi_ports.spiCLK);
  sync(spi_ports.spiCLK);
}

static int spi_command_status(int cmd, unsigned returnBytes, fl_SPIPorts &spi_ports) {
  int data = 0;
  spi_cmd(cmd, spi_ports);
  clearbuf(spi_ports.spiMISO);
  while(returnBytes--) {
    eightPulses(spi_ports.spiCLK);
    spi_ports.spiMISO :> >> data;
  }
  spi_ports.spiSS <: 1;
  return bitrev(data);
}

[[distributable]]
void spi_task(server interface spi_interface i_spi) {

  // spi_init();

  while (1) {
    select {
      case i_spi.command_status(int cmd, unsigned returnBytes) -> int read_bytes:
        fl_SPIPorts *movable spi_ports = overlay_flash_claim_ports();
        read_bytes = spi_command_status(cmd, returnBytes, *spi_ports);
        overlay_flash_return_ports(move(spi_ports));
        break;
      case i_spi.command_address_status(int cmd, unsigned int addr, unsigned char data[returnBytes], unsigned returnBytes):
        fl_SPIPorts *movable spi_ports = overlay_flash_claim_ports();
        spi_cmd(cmd, *spi_ports);
        addr = bitrev(addr << 8);
        spi_ports->spiMOSI <: >> addr;
        eightPulses(spi_ports->spiCLK);
        spi_ports->spiMOSI <: >> addr;
        eightPulses(spi_ports->spiCLK);
        spi_ports->spiMOSI <: >> addr;
        eightPulses(spi_ports->spiCLK);
        for (int i = 0; i < -returnBytes; i++) {
          spi_ports->spiMOSI <: bitrev(data[i]<<24);
          eightPulses(spi_ports->spiCLK);
        }
        sync(spi_ports->spiCLK);
        clearbuf(spi_ports->spiMISO);
        for (int i = 0; i < returnBytes; i++) {
          int x;
          eightPulses(spi_ports->spiCLK);
          spi_ports->spiMISO :> x;
          data[i] = bitrev(x<<24);
        }
        spi_ports->spiSS <: 1;
        overlay_flash_return_ports(move(spi_ports));
        break;
    }
  }
}
