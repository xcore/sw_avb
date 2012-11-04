#ifndef __avb_1722_talkerlistener_h__
#define __avb_1722_talkerlistener_h__

void avb_1722_talkerlistener(chanend c_ptp,
                             chanend c_mac_rx,
                             chanend c_mac_tx,
                             chanend c_listener_ctl,
                             chanend c_talker_ctl,
                             chanend c_buf_ctl,
                             int num_listener_streams,
                             int num_talker_streams);

#endif// __avb_1722_talkerlistener_h__
