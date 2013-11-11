#include <string.h>
#include <print.h>
#include "avb_1722_common.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_acmp.h"
#include "simple_printf.h"
#ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
#include "avb_1722_1_acmp_debug.h"
#endif
#include "avb_api.h"
#include "misc_timer.h"
#ifdef AVB_1722_1_ENABLE_ASSERTIONS
#include <assert.h>
#endif
#include "avb_1722_1_app_hooks.h"
#include "avb_1722_1.h"

/* Inflight command defines */
#define CONTROLLER  0
#define LISTENER    1

#define TRUE 1
#define FALSE 0

extern enum acmp_controller_state_t acmp_controller_state;
extern enum acmp_talker_state_t acmp_talker_state;
extern enum acmp_listener_state_t acmp_listener_state;

extern guid_t my_guid;

// Stream info lists
extern avb_1722_1_acmp_listener_stream_info acmp_listener_streams[AVB_1722_1_MAX_LISTENERS];
extern avb_1722_1_acmp_talker_stream_info acmp_talker_streams[AVB_1722_1_MAX_TALKERS];

// Inflight command lists
extern avb_1722_1_acmp_inflight_command acmp_controller_inflight_commands[AVB_1722_1_MAX_INFLIGHT_COMMANDS];
extern avb_1722_1_acmp_inflight_command acmp_listener_inflight_commands[AVB_1722_1_MAX_INFLIGHT_COMMANDS];

static int acmp_inflight_timeout_idx[2];

// Controller command
extern avb_1722_1_acmp_cmd_resp acmp_controller_cmd_resp;

// Talker's rcvdCmdResp
extern avb_1722_1_acmp_cmd_resp acmp_talker_rcvd_cmd_resp;

// Listener's rcvdCmdResp
extern avb_1722_1_acmp_cmd_resp acmp_listener_rcvd_cmd_resp;

extern short sequence_id[2];

extern void acmp_zero_listener_stream_info(int unique_id);
extern unsigned int avb_1722_1_buf[AVB_1722_1_PACKET_SIZE_WORDS];


void acmp_send_command(int entity_type, int message_type, avb_1722_1_acmp_cmd_resp *command, int retry, int inflight_idx, chanend c_tx)
{
    /* We need to save the sequence_id of the Listener command that generated this Talker command for the response */
    unsigned short original_sequence_id = command->sequence_id;
    char *pkt_without_eth_header = ((char *)avb_1722_1_buf)+14;
    command->sequence_id = sequence_id[entity_type];
    sequence_id[entity_type]++;

    avb_1722_1_create_acmp_packet(command, message_type, ACMP_STATUS_SUCCESS);
    mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ACMP_PACKET_SIZE, -1);
    process_avb_1722_1_acmp_packet((avb_1722_1_acmp_packet_t *)pkt_without_eth_header, c_tx);

    if (!retry)
    {
        acmp_add_inflight(entity_type, message_type, original_sequence_id);
    }
    else
    {
#ifdef AVB_1722_1_ENABLE_ASSERTIONS
        assert(inflight_idx >= 0);
#endif
        acmp_set_inflight_retry(entity_type, message_type, inflight_idx);
    }
}

void acmp_send_response(int message_type, avb_1722_1_acmp_cmd_resp *response, int status, chanend c_tx)
{
    avb_1722_1_create_acmp_packet(response, message_type, status);
    mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ACMP_PACKET_SIZE, -1);
}

void acmp_controller_connect_disconnect(int message_type, const_guid_ref_t talker_guid, const_guid_ref_t listener_guid, int talker_id, int listener_id, chanend c_tx)
{
    acmp_controller_cmd_resp.controller_guid = my_guid;
    acmp_controller_cmd_resp.talker_guid.l = talker_guid.l;
    acmp_controller_cmd_resp.listener_guid.l = listener_guid.l;
    acmp_controller_cmd_resp.talker_unique_id = talker_id;
    acmp_controller_cmd_resp.listener_unique_id = listener_id;

    acmp_send_command(CONTROLLER, message_type, &acmp_controller_cmd_resp, FALSE, -1, c_tx);
}


/**
 * See 8.2.2.5.2.2 and 8.2.2.5.2.3 for explanation.
 *
 * The connected_to param equal to 1 is equivalent to listenerIsConnectedTo(command) in spec
 */
static unsigned acmp_listener_is_connected(int connected_to, client interface avb_interface avb)
{
    enum avb_sink_state_t state;
    unsigned stream_is_reserved;
    int unique_id = acmp_listener_rcvd_cmd_resp.listener_unique_id;

    avb.get_sink_state(unique_id, state);
    stream_is_reserved = (state != AVB_SINK_STATE_DISABLED);

    if (stream_is_reserved)
    {
        if (acmp_listener_streams[unique_id].talker_guid.l == acmp_listener_rcvd_cmd_resp.talker_guid.l &&
            acmp_listener_streams[unique_id].talker_unique_id == acmp_listener_rcvd_cmd_resp.talker_unique_id)
        {
            if (connected_to) return 1;
            else return 0;
        }
        else
        {
            if (!connected_to) return 1;
        }
    }

    return 0;
}


void avb_1722_1_acmp_controller_periodic(chanend c_tx, client interface avb_interface avb)
{
    switch (acmp_controller_state)
    {
        case ACMP_CONTROLLER_IDLE:
        {
            break;
        }
        case ACMP_CONTROLLER_WAITING:
        {
            acmp_progress_inflight_timer(CONTROLLER);

            // acmp_inflight_timeout_idx is a global index provided to ACMP_CONTROLLER_TIMEOUT
            acmp_inflight_timeout_idx[CONTROLLER] = acmp_check_inflight_command_timeouts(CONTROLLER);

            if (acmp_inflight_timeout_idx[CONTROLLER] >= 0) // An inflight command has timed out
            {
                acmp_controller_state = ACMP_CONTROLLER_TIMEOUT;
            }

            break;
        }
        case ACMP_CONTROLLER_TIMEOUT:
        {
            int i = acmp_inflight_timeout_idx[CONTROLLER];
            if (acmp_controller_inflight_commands[i].retried)
            {
                // Remove inflight command
                acmp_controller_inflight_commands[i].in_use = 0;

#ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
                simple_printf("ACMP Controller: Removed inflight %s with timed out retry - seq id: %d\n",
                        debug_acmp_message_s[acmp_controller_inflight_commands[i].command.message_type],
                        acmp_controller_inflight_commands[i].original_sequence_id);
#endif
            }
            else
            {
                acmp_send_command(CONTROLLER, acmp_controller_inflight_commands[i].command.message_type,
                                        &acmp_controller_inflight_commands[i].command, TRUE, i, c_tx);

#ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
                simple_printf("ACMP Controller: Sent retry for timed out %s - seq id: %d\n",
                        debug_acmp_message_s[acmp_controller_inflight_commands[i].command.message_type],
                        acmp_controller_inflight_commands[i].original_sequence_id);
#endif
            }

            acmp_controller_state = ACMP_CONTROLLER_WAITING;

            break;
        }
        // TODO:
        case ACMP_CONTROLLER_CONNECT_RX_RESPONSE:
        {
            // Remove inflight command
            acmp_remove_inflight(CONTROLLER);

            if (acmp_controller_cmd_resp.status != ACMP_STATUS_SUCCESS)
            {
                avb_talker_on_listener_connect_failed(avb, my_guid, acmp_controller_cmd_resp.talker_unique_id,
                        acmp_controller_cmd_resp.listener_guid, acmp_controller_cmd_resp.status, c_tx);
            }

            acmp_controller_state = ACMP_CONTROLLER_WAITING;
            break;
        }
        case ACMP_CONTROLLER_DISCONNECT_RX_RESPONSE:
        case ACMP_CONTROLLER_GET_TX_STATE_RESPONSE:
        case ACMP_CONTROLLER_GET_RX_STATE_RESPONSE:
        case ACMP_CONTROLLER_GET_TX_CONNECTION_RESPONSE:
        {

#ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
            // Remove inflight command
            avb_1722_1_acmp_inflight_command *inflight = acmp_remove_inflight(CONTROLLER);
            if (inflight)
            {
                simple_printf("ACMP Controller: Removed inflight %s with response %s - seq id: %d\n",
                debug_acmp_message_s[inflight->command.message_type],
                debug_acmp_status_s[inflight->command.status],
                inflight->original_sequence_id);
            }

#else
            // Remove inflight command
            acmp_remove_inflight(CONTROLLER);
#endif

            acmp_controller_state = ACMP_CONTROLLER_WAITING;
            break;
        }
    }
}

void avb_1722_1_acmp_talker_periodic(chanend c_tx, client interface avb_interface avb)
{
    switch (acmp_talker_state)
    {
        case ACMP_TALKER_IDLE:
        {
            break;
        }
        case ACMP_TALKER_WAITING:
        {
            return;
        }
        case ACMP_TALKER_CONNECT:
        {
            if (!acmp_talker_valid_talker_unique())
            {
                acmp_send_response(ACMP_CMD_CONNECT_TX_RESPONSE, &acmp_talker_rcvd_cmd_resp, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
            }
            else
            {
                acmp_add_talker_stream_info();
                /* Application hook */
                avb_talker_on_listener_connect(avb, acmp_talker_rcvd_cmd_resp.talker_unique_id, acmp_talker_rcvd_cmd_resp.listener_guid);

                acmp_set_talker_response();
                acmp_send_response(ACMP_CMD_CONNECT_TX_RESPONSE, &acmp_talker_rcvd_cmd_resp, ACMP_STATUS_SUCCESS, c_tx);

            }
            acmp_talker_state = ACMP_TALKER_WAITING;
            break;
        }
        case ACMP_TALKER_DISCONNECT:
        {
            if (!acmp_talker_valid_talker_unique())
            {
                acmp_send_response(ACMP_CMD_DISCONNECT_TX_RESPONSE, &acmp_talker_rcvd_cmd_resp, ACMP_STATUS_TALKER_UNKNOWN_ID, c_tx);
            }
            else
            {
                unsigned unique_id = acmp_talker_rcvd_cmd_resp.talker_unique_id;
                acmp_remove_talker_stream_info();
                /* Application hook */
                avb_talker_on_listener_disconnect(avb, unique_id, acmp_talker_rcvd_cmd_resp.listener_guid, acmp_talker_streams[unique_id].connection_count);

                acmp_set_talker_response();
                acmp_send_response(ACMP_CMD_DISCONNECT_TX_RESPONSE, &acmp_talker_rcvd_cmd_resp, ACMP_STATUS_SUCCESS, c_tx);
            }
            acmp_talker_state = ACMP_TALKER_WAITING;
            break;
        }
        case ACMP_TALKER_GET_STATE:
        {
            int acmp_status;
            if (!acmp_talker_valid_talker_unique())
            {
                acmp_status = ACMP_STATUS_TALKER_UNKNOWN_ID;
            }
            else
            {
                acmp_status = acmp_talker_get_state();
            }
            acmp_send_response(ACMP_CMD_GET_TX_STATE_RESPONSE, &acmp_talker_rcvd_cmd_resp, acmp_status, c_tx);

            acmp_talker_state = ACMP_TALKER_WAITING;
            return;
        }
        case ACMP_TALKER_GET_CONNECTION:
        {
            int acmp_status;
            if (!acmp_talker_valid_talker_unique())
            {
                acmp_status = ACMP_STATUS_TALKER_UNKNOWN_ID;
            }
            else
            {
                acmp_status = acmp_talker_get_connection();
            }
            acmp_send_response(ACMP_CMD_GET_TX_CONNECTION_RESPONSE, &acmp_talker_rcvd_cmd_resp, acmp_status, c_tx);

            acmp_talker_state = ACMP_TALKER_WAITING;
            return;
        }
    }
}

void avb_1722_1_acmp_listener_periodic(chanend c_tx, client interface avb_interface avb)
{
    switch (acmp_listener_state)
    {
        case ACMP_LISTENER_IDLE:
        {
            break;
        }
        case ACMP_LISTENER_WAITING:
        {
            acmp_progress_inflight_timer(LISTENER);

            acmp_inflight_timeout_idx[LISTENER] = acmp_check_inflight_command_timeouts(LISTENER);

            if (acmp_inflight_timeout_idx[LISTENER] >= 0)   // An inflight command has timed out
            {
                acmp_listener_state = ACMP_LISTENER_RX_TIMEOUT;
            }
            break;
        }

        case ACMP_LISTENER_CONNECT_RX_COMMAND:
        {
            if (!acmp_listener_valid_listener_unique())
            {
                acmp_send_response(ACMP_CMD_CONNECT_RX_RESPONSE, &acmp_listener_rcvd_cmd_resp, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
            }
            else
            {
                if (acmp_listener_is_connected(0, avb))
                {
                    acmp_send_response(ACMP_CMD_CONNECT_RX_RESPONSE, &acmp_listener_rcvd_cmd_resp, ACMP_STATUS_LISTENER_EXCLUSIVE, c_tx);
                }
                else
                {

                    acmp_send_command(LISTENER, ACMP_CMD_CONNECT_TX_COMMAND, &acmp_listener_rcvd_cmd_resp, FALSE, -1, c_tx);
                }
            }
            acmp_listener_state = ACMP_LISTENER_WAITING;
            break;
        }
        case ACMP_LISTENER_DISCONNECT_RX_COMMAND:
        {
            if (!acmp_listener_valid_listener_unique())
            {
                acmp_send_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, &acmp_listener_rcvd_cmd_resp, ACMP_STATUS_LISTENER_UNKNOWN_ID, c_tx);
            }
            else
            {
                if (acmp_listener_is_connected(1, avb))
                {
                    unsigned stream_id[2];
                    acmp_send_command(LISTENER, ACMP_CMD_DISCONNECT_TX_COMMAND, &acmp_listener_rcvd_cmd_resp, FALSE, -1, c_tx);

                    stream_id[1] = (unsigned)(acmp_listener_rcvd_cmd_resp.stream_id.l >> 0);
                    stream_id[0] = (unsigned)(acmp_listener_rcvd_cmd_resp.stream_id.l >> 32);

                    avb_listener_on_talker_disconnect(avb,
                                            acmp_listener_rcvd_cmd_resp.listener_unique_id,
                                            acmp_listener_rcvd_cmd_resp.talker_guid,
                                            acmp_listener_rcvd_cmd_resp.stream_dest_mac,
                                            stream_id,
                                            my_guid);
                }
                else
                {
                    acmp_send_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, &acmp_listener_rcvd_cmd_resp, ACMP_STATUS_NOT_CONNECTED, c_tx);
                }
            }
            acmp_listener_state = ACMP_LISTENER_WAITING;
            break;
        }
        case ACMP_LISTENER_CONNECT_TX_RESPONSE:
        {
            unsafe {

                if (acmp_listener_valid_listener_unique())
                {
                    unsigned stream_id[2];
                    avb_1722_1_acmp_inflight_command *unsafe inflight = acmp_remove_inflight(LISTENER);

                    if (inflight != NULL)
                    {
                        acmp_listener_rcvd_cmd_resp.sequence_id = inflight->original_sequence_id; // FIXME: This is a bit messy

    #ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
                        simple_printf("ACMP Listener: Removed inflight CONNECT_TX_COMMAND with response %s - seq id: %d\n",
                                debug_acmp_status_s[inflight->command.status],
                                inflight->command.sequence_id);
    #endif

                            /* FIXME: Make stream ID representation consistent: we have long long, 2 ints and 6 chars */

                            stream_id[1] = (unsigned)(acmp_listener_rcvd_cmd_resp.stream_id.l >> 0);
                            stream_id[0] = (unsigned)(acmp_listener_rcvd_cmd_resp.stream_id.l >> 32);

                            avb_listener_on_talker_disconnect(avb,
                                                        acmp_listener_rcvd_cmd_resp.listener_unique_id,
                                                        acmp_listener_rcvd_cmd_resp.talker_guid,
                                                        acmp_listener_rcvd_cmd_resp.stream_dest_mac,
                                                        stream_id,
                                                        my_guid);

                        acmp_listener_rcvd_cmd_resp.status =
                            avb_listener_on_talker_connect(avb,
                                                    acmp_listener_rcvd_cmd_resp.listener_unique_id,
                                                    acmp_listener_rcvd_cmd_resp.talker_guid,
                                                    acmp_listener_rcvd_cmd_resp.stream_dest_mac,
                                                    stream_id,
                                                    my_guid);

                        acmp_send_response(ACMP_CMD_CONNECT_RX_RESPONSE, &acmp_listener_rcvd_cmd_resp, acmp_listener_rcvd_cmd_resp.status, c_tx);
                        acmp_add_listener_stream_info();
                    }

                    acmp_listener_state = ACMP_LISTENER_WAITING;

                    return;
                }
            }
            break;
        }
        case ACMP_LISTENER_DISCONNECT_TX_RESPONSE:
        {
            unsafe {
                if (acmp_listener_valid_listener_unique())
                {
                    avb_1722_1_acmp_inflight_command *unsafe inflight = acmp_remove_inflight(LISTENER);

                    if (inflight != NULL)
                    {
                        acmp_listener_rcvd_cmd_resp.sequence_id = inflight->original_sequence_id;

                        acmp_send_response(ACMP_CMD_DISCONNECT_RX_RESPONSE, &acmp_listener_rcvd_cmd_resp, acmp_listener_rcvd_cmd_resp.status, c_tx);
                        acmp_zero_listener_stream_info(acmp_listener_rcvd_cmd_resp.listener_unique_id);

        #ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
                        simple_printf("ACMP Listener: Removed inflight %d DISCONNECT_TX_COMMAND with response %s - seq id: %d\n",
                                (int)inflight,
                                debug_acmp_status_s[inflight->command.status],
                                inflight->command.sequence_id);
        #endif
                    }


                    acmp_listener_rcvd_cmd_resp.status = ACMP_STATUS_SUCCESS;
                    acmp_listener_state = ACMP_LISTENER_WAITING;

                    return;
                }
                break;
            }
        }
        case ACMP_LISTENER_GET_STATE:
        {
            int status;

            if (!acmp_listener_valid_listener_unique())
            {
                status = ACMP_STATUS_LISTENER_UNKNOWN_ID;
            }
            else
            {
                /* Sets appropriate state fields in acmp_listener_rcvd_cmd_resp for responding and returns status message */
                status = acmp_listener_get_state();
            }

            acmp_send_response(ACMP_CMD_GET_RX_STATE_RESPONSE, &acmp_listener_rcvd_cmd_resp, status, c_tx);

            acmp_listener_state = ACMP_LISTENER_WAITING;
            break;
        }
        case ACMP_LISTENER_RX_TIMEOUT:
        {
            int i = acmp_inflight_timeout_idx[LISTENER];
            avb_1722_1_acmp_inflight_command *inflight = &acmp_listener_inflight_commands[i];
            if (inflight->retried)
            {
                inflight->command.sequence_id = inflight->original_sequence_id;
                // + 7 of the message_type transforms a CONNECT_TX_COMMAND to a CONNECT_RX_RESPONSE etc.
                acmp_send_response(inflight->command.message_type + 7, &inflight->command, ACMP_STATUS_LISTENER_TALKER_TIMEOUT, c_tx);
                // Remove inflight command
                inflight->in_use = 0;

#ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
                simple_printf("ACMP Listener: Removed inflight %d %s with timed out retry - seq id: %d\n",
                        (int)inflight,
                        debug_acmp_message_s[inflight->command.message_type],
                        inflight->command.sequence_id);
#endif
            }
            else
            {
                int message_type = inflight->command.message_type;

                acmp_send_command(LISTENER, message_type, &inflight->command, TRUE, i, c_tx);

#ifdef AVB_1722_1_ACMP_DEBUG_INFLIGHT
                simple_printf("ACMP Listener:  Sent retry for timed out %d %s - seq id: %d\n",
                        (int)inflight,
                        debug_acmp_message_s[inflight->command.message_type],
                        inflight->command.sequence_id);
#endif
            }

            acmp_listener_state = ACMP_LISTENER_WAITING;
            break;
        }

    }
}

#undef CONTROLLER
#undef LISTENER

#undef TRUE
#undef FALSE
