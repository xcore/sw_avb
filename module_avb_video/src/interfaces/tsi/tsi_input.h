/*
 * MPEG Transport stream synchronous parallel interface
 */
#ifndef __AVB_VIDEO_TSI_INPUT_H__
#define __AVB_VIDEO_TSI_INPUT_H__

#include "media_fifo.h"

/**
 *  \brief Hardware interface for the MPEG2 synchronous parallel interface
 *
 *  \param clk a clock block for controlling the input
 *  \param p_data MPEG data, an 8 bit port to be driven as a 32 bit buffered port
 *  \param p_clk a one bit clock input with the 23MHz MPEG TSI clock
 *  \param p_sync MPEG start of packet but, a one bit port
 *  \param p_valid Valid bit for the data and sync bits. A one bit pin
 *  \param ififo The media FIFO from which to draw samples
 */
void tsi_input(clock clk, in buffered port:32 p_data, in port p_clk, in buffered port:4 p_sync, in port p_valid,
		ififo_t& ififo);

#endif

