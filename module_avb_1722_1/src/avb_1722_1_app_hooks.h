#ifndef AVB_1722_1_APP_HOOKS_H_
#define AVB_1722_1_APP_HOOKS_H_

#include <xccompat.h>
#include "avb_1722_1_common.h"
#include "avb_1722_1_adp.h"


/** A new AVDECC entity has advertised itself as available. It may be an entity starting up or 
 *  a previously seen entity that had timed out.
 *
 * \param my_guid   The GUID of this entity
 * \param entity    The information advertised by the remote entity
 * \param c_tc      A transmit channel end to the Ethernet server
 **/
void avb_entity_on_new_entity_available(REFERENCE_PARAM(guid_t, my_guid), REFERENCE_PARAM(avb_1722_1_entity_record, entity), chanend c_tx);


/** A Controller has indicated that a Listener is connecting to this Talker stream source 
 *
 * \param source_num        The local id of the Talker stream source
 * \param listener_guid     The GUID of the Listener entity that is connecting
 **/
void avb_talker_on_listener_connect(int source_num, REFERENCE_PARAM(guid_t, listener_guid));

/** A Controller has indicated that a Listener is disconnecting from this Talker stream source 
 *
 * \param source_num        The local id of the Talker stream source
 * \param listener_guid     The GUID of the Listener entity that is disconnecting
 * \param connection_count  The number of connections a Talker thinks it has on it’s stream source,
                            i.e. the number of connect TX stream commands it has received less the number of
                            disconnect TX stream commands it has received. This number may not be accurate since an AVDECC Entity may
                            not have sent a disconnect command if the cable was disconnected or the AVDECC Entity abruptly powered down.
 **/
void avb_talker_on_listener_disconnect(int source_num, REFERENCE_PARAM(guid_t, listener_guid), int connection_count);

/** A Controller has indicated to connect this Listener sink to a Talker stream
 *
 * \param sink_num          The local id of the Listener stream sink
 * \param talker_guid       The GUID of the Talker entity that is connecting
 * \param dest_addr         The destination MAC address of the Talker stream
 * \param stream_id         The 64 bit Stream ID of the Talker stream
 **/
void avb_listener_on_talker_connect(int sink_num, REFERENCE_PARAM(guid_t, talker_guid), unsigned char dest_addr[6], unsigned int stream_id[2]);

/** A Controller has indicated to disconnect this Listener sink from a Talker stream
 *
 * \param sink_num          The local id of the Listener stream sink
 * \param talker_guid       The GUID of the Talker entity that is disconnecting
 * \param dest_addr         The destination MAC address of the Talker stream
 * \param stream_id         The 64 bit Stream ID of the Talker stream
 **/
void avb_listener_on_talker_disconnect(int sink_num, REFERENCE_PARAM(guid_t, talker_guid), unsigned char dest_addr[6], unsigned int stream_id[2]);


#endif