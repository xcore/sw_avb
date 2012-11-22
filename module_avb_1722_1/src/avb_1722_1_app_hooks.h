#ifndef AVB_1722_1_APP_HOOKS_H_
#define AVB_1722_1_APP_HOOKS_H_

#include <xccompat.h>
#include "avb_1722_1_common.h"
#include "avb_1722_1_adp.h"

void avb_controller_on_new_entity_available(REFERENCE_PARAM(guid_t, my_guid), REFERENCE_PARAM(avb_1722_1_entity_record, entity), chanend c_tx);

void avb_talker_on_listener_connect(int source_num, unsigned char listener_guid[8]);

void avb_talker_on_listener_disconnect(int talker_id, unsigned char listener_guid[8], int connection_count);

void avb_listener_on_talker_connect(int sink_num, unsigned char dest_addr[6], unsigned int stream_id[2]);

void avb_listener_on_talker_disconnect(int sink_num, unsigned char dest_addr[6], unsigned int stream_id[2]);


#endif