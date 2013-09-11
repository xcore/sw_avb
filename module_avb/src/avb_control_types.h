#ifndef _avb_control_types_h_
#define _avb_control_types_h_

#define __AVB_STREAM_FORMAT_T__
enum avb_stream_format_t
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
  DEVICE_MEDIA_CLOCK_INPUT_STREAM_DERIVED,
  DEVICE_MEDIA_CLOCK_LOCAL_CLOCK
};

#endif
