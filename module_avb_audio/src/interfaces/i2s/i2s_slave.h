// Copyright (c) 2011, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

///////////////////////////////////////////////////////////////////////////////
//
// Multichannel I2S_SLAVE slave receiver-transmitter

#ifndef _I2S_SLAVE_H_
#define _I2S_SLAVE_H_

#include "media_fifo.h"

// number of input and output ports, each carries two channels of audio
#ifndef I2S_SLAVE_NUM_IN
#define I2S_SLAVE_NUM_IN 1
#endif


/* 0: No debug prints, 1: debug prints */
//#define I2S_DEBUG	1

//void i2s_slave(struct i2s_slave &r_i2s_slave, media_input_fifo_t ?input_fifos[] );
void i2s_slave(const clock mclk,
				clock bclk,
				in port p_bclk,
				in port p_lrclk,
				in buffered port:32 p_din[],
				int num_in,
				media_input_fifo_t ?input_fifos[],
				chanend media_ctl,
				int clk_ctl_index);

#endif // _I2S_SLAVE_H_
