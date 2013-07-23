#ifndef _api_h_
#define _api_h_
// #include <xccompat.h>
#include "avb_control_types.h"

#ifdef __XC__
interface avb_interface {
/** Set the presentation time offset of an AVB source.
 *
 *  Sets the presentation time offset of a source i.e. the
 *  time after sampling that the stream should be played. The default
 *  value for this is 2ms.
 * 
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num       the local source number to set
 *  \param presentation     the presentation offset in ms
 *
 *
 **/
  int set_source_presentation(int source_num, int presentation);

/** Get the presentation time offset of an AVB source.
 *  \param source_num       the local source number to set
 *  \param presentation     the presentation offset in ms
 */
  int get_source_presentation(int source_num, int &presentation);

/** Set the channel map of an avb source.
 *
 *  Sets the channel map of a source i.e. the list of
 *  input FIFOs that constitute the stream.
 *
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num the local source number to set
 *  \param map the map, an array of integers giving the input FIFOs that
 *             make up the stream
 *  \param len the length of the map; should be equal to the number of channels
 *             in the stream
 *
 **/
  int set_source_map(int source_num, int map[len], int len);

/** Get the channel map of an avb source.
 *  \param source_num the local source number to set
 *  \param map the map, an array of integers giving the input FIFOs that
 *             make up the stream
 *  \param len the length of the map; should be equal to the number of channels
 *             in the stream
 */
  int get_source_map(int source_num, int map[], int &len);

/** Set the destination address of an avb source.
 *
 *  Sets the destination MAC address of a source.
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num   the local source number
 *  \param addr         the destination address as an array of 6 bytes
 *  \param len          the length of the address, should always be equal to 6
 *
 **/
  int set_source_dest(int source_num, unsigned char addr[], int len);

/** Get the destination address of an avb source.
 *  \param source_num   the local source number
 *  \param addr         the destination address as an array of 6 bytes
 *  \param len          the length of the address, should always be equal to 6
 */
  int get_source_dest(int source_num, unsigned char addr[], int &len);

/** Set the format of an AVB source.
 *
 *  The AVB source format covers the encoding and sample rate of the source.
 *  Currently the format is limited to a single encoding MBLA 24 bit signed
 *  integers.
 *  
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num the local source number
 *  \param format     the format of the stream
 *  \param rate       the sample rate of the stream in Hz
 */
  int set_source_format(int source_num, enum avb_stream_format_t format, int rate);

/** Get the format of an AVB source.
 *  \param source_num the local source number
 *  \param format     the format of the stream
 *  \param rate       the sample rate of the stream in Hz
 */
  int get_source_format(int source_num, enum avb_stream_format_t &format, int &rate);

/** Set the channel count of an AVB source.
 *
 *  Sets the number of channels in the stream.
 *
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *  
 *  \param source_num   the local source number
 *  \param channels     the number of channels
 */
  int set_source_channels(int source_num, int channels);

/** Get the channel count of an AVB source.
 *  \param source_num   the local source number
 *  \param channels     the number of channels
 */
  int get_source_channels(int source_num, int &channels);

/** Set the media clock of an AVB source.
 *
 *  Sets the media clock of the stream.
 *  
 *  \param source_num   the local source number
 *  \param sync         the media clock number
 */
  int set_source_sync(int source_num, int sync);

/** Get the media clock of an AVB source.
 *  \param source_num   the local source number
 *  \param sync         the media clock number
 */
  int get_source_sync(int source_num, int &sync);

  int set_source_port(int source_num, int port_num);

/** Set the destination vlan of an AVB source.
 *
 *  Sets the vlan that the source will transmit on. This defaults
 *  to 2.
 *
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num the local source number
 *  \param vlan       the destination vlan id, The media clock number
 */
  int set_source_vlan(int source_num, int vlan);

/** Get the destination vlan of an AVB source.
 *  \param source_num the local source number
 *  \param vlan       the destination vlan id, The media clock number
 */
  int get_source_vlan(int source_num, int &vlan);

/** Set the current state of an AVB source.
 *
 *  Sets the current state of an AVB source. You cannot set the
 *  state to ``ENABLED``. Changing the state to ``AVB_SOURCE_STATE_POTENTIAL`` turns the stream
 *  on and it will automatically change to ``ENABLED`` when connected to 
 *  a listener and streaming.
 *  
 *  \param source_num the local source number
 *  \param state      the state of the source
 */
  int set_source_state(int source_num, enum avb_source_state_t state);

/** Get the current state of an AVB source.
 *  \param source_num the local source number
 *  \param state      the state of the source
 */
  int get_source_state(int source_num, enum avb_source_state_t &state);

  int get_source_id(int source_num, unsigned int id[2]);

#if 0 
  int get_ptp_gm(unsigned char id[8]);
  int get_ptp_ports(int *a0);
  int get_ptp_rateratio(int *ratio);
  int get_ptp_port_pdelay(int port, unsigned *delay);
#endif

  /** Set the format of an AVB sink.
 *
 *  The AVB sink format covers the encoding and sample rate of the sink.
 *  Currently the format is limited to a single encoding MBLA 24 bit signed
 *  integers.
 *  
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 *  \param sink_num     the local sink number
 *  \param format       the format of the stream
 *  \param rate         the sample rate of the stream in Hz
 */
 int set_sink_format(int sink_num, enum avb_stream_format_t format, int rate);

 /** Get the format of an AVB sink.
 *  \param sink_num the local sink number
 *  \param format     the format of the stream
 *  \param rate       the sample rate of the stream in Hz
 */
  int get_sink_format(int sink_num, enum avb_stream_format_t &format, int &rate);

/** Set the channel count of an AVB sink.
 *
 *  Sets the number of channels in the stream.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *  
 *  \param sink_num     the local sink number
 *  \param channels     the number of channels
 */
  int set_sink_channels(int sink_num, int channels);

/** Get the channel count of an AVB sink.
 *  \param sink_num     the local sink number
 *  \param channels     the number of channels
 */
  int get_sink_channels(int sink_num, int &channels);

/** Set the media clock of an AVB sink.
 *
 *  Sets the media clock of the stream.
 *  
 *  \param sink_num   the local sink number
 *  \param sync         the media clock number
 */
  int set_sink_sync(int sink_num, int sync);

/** Get the media clock of an AVB sink.
 *  \param sink_num   the local sink number
 *  \param sync         the media clock number
 */
  int get_sink_sync(int sink_num, int &sync);


/** Set the virtual lan id of an AVB sink.
 *
 *  Sets the vlan id of the incoming stream.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num the number of the sink
 * \param vlan     the vlan id of the sink
 *
 */
  int set_sink_vlan(int sink_num, int vlan);

/** Get the virtual lan id of an AVB sink.
 * \param sink_num the number of the sink
 * \param vlan     the vlan id of the sink
 */
  int get_sink_vlan(int sink_num, int &vlan);

/** Set the state of an AVB sink.
 *
 *  Sets the current state of an AVB sink. You cannot set the
 *  state to ``ENABLED``. Changing the state to ``POTENTIAL`` turns the stream
 *  on and it will automatically change to ``ENABLED`` when connected to 
 *  a talker and receiving samples.
 *
 * \param sink_num the number of the sink
 * \param state the state of the sink
 *
 */
  int set_sink_state(int sink_num, enum avb_sink_state_t state);

/** Get the state of an AVB sink.
 * \param sink_num the number of the sink
 * \param state the state of the sink
 */
  int get_sink_state(int sink_num, enum avb_sink_state_t &state);

/** Set the map of an AVB sink.
 *
 *  Sets the map i.e. the mapping from the 1722 stream to output FIFOs.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num   the number of the sink
 * \param map        array containing the media output FIFOs that the 
 *                   stream will be split into
 * \param len        the length of the map; should equal to the number
 *                   of channels in the stream
 */
  int set_sink_map(int sink_num, int map[len], int len);

/** Get the map of an AVB sink.
 * \param sink_num   the number of the sink
 * \param map        array containing the media output FIFOs that the
 *                   stream will be split into
 * \param len        the length of the map; should equal to the number
 *                   of channels in the stream
 */
  int get_sink_map(int sink_num, int map[], int &len);

/** Set the stream id that an AVB sink listens to.
 *
 *  Sets the stream id that an AVB sink listens to.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num      the number of the sink
 * \param stream_id     int array containing the 64-bit of the stream
 *
 */
  int set_sink_id(int sink_num, unsigned int stream_id[2]);

/** Get the stream id that an AVB sink listens to.
 * \param sink_num      the number of the sink
 * \param stream_id     int array containing the 64-bit of the stream
 */
  int get_sink_id(int sink_num, unsigned int stream_id[2]);

/** Set the incoming destination mac address of an avb sink.
 *
 *  Set the incoming destination mac address of a sink.
 *  This needs to be set if the address is a multicast address so
 *  the endpoint can register for that multicast group with the switch.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 *  \param sink_num     The local sink number
 *  \param addr         The mac address as an array of 6 bytes.
 *  \param len          The length of the address, should always be equal to 6.
 *
 **/
  int set_sink_addr(int sink_num, unsigned char addr[len], int len);

/** Get the incoming destination mac address of an avb sink.
 *  \param sink_num     The local sink number
 *  \param addr         The mac address as an array of 6 bytes.
 *  \param len          The length of the address, should always be equal to 6.
 */
  int get_sink_addr(int sink_num, unsigned char addr[], int &len);

/** Set the source of a media clock.
 *
 *  For clocks that are derived from an output FIFO. This function
 *  gets/sets which FIFO the clock should be derived from.
 * 
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 *
 **/
  int set_device_media_clock_source(int clock_num, int source);

/** Get the source of a media clock.
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 */
  int get_device_media_clock_source(int clock_num, int &source);

/** Set the rate of a media clock.
 *
 *  Sets the rate of the media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 *
 **/
  int set_device_media_clock_rate(int clock_num, int rate);

/** Get the rate of a media clock.
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 */
  int get_device_media_clock_rate(int clock_num, int &rate);

/** Set the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock 
 * 
 **/
  int set_device_media_clock_type(int clock_num, enum device_media_clock_type_t clock_type);

/** Get the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock
 */
  int get_device_media_clock_type(int clock_num, enum device_media_clock_type_t &clock_type);

/** Set the state of a media clock.
 *
 *  This function can be used to enabled/disable a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 **/
  int set_device_media_clock_state(int clock_num, enum device_media_clock_state_t state);

/** Get the state of a media clock.
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 */
  int get_device_media_clock_state(int clock_num, enum device_media_clock_state_t &state);
};
#endif

#endif // _api_h_
