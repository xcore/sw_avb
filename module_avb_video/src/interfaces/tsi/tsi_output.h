/*
 * MPEG Transport stream synchronous parallel interface
 */
#ifndef __AVB_VIDEO_TSI_OUTPUT_H__
#define __AVB_VIDEO_TSI_OUTPUT_H__

#include "media_fifo.h"

/*
 *  The TSI interface consists of 8 bits of data, a clock, a valid, and a sync.
 */

/**
 *  \brief Hardware interface for the MPEG2 synchronous parallel interface
 *
 *  \param clk the clock block for the output
 *  \param p_data MPEG data, an 8 bit port to be driven as a 32 bit buffered port
 *  \param p_clk a one bit clock input with the 23MHz MPEG TSI clock
 *  \param p_sync MPEG start of packet but, a one bit port to be driven as a 4 bit buffered port
 *  \param p_valid Valid bit for the data and sync bits. A one bit pin
 *  \param output_fifo The media FIFO from which to draw samples
 */
void tsi_output(clock clk, out buffered port:32 p_data, in port p_clk, out buffered port:4 p_sync, out port p_valid,
		ofifo_t& output_fifo);

#endif

