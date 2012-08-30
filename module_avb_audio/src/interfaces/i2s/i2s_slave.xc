// Copyright (c) 2011, XMOS Ltd., All rights reserved
// This software is freely distributable under a derivative of the
// University of Illinois/NCSA Open Source License posted in
// LICENSE.txt and at <http://github.xcore.com/>

#define USE_UNTESTED_I2S_SLAVE
#ifdef USE_UNTESTED_I2S_SLAVE

///////////////////////////////////////////////////////////////////////////////
//
// Multichannel I2S_SLAVE slave receiver-transmitter

/**
 * @file	i2s_slave.xc
 * @author  Andreas Ertel <andreas.ertel@s1nn.com>
 * @version 1.0
 * @date	2011-09-08
 *
 * @section DESCRIPTION
 *
 * Functions to handle i2s input/output with XMOS as a Slave.
 * Modifications to merge the XMOS I2S Slave implementation into the XMOS AVB Reference Design
 */

#include <xs1.h>
#include <xclib.h>
#include "i2s_slave.h"
#include "simple_printf.h"
#include "media_input_fifo.h"
#include "print.h"


/*\brief i2s_slave_loop function
 * 		 Main loop for I2S Slave - Reads at the specific time from din Port
 * 		 and push it to the FIFO
 *
 * \param din     		array of ports to input data from
 * \param p_lrclk   	lrclock (wordclock) input
 * \param input_fifos 	input Fifos
 * \param num_in    	number of input ports
 */

#pragma unsafe arrays
void i2s_slave_loop(in buffered port:32 din[],
					in port p_lrclk,
					media_input_fifo_t ?input_fifos[],
					int num_in
					)
{
  int lr = 0;
  unsigned frame_counter = 0;
  unsigned int timestamp;
  timer tmr;

#if I2S_DEBUG
  printstr("i2s_slave_loop: Starting Main Loop!\n");
#endif

  while (1) {
    int t;

    /* wait for lrclk edge
     * timestamp this edge
     */
    p_lrclk when pinsneq(lr) :> lr @ t;

    /* Get a timestamp from the Timer */
    tmr :> timestamp;

#if I2S_DEBUG
		printstr("Word Clock: ");
		if( lr == 1 )
			printstr("Right");
		if(lr == 0 )
			printstr("Left");
		printstr("\n");
		printstr("Frame Counter: ");
		printint(frame_counter);
		printstr("\n");
		printstr("timestamp: ");
		printint(timestamp);
		printstr("\n ------ \n");
#endif

		/* Set input time for port din
		 * setpt sets port time to the timestamp t + 1 + 23
		 */
#pragma loop unroll
		for (int i = 0; i < num_in; i++) {
			asm("setpt res[%0], %1" :: "r"(din[i]), "r"(t + 24));
		}

		/* Input the Audio Data from the din (I2S) Ports
		 * 4 Inputs
		 * port will capture I2S MSb at t + 1 and LSb at t + 24
		 * bits 0..7 are older than I2S MSb and hence discard them
		 * compiler would insert SETC FULL on DIN input, because it doesn't know about inline SETPT above
		 * hence we need inline IN too
		 */
#pragma loop unroll
		/* Check all inputs */
		for (int k=0;k<num_in;k++)
		{	/* We have only half of the inputs */
			if( k < num_in>>1 )
			{
				  unsigned int sample_in;
				  asm("in %0, res[%1]":"=r"(sample_in):"r"(din[k]));
				  /* Sample is bitreversed */
				  sample_in = bitrev(sample_in); //<< 8
	#if I2S_DEBUG
				  printstr("i2s_slave_loop: Putting Sample into FIFO: ");
				  printhex(sample_in);
				  printstr("\n");
	#endif
				  /* Push sample to FIFO
				   * We push right samples at 	0,2,4,6
				   * and left samples at		1,3,5,7
				   */
				  media_input_fifo_push_sample(input_fifos[lr+k*2],
						  sample_in,
						  timestamp);
			}
	  }
	frame_counter++;
  }
}

/*\brief i2s_slave function
 * 		 Function that handles the i2s Slave thread and inputs/outputs data as a i2s slave
 *
 * \param mclk      	clock block that clocks the system clock of the codec;
 *                  	needs to be configured before the function call
 * \param blck      	clock block that clocks the bit clock; configured
 *                  	within the i2s_master function
 * \param p_bclk    	the port to output the bit clock to
 * \param p_lrclk   	the port to output the word clock to
 * \param p_din     	array of ports to input data from
 * \param num_in    	number of input ports
 * \param input_fifos 	input Fifos
 * \param media_ctl  	Media Control Channel End
 * \param clk_ctl_index the index of the clk_ctl channel array that
 *                      controls the master clock fo the codec
 */
void i2s_slave(const clock mclk,
				clock bclk,
				in port p_bclk,
				in port p_lrclk,
				in buffered port:32 p_din[],
				int num_in,
				media_input_fifo_t ?input_fifos[],
				chanend media_ctl,
				int clk_ctl_index)
{
	media_ctl_register(media_ctl, num_in, input_fifos, 0, null, clk_ctl_index);


#if I2S_DEBUG
	printstr("Setting Clocks!\n");
#endif

	/* Set clock uses port p_bclk to clock bclk */
	  set_clock_src(bclk, p_bclk);

	  /* p_lrclk will be clocked and sampled by bclk */
	  set_port_clock(p_lrclk, bclk);

	  for (int i = 0; i < I2S_SLAVE_NUM_IN; i++)
	  {
	    set_port_clock(p_din[i], bclk);
	  }
	  // start clock block after configuration
	  start_clock(bclk);

	// fast mode - instructions repeatedly issued instead of paused
	set_thread_fast_mode_on();

	i2s_slave_loop(p_din, p_lrclk, input_fifos, num_in );

	set_thread_fast_mode_off();
}

#endif


