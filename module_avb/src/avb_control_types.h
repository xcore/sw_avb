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

/** AVB Status Report Type.
 *
 *  This type is returned by avb_periodic(), avb_srp_process_packet()
 *  and avb_1722_maap_process_packet().
 *  and indicates any change in state of the AVB system.
 */
typedef enum avb_status_t {
  AVB_NO_STATUS=-1,              /*!< No status to report */
  AVB_SRP_OK=0,                  /*!< Status is ok (no change) */
  AVB_SRP_TALKER_ROUTE_FAILED,   /*!< A route from one of the devices sources
                                      has failed to reach an intended listener 
                                      (probably due to lack of bandwidth).
                                      Use avb_srp_get_failed_stream() to
                                      find the streamID of the failed stream.
                                 */
  AVB_SRP_LISTENER_ROUTE_FAILED, /*!< A route from to one of the devices sinks
                                      has failed to reach from an
                                      intended talker
                                      (probably due to lack of bandwidth).
                                      Use avb_srp_get_failed_stream() to
                                      find the streamID of the failed stream.
                                 */
  AVB_MAAP_ADDRESSES_RESERVED,   /*!< Multicast addresses have been 
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
  AVB_1722_1_OK					 /*!< 1722.1 status ok */
} avb_status_t;


#endif
