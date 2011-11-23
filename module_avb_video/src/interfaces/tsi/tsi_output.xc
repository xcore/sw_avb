#include <platform.h>
#include "tsi_output.h"
#include "avb_conf.h"

void tsi_output(clock clk, out buffered port:32 p_data, in port p_clk, out buffered port:4 p_sync, out port p_valid,
		media_output_fifo_data_t& ofifo)
{
	unsigned rd_ptr = 0;

	// Intialise port, clearbufs and start clock last
	configure_clock_src(clk, p_clk);
	configure_out_port_strobed_master(p_data, p_valid, clk, 0);

	clearbuf(p_data);
	clearbuf(p_sync);

	start_clock(clk);


	while (1) {
		// Wait for the next packet
		while (ofifo.packet_wr != rd_ptr)
		{

		}
	}
}

