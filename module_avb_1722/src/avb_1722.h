/*
 * @ModuleName IEC 61883-6/AVB1722 Audio over 1722 AVB Transport.
 * @Description: Implement top level
 *
 *
 *
 */

#ifndef _AVB1722_TOP_H_ 
#define _AVB1722_TOP_H_ 1


/** An AVB IEEE 1722 audio talker thread. 
 *  This thread implements a talker, taking
 *  media input FIFOs and combining them into 1722 packets to be
 *  sent to the ethernet component. It is dynamically configured 
 *  using the AVB control API.
 * 
 *  \param ptp_svr          link to the PTP timing server
 *  \param ethernet_tx_svr  transmit link to the ethernet MAC
 *  \param talker_ctl       channel to configure the talker (given 
 *                          to avb_init())
 *  \param num_streams      the number of streams the unit controls
 **/
void avb_1722_talker(chanend ptp_svr,
                     chanend ethernet_tx_svr,
                     chanend talker_ctl,
                     int num_streams);

/** An AVB IEEE 1722 audio listener thread.
 *
 *  This thread implements a listener. It takes IEEE 1722 packets from 
 *  the ethernet MAC and splits them into output FIFOs. The 
 *  buffer fill level of these streams is set in conjunction with communication
 *  to the media clock server. This thread is dynamically configured
 *  using the AVB control API.
 *
 *  \param ethernet_rx_svr  receive link to the ethernet MAC
 *  \param ethernet_tx_svr  transmit link to the ethernet MAC
 *  \param buf_ctl          buffer control link to the media clock server
 *  \param listener_ctl     channel to configure the listener (given
 *                          to avb_init())
 *  \param num_stream       the number of streams the unit will handle
 */
void avb_1722_listener(chanend ethernet_rx_svr,
                       chanend ethernet_tx_svr,
                       chanend buf_ctl,
                       chanend listener_ctl,
                       int num_streams);


#endif
