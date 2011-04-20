#ifndef __media_clock_h__
#define __media_clock_h__
#include <xccompat.h>

/**
 * \file avb_media_clock.h
 * \brief Client functions for the media clock interface
 *
 * These are called by the top level AVB API
 */

/**
 *  \brief Set the frequency of the media clock
 *
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 *  \param rate the new rate of the media clock in Hz
 */
void media_clock_set_rate(chanend media_clock_svr, int media_clock_num, int rate);

/**
 *  \brief Get the rate of a media clock
 *
 *  \return the rate of the media clock in Hz
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 */
int  media_clock_get_rate(chanend media_clock_svr, int media_clock_num);

/**
 *  \brief Set the type of the media clock
 *
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 *  \param type the type of the media clock, from the media_clock_type_t enumeration
 */
void media_clock_set_type(chanend media_clock_svr, int media_clock_num, int type);

/**
 *  \brief Get the type of the media clock
 *
 *  \return The type of the clock, from the media_clock_type_t enumeration
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 */
int  media_clock_get_type(chanend media_clock_svr, int media_clock_num);

/**
 *  \brief Set the source of the media clock
 *
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 *  \param a the new clock source, the index of the FIFO that it is based on
 */
void media_clock_set_source(chanend media_clock_svr, int media_clock_num, 
                         int a);

/**
 *  \brief Get the source of the media clock
 *
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 *  \param a pointer to an integer where the clock source is placed
 */
void media_clock_get_source(chanend media_clock_svr, int media_clock_num, 
                         REFERENCE_PARAM(int,a));


/**
 *  \brief Set the state of the media clock
 *
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 *  \param state the new state, from the
 */
void media_clock_set_state(chanend media_clock_svr, int media_clock_num, int state);

/**
 *  \brief
 *
 *  \return The state of the clock, from the
 *  \param media_clock_svr the media server control channel
 *  \param media_clock_num the index of the specific media clock
 */
int  media_clock_get_state(chanend media_clock_svr, int media_clock_num);

/**
 *  \brief
 *
 *  \param media_clock_svr the media server control channel
 *  \param clk_ctl
 *  \param clk_num
 */
void media_clock_register(chanend media_clock_svr, int clk_ctl, int clk_num);

#endif 
