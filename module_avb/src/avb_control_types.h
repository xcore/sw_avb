#ifndef _avb_control_types_h_
#define _avb_control_types_h_

#define __AVB_SOURCE_FORMAT_T__
enum avb_source_format_t 
{
AVB_SOURCE_FORMAT_MBLA_24BIT,
};


#define __AVB_SOURCE_STATE_T__
/** The state of an AVB source.
 **/
enum avb_source_state_t 
{
  AVB_SOURCE_STATE_DISABLED, /*!< The source is disabled and will not transmit. */
  AVB_SOURCE_STATE_POTENTIAL, /*!< The source is enabled and will transmit if
                                a listener requests it */
  AVB_SOURCE_STATE_ENABLED, /*!< The source is enabled and transmitting */
};

#define __AVB_SINK_STATE_T__
enum avb_sink_state_t 
{
AVB_SINK_STATE_DISABLED,
AVB_SINK_STATE_POTENTIAL,
AVB_SINK_STATE_ENABLED,
};

#define __DEVICE_MEDIA_CLOCK_STATE_T__
enum device_media_clock_state_t
{
  DEVICE_MEDIA_CLOCK_STATE_DISABLED,
  DEVICE_MEDIA_CLOCK_STATE_ENABLED
};

#define __DEVICE_MEDIA_CLOCK_TYPE_T__
enum device_media_clock_type_t 
{
  DEVICE_MEDIA_CLOCK_TYPE_PTP,
  DEVICE_MEDIA_CLOCK_TYPE_STREAM,
};

enum avb_update_status_t 
{
  AVB_NO_STATUS,
  AVB_STATUS_UPDATED,
};

typedef enum avb_protocol_t
{
  AVB_SRP=0,
  AVB_MAAP,
  AVB_1722_1
} avb_protocol_t;

typedef enum srp_status_t 
{
  AVB_SRP_OK=0,                  /*!< Status is ok (no change) */
  AVB_SRP_TALKER_ROUTE_FAILED,   /*!< A route from one of the devices sources
                                      has failed to reach an intended listener 
                                      (probably due to lack of bandwidth).
                                 */
  AVB_SRP_LISTENER_ROUTE_FAILED, /*!< A route from to one of the devices sinks
                                      has failed to reach from an
                                      intended talker
                                      (probably due to lack of bandwidth).
                                 */
  AVB_SRP_INDICATION            /*!< One of the SRP indications has occured.
                                    The stream flags will be marked appropriately.
                                */
} srp_status_t;

typedef struct avb_srp
{
  int msg;
  unsigned int streamId[2];
} avb_srp;


typedef enum maap_status_t
{
  AVB_MAAP_ADDRESSES_RESERVED=0,   /*!< Multicast addresses have been 
                                      successfully been reserved after
                                      a called to 
                                      avb_1722_maap_request_addresses().
                                      Use avb_1722_maap_get_base_address() or
                                      avb_1722_maap_get_offset_address()
                                      to access the reserved addresses. */
                                      
  AVB_MAAP_ADDRESSES_LOST,       /*!< Previously reserved multicast
                                      addresses have been lost.
                                      After this event the control thread
                                      should disable any streams using 
                                      MAAP addresses and recall
                                      avb_1722_maap_request_addresses()
                                 */
} maap_status_t;

typedef struct avb_maap
{
  int msg;
} avb_maap;

typedef enum avb_1722_1_status_t
{
  AVB_1722_1_OK,				 /*!< 1722.1 status ok */

  AVB_1722_1_ENTITY_ADDED,		 /*!< An entity has been added to the
                                      database by the discovery protocol */

  AVB_1722_1_ENTITY_REMOVED,  	 /*!< An entity has been removed from the
                                      database by the discovery protocol */

  AVB_1722_1_CONNECT_TALKER,	 /*!< A connection management protocol message
									  has been received indicating that a talker
									  component should initiate a connection */

  AVB_1722_1_DISCONNECT_TALKER,  /*!< An ACMP message indicates that a talker should
									  release a connection */

  AVB_1722_1_CONNECT_LISTENER,	 /*!< An ACMP message indicates that a listener should
									  initiate a connection */

  AVB_1722_1_DISCONNECT_LISTENER /*!< An ACMP message indicates that a listener should
									  release a connection */
} avb_1722_1_status_t;

typedef struct avb_1722_1
{
  int msg;
  int adp_entity_record_id;
  int acmp_listener_info_id;
  int acmp_talker_info_id;
  
} avb_1722_1;

typedef union avb_info_t
{
  avb_srp srp;
  avb_maap maap;
  avb_1722_1 a1722_1;
} avb_info_t;

/** AVB Status Report Type.
 *
 *  This type is modified by avb_periodic(), avb_srp_process_packet()
 *  and avb_1722_maap_process_packet().
 *  and indicates any change in state of the AVB system.
 */
typedef struct avb_status_t
{
  avb_protocol_t type;
  avb_info_t info;
} avb_status_t;


#endif
