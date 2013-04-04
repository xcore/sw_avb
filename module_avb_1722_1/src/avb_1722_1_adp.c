#include "avb_1722_common.h"
#include "avb_1722_1_common.h"
#include "avb_1722_1_adp.h"
#include "avb_1722_1_aecp.h"
#include "misc_timer.h"
#include "gptp.h"
#include <string.h>
#include "xccompat.h"
#include <print.h>
#include "simple_printf.h"
#include "avb_1722_1_app_hooks.h"

/* Enumerations for state variables */
static enum { ADP_ADVERTISE_IDLE,
       ADP_ADVERTISE_WAITING,
       ADP_ADVERTISE_ADVERTISE_0,
       ADP_ADVERTISE_ADVERTISE_1,
       ADP_ADVERTISE_DEPARTING,
       ADP_ADVERTISE_DEPART_THEN_ADVERTISE
} adp_advertise_state = ADP_ADVERTISE_IDLE;

static enum { ADP_DISCOVERY_IDLE,
       ADP_DISCOVERY_WAITING,
       ADP_DISCOVERY_DISCOVER,
       ADP_DISCOVERY_ADDED,
       ADP_DISCOVERY_REMOVED
} adp_discovery_state = ADP_DISCOVERY_IDLE;

extern unsigned int avb_1722_1_buf[];
extern guid_t my_guid;
// The GUID whose information we are currently trying to discover
static guid_t discover_guid;

static const unsigned char avb_1722_1_adp_dest_addr[6] = AVB_1722_1_ADP_DEST_MAC;

// The ADP available index counter
static unsigned long avb_1722_1_available_index = 0;

// The GUID for the PTP grandmaster server
static guid_t as_grandmaster_id;

// Timers for various parts of the state machines
static avb_timer adp_advertise_timer;
static avb_timer adp_readvertise_timer;
static avb_timer adp_discovery_timer;
static avb_timer ptp_monitor_timer; // TODO: Remove me

// Counts two second intervals
static unsigned adp_two_second_counter = 0;

// Entity database
avb_1722_1_entity_record entities[AVB_1722_1_MAX_ENTITIES];
static int adp_latest_entity_added_index = -1;


void avb_1722_1_adp_init()
{
    avb_1722_1_entity_database_flush();
    init_avb_timer(&adp_advertise_timer, 1);
    init_avb_timer(&adp_readvertise_timer, 100);
    init_avb_timer(&adp_discovery_timer, 200);
    init_avb_timer(&ptp_monitor_timer, 100);

    adp_advertise_state = ADP_ADVERTISE_IDLE;
    adp_discovery_state = ADP_DISCOVERY_WAITING;
    start_avb_timer(&adp_discovery_timer, 1);
}

void avb_1722_1_adp_announce()
{
    if (adp_advertise_state == ADP_ADVERTISE_IDLE || 
        adp_advertise_state == ADP_ADVERTISE_WAITING)
    {
        adp_advertise_state = ADP_ADVERTISE_ADVERTISE_0;
    }
}

void avb_1722_1_adp_depart()
{
    if (adp_advertise_state == ADP_ADVERTISE_IDLE || 
        adp_advertise_state == ADP_ADVERTISE_WAITING)
    {
        adp_advertise_state = ADP_ADVERTISE_DEPARTING;
    }
}

void avb_1722_1_adp_depart_then_announce()
{
    if (adp_advertise_state == ADP_ADVERTISE_IDLE || 
        adp_advertise_state == ADP_ADVERTISE_WAITING)
    {
        adp_advertise_state = ADP_ADVERTISE_DEPART_THEN_ADVERTISE;
    }    
}

void avb_1722_1_adp_discover(guid_t *guid)
{
    if (adp_discovery_state == ADP_DISCOVERY_WAITING)
    {
        adp_discovery_state = ADP_DISCOVERY_DISCOVER;
        discover_guid.l = guid->l;
    } 
}

void avb_1722_1_adp_discover_all()
{
    guid_t guid;
    guid.l = 0;
    avb_1722_1_adp_discover(&guid);
}

void avb_1722_1_adp_change_ptp_grandmaster(unsigned char grandmaster[8])
{
    memcpy(as_grandmaster_id.c, grandmaster, 8);
}

int avb_1722_1_get_latest_new_entity_idx()
{
    return adp_latest_entity_added_index;
}

int avb_1722_1_entity_database_find(const_guid_ref_t guid)
{
    for (int i=0; i < AVB_1722_1_MAX_ENTITIES; ++i)
    {
        if (entities[i].guid.l == guid->l)
            return i;
    }
    return AVB_1722_1_MAX_ENTITIES;
}

static int avb_1722_1_entity_database_add(avb_1722_1_adp_packet_t* pkt)
{
    guid_t guid;
    int found_slot_index = -1;
    int i;
    int entity_update = 0;

    get_64(guid.c, pkt->entity_guid);

    for (i=0; i < AVB_1722_1_MAX_ENTITIES; ++i)
    {
        if (entities[i].guid.l == 0)
        {
            found_slot_index = i;  // Found an empty entry in the database
        }
        else if (entities[i].guid.l == guid.l)
        {
            // Entity is already in the database - break from loop early and update it
            found_slot_index = i;
            entity_update = 1;
            break;
        }
    }

    if (found_slot_index != -1)
    {
        entities[found_slot_index].guid.l = guid.l;
        entities[found_slot_index].vendor_id = ntoh_32(pkt->vendor_id);
        entities[found_slot_index].entity_model_id = ntoh_32(pkt->entity_model_id);
        entities[found_slot_index].capabilities = ntoh_32(pkt->entity_capabilities);
        entities[found_slot_index].talker_stream_sources = ntoh_16(pkt->talker_stream_sources);
        entities[found_slot_index].talker_capabilities = ntoh_16(pkt->talker_capabilities);
        entities[found_slot_index].listener_stream_sinks = ntoh_16(pkt->listener_stream_sinks);
        entities[found_slot_index].listener_capabilities = ntoh_16(pkt->listener_capabilities);
        entities[found_slot_index].controller_capabilities = ntoh_32(pkt->controller_capabilities);
        entities[found_slot_index].available_index = ntoh_32(pkt->available_index);
        get_64(entities[found_slot_index].as_grandmaster_id.c, pkt->as_grandmaster_id);
        entities[found_slot_index].association_id = ntoh_32(pkt->association_id);
        entities[found_slot_index].timeout = GET_1722_1_VALID_TIME(&pkt->header) + adp_two_second_counter;

        if (entity_update)
        {
            return 0;
        }
        else
        {
            adp_latest_entity_added_index = found_slot_index;
            return 1;
        }
    }
    else
    {
        // TODO: Database full - we should have a scheme for dealing with this
    }

    return 0;
}

void avb_1722_1_entity_database_flush(void)
{
    for (int i=0; i < AVB_1722_1_MAX_ENTITIES; ++i)
    {
        entities[i].guid.l = 0;
    }
}

static void avb_1722_1_entity_database_remove(avb_1722_1_adp_packet_t* pkt)
{
    guid_t guid;
    int i;
    get_64(guid.c, pkt->entity_guid);

    i = avb_1722_1_entity_database_find(&guid);

    if (i != AVB_1722_1_MAX_ENTITIES)
    {
#ifdef AVB_1722_1_ADP_DEBUG_ENTITY_REMOVAL
        printstr("ADP: Removing entity who advertised departing -> GUID "); print_guid_ln(&entities[i].guid);
#endif
        entities[i].guid.l = 0;
    }
}

static unsigned avb_1722_1_entity_database_check_timeout()
{
    int i;

    for (i=0; i < AVB_1722_1_MAX_ENTITIES; ++i)
    {
        if (entities[i].guid.l==0) continue;

        if (entities[i].timeout < adp_two_second_counter)
        {
#ifdef AVB_1722_1_ADP_DEBUG_ENTITY_REMOVAL
            printstr("ADP: Removing entity who timed out -> GUID "); print_guid_ln(&entities[i].guid);
#endif
            entities[i].guid.l=0;
            return 1;
        }
    }
    return 0;
}

void process_avb_1722_1_adp_packet(avb_1722_1_adp_packet_t* pkt, chanend c_tx)
{
    unsigned message_type = GET_1722_1_MSG_TYPE(((avb_1722_1_packet_header_t*)pkt));
    guid_t zero_guid = { 0 };

    switch (message_type)
    {
        case ENTITY_DISCOVER:
        {
            if ( compare_guid(pkt->entity_guid, &my_guid) || compare_guid(pkt->entity_guid, &zero_guid) )
            {
                if (adp_advertise_state == ADP_ADVERTISE_WAITING)
                    adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
            }
            return;
        }
        case ENTITY_AVAILABLE:
        {
            if (avb_1722_1_entity_database_add(pkt))
            {
                /* We only indicate ADP_DISCOVERY_ADDED state if a new (unseen) entity was added,
                 * and not if an existing entity was updated
                 */
                adp_discovery_state = ADP_DISCOVERY_ADDED;
            }
            return;
        }
        case ENTITY_DEPARTING:
        {
            avb_1722_1_entity_database_remove(pkt);
            adp_discovery_state = ADP_DISCOVERY_REMOVED;
            return;
        }
    }
}

static void avb_1722_1_create_adp_packet(int message_type, guid_t guid)
{
    ethernet_hdr_t *hdr = (ethernet_hdr_t*) &avb_1722_1_buf[0];
    avb_1722_1_adp_packet_t *pkt = (avb_1722_1_adp_packet_t*) (hdr + AVB_1722_1_PACKET_BODY_POINTER_OFFSET);

    memset(pkt, 0, sizeof(avb_1722_1_adp_packet_t));

    avb_1722_1_create_1722_1_header(avb_1722_1_adp_dest_addr, DEFAULT_1722_1_ADP_SUBTYPE, message_type,
          (message_type==ENTITY_AVAILABLE)?AVB_1722_1_ADP_VALID_TIME:0, AVB_1722_1_ADP_CD_LENGTH, hdr);

    set_64(pkt->entity_guid, guid.c);

    if (message_type != ENTITY_DISCOVER)
    {
        hton_32(pkt->vendor_id, AVB_1722_1_ADP_VENDOR_ID);
        hton_32(pkt->entity_model_id, AVB_1722_1_ADP_MODEL_ID);
        hton_32(pkt->entity_capabilities, AVB_1722_1_ADP_ENTITY_CAPABILITIES);
        hton_16(pkt->talker_stream_sources, AVB_1722_1_ADP_TALKER_STREAM_SOURCES);
        hton_16(pkt->talker_capabilities, AVB_1722_1_ADP_TALKER_CAPABILITIES);
        hton_16(pkt->listener_stream_sinks, AVB_1722_1_ADP_LISTENER_STREAM_SINKS);
        hton_16(pkt->listener_capabilities, AVB_1722_1_ADP_LISTENER_CAPABILITIES);
        hton_32(pkt->controller_capabilities, AVB_1722_1_ADP_CONTROLLER_CAPABILITIES);
        hton_32(pkt->available_index, avb_1722_1_available_index);
        memcpy(pkt->as_grandmaster_id, as_grandmaster_id.c, 8);
        hton_32(pkt->association_id, AVB_1722_1_ADP_ASSOCIATION_ID);
    }
}

void avb_1722_1_adp_discovery_periodic(chanend c_tx)
{
    switch (adp_discovery_state)
    {
        case ADP_DISCOVERY_IDLE:
            break;

        case ADP_DISCOVERY_WAITING:
        {
            unsigned lost=0;
            if (avb_timer_expired(&adp_discovery_timer))
            {
                adp_two_second_counter++;
                lost = avb_1722_1_entity_database_check_timeout();
                start_avb_timer(&adp_discovery_timer, 1);
            }
            if (lost > 0)
            {
                /* 5.2 TODO: Generate AVB_1722_1_ENTITY_REMOVED */
            }
            break;
        }
        case ADP_DISCOVERY_DISCOVER:
        {
            avb_1722_1_create_adp_packet(ENTITY_DISCOVER, discover_guid);
            mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ADP_PACKET_SIZE, 0);
            adp_discovery_state = ADP_DISCOVERY_WAITING;
            break;
        }
        case ADP_DISCOVERY_ADDED:
        {
            avb_entity_on_new_entity_available(&my_guid, &entities[adp_latest_entity_added_index], c_tx);
            adp_discovery_state = ADP_DISCOVERY_WAITING;
            break;
        }
        case ADP_DISCOVERY_REMOVED:
        {
            adp_discovery_state = ADP_DISCOVERY_WAITING;
            /* 5.2 TODO: Generate VB_1722_1_ENTITY_REMOVED */
            break;
        }
    }
}

void avb_1722_1_adp_advertising_periodic(chanend c_tx, chanend ptp)
{
    guid_t ptp_current;

    switch (adp_advertise_state)
    {
        case ADP_ADVERTISE_IDLE:
            break;

        case ADP_ADVERTISE_WAITING:
            if (avb_timer_expired(&adp_readvertise_timer))
            {
                adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
            }
            break;

        case ADP_ADVERTISE_ADVERTISE_0:
            avb_1722_1_adp_change_ptp_grandmaster(ptp_current.c);
            start_avb_timer(&ptp_monitor_timer, 1); //Every second
            adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
            // Fall through and send immediately

        case ADP_ADVERTISE_ADVERTISE_1:
            avb_1722_1_create_adp_packet(ENTITY_AVAILABLE, my_guid);
            mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ADP_PACKET_SIZE, 0);

            start_avb_timer(&adp_readvertise_timer, AVB_1722_1_ADP_REPEAT_TIME);
            adp_advertise_state = ADP_ADVERTISE_WAITING;
            avb_1722_1_available_index++;
            break;

        case ADP_ADVERTISE_DEPART_THEN_ADVERTISE:
        case ADP_ADVERTISE_DEPARTING:
            avb_1722_1_create_adp_packet(ENTITY_DEPARTING, my_guid);
            mac_tx(c_tx, avb_1722_1_buf, AVB_1722_1_ADP_PACKET_SIZE, 0);

            adp_advertise_state = ADP_ADVERTISE_DEPART_THEN_ADVERTISE ? ADP_ADVERTISE_ADVERTISE_0 : ADP_ADVERTISE_IDLE;
            avb_1722_1_available_index = 0;

            break;

        default:
            break;
    }

    if (ADP_ADVERTISE_IDLE != adp_advertise_state)
    {
        if (avb_timer_expired(&ptp_monitor_timer))
        {
            ptp_get_current_grandmaster(ptp, ptp_current.c);
            if (as_grandmaster_id.l != ptp_current.l)
            {
                avb_1722_1_adp_change_ptp_grandmaster(ptp_current.c);
                adp_advertise_state = ADP_ADVERTISE_ADVERTISE_1;
            }
            start_avb_timer(&ptp_monitor_timer, 1); //Every second
        }
    }
}
