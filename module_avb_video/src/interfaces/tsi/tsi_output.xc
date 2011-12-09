#include <xs1.h>
#include <platform.h>

#include "tsi_output.h"
#include "avb_conf.h"

#if AVB_NUM_MEDIA_OUTPUTS > 0

#pragma xta command "remove exclusion *"
#pragma xta command "analyze endpoints ts_spi_output_first ts_spi_output_loop"
#pragma xta command "set required - 148 ns"

#pragma xta command "remove exclusion *"
#pragma xta command "add exclusion ts_spi_output_loop"
#pragma xta command "add exclusion ts_spi_output_no_data"
#pragma xta command "analyze endpoints ts_spi_output_loop ts_spi_output_first"
#pragma xta command "set required - 148 ns"


#pragma unsafe arrays
void tsi_output(clock clk, out buffered port:32 p_data, in port p_clk, out buffered port:4 p_sync, out port p_valid,
		media_output_fifo_data_t& ofifo)
{
	unsigned rd_ptr = 0;
	timer t;

	// Intialise port, clearbufs and start clock last
	configure_clock_src(clk, p_clk);
	configure_out_port_strobed_master(p_data, p_valid, clk, 0);

	clearbuf(p_data);
	clearbuf(p_sync);

	start_clock(clk);


	while (1) {
#pragma xta label "ts_spi_output_no_data"

		// Wait for the next packet
		while (ofifo.fifo[rd_ptr+MEDIA_OUTPUT_FIFO_INUSE_OFFSET])
		{
			// Wait until it is time to transmit the packet
			unsigned ts = ofifo.fifo[rd_ptr];
			rd_ptr++;
			//t when timerafter (ts) :> void;

			// Transmit first word
			sync(p_data);
#pragma xta endpoint "ts_spi_output_first"
			p_sync <: 1;
			p_data <: ofifo.fifo[rd_ptr];
			rd_ptr++;

#pragma loop unroll
			for (unsigned i=0; i<46; i++) {
#pragma xta endpoint "ts_spi_output_loop"
				p_sync <: 0;
				p_data <: ofifo.fifo[rd_ptr];
				rd_ptr++;
			}

			ofifo.fifo[rd_ptr] = 0;
			rd_ptr++;

			// In the XMOS architecture, the various test instructions
			// return a value of 1 or 0 in a register, therefore the
			// result of a comparison like '<' is 1 or 0, and therefore
			// can be safely used as it is below
			rd_ptr *= (rd_ptr < MEDIA_OUTPUT_FIFO_WORD_SIZE);
		}
	}
}

#endif

