#ifndef __media_clock_server_h__
#define __media_clock_server_h__

#include "avb_conf.h"

#ifndef MAX_NUM_MEDIA_CLOCKS
#define MAX_NUM_MEDIA_CLOCKS 2
#endif

#ifndef MAX_CLK_CTL_CLIENTS
#define MAX_CLK_CTL_CLIENTS 8
#endif



/** The type of a media clock.
 *  A media clock can be either derived from the PTP clock or recovered 
 *  from an 
 *  incoming media FIFO (which in turn will derive its timing
 *  from the IEEE 1722 audio stream it came from)
 */
typedef enum media_clock_type_t {
  PTP_DERIVED,
  INPUT_STREAM_DERIVED,
  LOCAL_CLOCK
} media_clock_type_t; 


/** The media clock server. 
 * 
 *  \param media_clock_ctl     chanend connected to the main control thread 
 *                          and passed into avb_init()
 *  \param ptp_svr          chanend connected to the PTP server
 *  \param buf_ctl[]        array of links to listener components 
 *                          requiring buffer management
 *  \param buf_ctl_size     size of the buf_ctl array
 *  \param clk_ctl[]        array of links to components requiring a media
 *                          clock
 *  \param clk_ctl_size     size of the clk_ctl array
 */
#ifdef __XC__
void media_clock_server(chanend media_clock_ctl,
                        chanend ptp_svr,
                        chanend ?buf_ctl[], int buf_ctl_size,
                        chanend ?clk_ctl[], int clk_ctl_size);
#else
void media_clock_server(chanend media_clock_ctl,
                        chanend ptp_svr,
                        chanend buf_ctl[], int buf_ctl_size,
                        chanend clk_ctl[], int clk_ctl_size);
#endif

#endif
