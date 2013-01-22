/**
 * \file avb_1722.h
 * \brief IEC 61883-6/AVB1722 common definitions
 */

#ifndef _AVB1722_TOP_H_ 
#define _AVB1722_TOP_H_ 1

#ifdef __XC__

/** An AVB IEEE 1722 audio talker thread. 
 *  This thread implements a talker, taking
 *  media input FIFOs and combining them into 1722 packets to be
 *  sent to the ethernet component. It is dynamically configured 
 *  using the AVB control API.
 * 
 *  \param c_ptp            link to the PTP timing server
 *  \param c_mac_tx         transmit link to the ethernet MAC
 *  \param c_talker_ctl     channel to configure the talker (given 
 *                          to avb_init())
 *  \param num_streams      the number of streams the unit controls
 **/
void avb_1722_talker(chanend c_ptp,
                     chanend c_mac_tx,
                     chanend c_talker_ctl,
                     int num_streams);

/** An AVB IEEE 1722 audio listener thread.
 *
 *  This thread implements a listener. It takes IEEE 1722 packets from 
 *  the ethernet MAC and splits them into output FIFOs. The 
 *  buffer fill level of these streams is set in conjunction with communication
 *  to the media clock server. This thread is dynamically configured
 *  using the AVB control API.
 *
 *  \param c_mac_rx         receive link to the ethernet MAC
 *  \param c_buf_ctl        buffer control link to the media clock server
 *  \param c_ptp_ctl        PTP server link for retreiving PTP time info
 *  \param c_listener_ctl   channel to configure the listener (given
 *                          to avb_init())
 *  \param num_streams      the number of streams the unit will handle
 */
void avb_1722_listener(chanend c_mac_rx,
                       chanend? c_buf_ctl,
                       chanend? c_ptp_ctl,
                       chanend c_listener_ctl,
                       int num_streams);



void avb_1722_talkerlistener(chanend c_ptp,
                             chanend c_mac_rx,
                             chanend c_mac_tx,
                             chanend c_listener_ctl,
                             chanend c_talker_ctl,
                             chanend c_buf_ctl,
                             int num_listener_streams,
                             int num_talker_streams);


#endif

#endif
