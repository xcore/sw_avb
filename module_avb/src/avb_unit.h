#ifndef __avb_unit_h__
#define __avb_unit_h__

int avb_register_listener_streams(chanend listener_ctl,
                                   int num_streams);

void avb_register_talker_streams(chanend listener_ctl,
                                 int num_streams);

#endif
