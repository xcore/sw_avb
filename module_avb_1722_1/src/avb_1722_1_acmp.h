#ifndef AVB_1722_1_ACMP_H_
#define AVB_1722_1_ACMP_H_

#include "avb_api.h"
#include <xccompat.h>
#include "xc2compat.h"
#include "avb_1722_1_acmp_pdu.h"
#include "avb_control_types.h"

#define AVB_1722_1_ACMP_DEST_MAC {0x91, 0xe0, 0xf0, 0x01, 0x00, 0x00};

enum acmp_controller_state_t {
        ACMP_CONTROLLER_IDLE,
        ACMP_CONTROLLER_WAITING,
        ACMP_CONTROLLER_TIMEOUT,
        ACMP_CONTROLLER_CONNECT_RX_RESPONSE,
        ACMP_CONTROLLER_DISCONNECT_RX_RESPONSE,
        ACMP_CONTROLLER_GET_TX_STATE_RESPONSE,
        ACMP_CONTROLLER_GET_RX_STATE_RESPONSE,
        ACMP_CONTROLLER_GET_TX_CONNECTION_RESPONSE
};

enum acmp_talker_state_t {
        ACMP_TALKER_IDLE,
        ACMP_TALKER_WAITING,
        ACMP_TALKER_CONNECT,
        ACMP_TALKER_DISCONNECT,
        ACMP_TALKER_GET_STATE,
        ACMP_TALKER_GET_CONNECTION
};

enum acmp_listener_state_t {
        ACMP_LISTENER_IDLE,
        ACMP_LISTENER_WAITING,
        ACMP_LISTENER_CONNECT_RX_COMMAND,
        ACMP_LISTENER_DISCONNECT_RX_COMMAND,
        ACMP_LISTENER_CONNECT_TX_RESPONSE,
        ACMP_LISTENER_DISCONNECT_TX_RESPONSE,
        ACMP_LISTENER_GET_STATE,
        ACMP_LISTENER_RX_TIMEOUT
};

void avb_1722_1_acmp_controller_init();
void avb_1722_1_acmp_controller_deinit();
void avb_1722_1_acmp_talker_init();
void avb_1722_1_acmp_listener_init();

#ifdef __XC__
void avb_1722_1_acmp_controller_periodic(chanend c_tx, client interface avb_interface avb);
void avb_1722_1_acmp_talker_periodic(chanend c_tx, client interface avb_interface avb);
void avb_1722_1_acmp_listener_periodic(chanend c_tx, client interface avb_interface avb);
#endif

/** Setup a new stream connection between a Talker and Listener entity.
 *
 *  The Controller shall send a CONNECT_RX_COMMAND to the Listener Entity. The Listener Entity shall then send a
 *  CONNECT_TX_COMMAND to the Talker Entity.
 *
 *  \param talker_guid      the GUID of the Talker being targeted by the command
 *  \param listener_guid    the GUID of the Listener being targeted by the command
 *  \param talker_id        the unique id of the Talker stream source to connect.
 *                          For entities using AEM, this corresponds to the id of the STREAM_OUTPUT descriptor
 *  \param listener_id      the unique id of the Listener stream source to connect.
 *                          For entities using AEM, this corresponds to the id of the STREAM_INPUT descriptor
 *  \param c_tx             a transmit chanend to the Ethernet server
 *
 **/
void avb_1722_1_controller_connect(const_guid_ref_t talker_guid,
                                   const_guid_ref_t listener_guid,
                                   int talker_id,
                                   int listener_id,
                                   chanend c_tx);

/** Disconnect an existing stream connection between a Talker and Listener entity.
 *
 *  The Controller shall send a DISCONNECT_RX_COMMAND to the Listener Entity. The Listener Entity shall then send a
 *  DISCONNECT_TX_COMMAND to the Talker Entity.
 *
 *  \param talker_guid      the GUID of the Talker being targeted by the command
 *  \param listener_guid    the GUID of the Listener being targeted by the command
 *  \param talker_id        the unique id of the Talker stream source to disconnect.
 *                          For entities using AEM, this corresponds to the id of the STREAM_OUTPUT descriptor
 *  \param listener_id      the unique id of the Listener stream source to disconnect.
 *                          For entities using AEM, this corresponds to the id of the STREAM_INPUT descriptor
 *  \param c_tx             a transmit chanend to the Ethernet server
 *
 **/
void avb_1722_1_controller_disconnect(const_guid_ref_t talker_guid,
                                      const_guid_ref_t listener_guid,
                                      int talker_id,
                                      int listener_id,
                                      chanend c_tx);

/** Disconnect all Listener sinks currently connected to the Talker stream source with ``talker_id``
 *
 *
 *  \param talker_id        the unique id of the Talker stream source to disconnect its listeners.
 *                          For entities using AEM, this corresponds to the id of the STREAM_OUTPUT descriptor
 *  \param c_tx             a transmit chanend to the Ethernet server
 *
 **/
void avb_1722_1_controller_disconnect_all_listeners(int talker_id, chanend c_tx);


/** Disconnect the Talker source currently connected to the Listener stream sink with ``listener_id``
 *
 *
 *  \param listener_id      the unique id of the Listener stream source to disconnect its Talker.
 *                          For entities using AEM, this corresponds to the id of the STREAM_INPUT descriptor
 *  \param c_tx             a transmit chanend to the Ethernet server
 *
 **/
void avb_1722_1_controller_disconnect_talker(int listener_id, chanend c_tx);

/**
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_TALKER and
 * AVB_1722_1_DISCONNECT_TALKER notifications.
 */
unsigned avb_1722_1_acmp_get_talker_connection_info(REFERENCE_PARAM(short,talker));

/**
 *
 * Return information that is required for processing the AVB_1722_1_CONNECT_LISTENER and
 * AVB_1722_1_DISCONNECT_LISTENER notifications.
 */
unsigned avb_1722_1_acmp_get_listener_connection_info(REFERENCE_PARAM(short,listener), unsigned char address[6], unsigned streamId[2], REFERENCE_PARAM(unsigned,vlan));

/**
 *
 *  Called by the application to inform 1722.1 of the source MAC
 *  address for the particular talker.
 */
void avb_1722_1_talker_set_mac_address(unsigned talker_unique_id, unsigned char macaddr[]);

/**
 *
 * Called by the application to inform 1722.1 of the source stream identifier
 * for a particular stream.
 */
void avb_1722_1_talker_set_stream_id(unsigned talker_unique_id, unsigned streamId[2]);

void acmp_progress_inflight_timer(int entity_type);

int acmp_check_inflight_command_timeouts(int entity_type);

void acmp_send_response(int message_type, avb_1722_1_acmp_cmd_resp *response, int status, chanend c_tx);

void acmp_set_talker_response(void);

unsigned acmp_listener_valid_listener_unique(void);

void acmp_add_listener_stream_info(void);

avb_1722_1_acmp_status_t acmp_listener_get_state(void);

avb_1722_1_acmp_status_t acmp_talker_get_state(void);

avb_1722_1_acmp_status_t acmp_talker_get_connection(void);

void acmp_add_talker_stream_info(void);

void acmp_remove_talker_stream_info(void);

unsigned acmp_talker_valid_talker_unique(void);

#ifdef __XC__
void acmp_send_command(int entity_type, int message_type, avb_1722_1_acmp_cmd_resp *alias command, int retry, int inflight_idx, chanend c_tx);
#endif

#ifdef __XC__
extern "C" {
#endif
void avb_1722_1_create_acmp_packet(avb_1722_1_acmp_cmd_resp *cr, int message_type, int status);
void process_avb_1722_1_acmp_packet(avb_1722_1_acmp_packet_t *pkt, chanend c_tx);
avb_1722_1_acmp_inflight_command *acmp_remove_inflight(int entity_type);
#ifdef __XC__
}
#endif
void acmp_set_inflight_retry(int entity_type, unsigned int message_type, int inflight_idx);

void acmp_add_inflight(int entity_type, unsigned int message_type, unsigned short original_sequence_id);

void acmp_controller_connect_disconnect(int message_type, const_guid_ref_t talker_guid, const_guid_ref_t listener_guid, int talker_id, int listener_id, chanend c_tx);

#endif /* AVB_17221_ACMP_H_ */
