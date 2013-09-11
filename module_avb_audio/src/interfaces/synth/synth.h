#ifndef __synth_h__
#define __synth_h__

/** Output synthesised beeps.
 *
 *  This function runs in a thread and outputs 48kHz stereo
 *  synthesized beeps to local talker streams. It is useful for testing.
 *
 *  \param period               - the period of the sinewave used (in samples)
 *  \param media_ctl            - the media control channel
 *  \param clk_ctl              - a link to the media clock server
 *  \param clk_ctl_index        - The index of the clk_ctl channel in the array * passed to the media clock server
 *  \param ififos               - array of media input FIFOs, one for each channel
 *  \param num_channels         - the number of channels
 */
void synth(int period,
           chanend media_ctl,
           chanend clk_ctl,
           int clk_ctl_index,
           media_input_fifo_t ififos[],
           int num_channels);

#endif
