#ifndef _avb_api_h_
#define _avb_api_h_
#include <xccompat.h>
#include "avb_control_types.h"

#ifdef __XC__
#define REFERENCE_TO
#else
#define REFERENCE_TO &
#endif

int getset_avb_source_presentation(int set, int source_num,REFERENCE_PARAM(int, a0));

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
inline int set_avb_source_presentation(int source_num,int presentation)
{return getset_avb_source_presentation(1, source_num,REFERENCE_TO presentation);}

/** Get the presentation time offset of an AVB source.
 *  \param source_num       the local source number to set
 *  \param presentation     the presentation offset in ms
 */
inline int get_avb_source_presentation(int source_num,REFERENCE_PARAM(int, presentation))
 {return getset_avb_source_presentation(0, source_num,presentation);}

int getset_avb_source_map(int set, int source_num,int a0[], REFERENCE_PARAM(int, a0_len));

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
inline int set_avb_source_map(int source_num,int map[], int len)
{return getset_avb_source_map(1, source_num,map, REFERENCE_TO len);}

/** Get the channel map of an avb source.
 *  \param source_num the local source number to set
 *  \param map the map, an array of integers giving the input FIFOs that
 *             make up the stream
 *  \param len the length of the map; should be equal to the number of channels
 *             in the stream
 */
inline int get_avb_source_map(int source_num,int map[], REFERENCE_PARAM(int, len))
 {return getset_avb_source_map(0, source_num,map, len);}

int getset_avb_source_dest(int set, int source_num,unsigned char a0[], REFERENCE_PARAM(int, a0_len));

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
inline int set_avb_source_dest(int source_num,unsigned char addr[], int len)
{return getset_avb_source_dest(1, source_num,addr, REFERENCE_TO len);}

/** Get the destination address of an avb source.
 *  \param source_num   the local source number
 *  \param addr         the destination address as an array of 6 bytes
 *  \param len          the length of the address, should always be equal to 6
 */
inline int get_avb_source_dest(int source_num,unsigned char addr[], REFERENCE_PARAM(int, len))
 {return getset_avb_source_dest(0, source_num,addr, len);}

int getset_avb_source_format(int set, int source_num,REFERENCE_PARAM(enum avb_stream_format_t, a0),REFERENCE_PARAM(int, a1));

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
inline int set_avb_source_format(int source_num,enum avb_stream_format_t format,int rate)
{return getset_avb_source_format(1, source_num,REFERENCE_TO format,REFERENCE_TO rate);}

/** Get the format of an AVB source.
 *  \param source_num the local source number
 *  \param format     the format of the stream
 *  \param rate       the sample rate of the stream in Hz
 */
inline int get_avb_source_format(int source_num,REFERENCE_PARAM(enum avb_stream_format_t, format),REFERENCE_PARAM(int, rate))
 {return getset_avb_source_format(0, source_num,format,rate);}

int getset_avb_source_channels(int set, int source_num,REFERENCE_PARAM(int, a0));

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
inline int set_avb_source_channels(int source_num,int channels)
{return getset_avb_source_channels(1, source_num,REFERENCE_TO channels);}

/** Get the channel count of an AVB source.
 *  \param source_num   the local source number
 *  \param channels     the number of channels
 */
inline int get_avb_source_channels(int source_num,REFERENCE_PARAM(int, channels))
 {return getset_avb_source_channels(0, source_num,channels);}

int getset_avb_source_sync(int set, int source_num,REFERENCE_PARAM(int, a0));

/** Set the media clock of an AVB source.
 *
 *  Sets the media clock of the stream.
 *  
 *  \param source_num   the local source number
 *  \param sync         the media clock number
 */
inline int set_avb_source_sync(int source_num,int sync)
{return getset_avb_source_sync(1, source_num,REFERENCE_TO sync);}

/** Get the media clock of an AVB source.
 *  \param source_num   the local source number
 *  \param sync         the media clock number
 */
inline int get_avb_source_sync(int source_num,REFERENCE_PARAM(int, sync))
 {return getset_avb_source_sync(0, source_num,sync);}

int get_avb_source_id(int source_num,unsigned int a0[2]);

int getset_avb_source_vlan(int set, int source_num,REFERENCE_PARAM(int, a0));

int set_avb_source_port(int source_num, int port_num);

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
inline int set_avb_source_vlan(int source_num,int vlan)
{return getset_avb_source_vlan(1, source_num,REFERENCE_TO vlan);}

/** Get the destination vlan of an AVB source.
 *  \param source_num the local source number
 *  \param vlan       the destination vlan id, The media clock number
 */
inline int get_avb_source_vlan(int source_num,REFERENCE_PARAM(int, vlan))
 {return getset_avb_source_vlan(0, source_num,vlan);}

int getset_avb_source_state(int set, int source_num,REFERENCE_PARAM(enum avb_source_state_t, a0));

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
inline int set_avb_source_state(int source_num,enum avb_source_state_t state)
{return getset_avb_source_state(1, source_num,REFERENCE_TO state);}

/** Get the current state of an AVB source.
 *  \param source_num the local source number
 *  \param state      the state of the source
 */
inline int get_avb_source_state(int source_num,REFERENCE_PARAM(enum avb_source_state_t, state))
 {return getset_avb_source_state(0, source_num,state);}

int get_avb_sources(REFERENCE_PARAM(int, a0));
int get_avb_sinks(REFERENCE_PARAM(int, a0));
int get_avb_ptp_gm(unsigned char a0[]);
int get_avb_ptp_ports(REFERENCE_PARAM(int, a0));
int get_avb_ptp_rateratio(REFERENCE_PARAM(int, a0));
int get_avb_ptp_port_pdelay(int h0,REFERENCE_PARAM(unsigned, a0));

int getset_avb_sink_format(int set, int h0,REFERENCE_PARAM(enum avb_stream_format_t, a0),REFERENCE_PARAM(int, a1));

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
inline int set_avb_sink_format(int sink_num,enum avb_stream_format_t format,int rate)
{return getset_avb_sink_format(1, sink_num,REFERENCE_TO format,REFERENCE_TO rate);}


/** Get the format of an AVB sink.
 *  \param sink_num the local sink number
 *  \param format     the format of the stream
 *  \param rate       the sample rate of the stream in Hz
 */
inline int get_avb_sink_format(int sink_num,REFERENCE_PARAM(enum avb_stream_format_t, format),REFERENCE_PARAM(int, rate))
 {return getset_avb_sink_format(0, sink_num,format,rate);}

int getset_avb_sink_channels(int set, int h0,REFERENCE_PARAM(int, a0));

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
inline int set_avb_sink_channels(int sink_num,int channels)
{return getset_avb_sink_channels(1, sink_num,REFERENCE_TO channels);}

/** Get the channel count of an AVB sink.
 *  \param sink_num     the local sink number
 *  \param channels     the number of channels
 */
inline int get_avb_sink_channels(int sink_num,REFERENCE_PARAM(int, channels))
 {return getset_avb_sink_channels(0, sink_num,channels);}

int getset_avb_sink_sync(int set, int sink_num,REFERENCE_PARAM(int, sync));

/** Set the media clock of an AVB sink.
 *
 *  Sets the media clock of the stream.
 *  
 *  \param sink_num   the local sink number
 *  \param sync         the media clock number
 */
inline int set_avb_sink_sync(int sink_num,int sync)
{return getset_avb_sink_sync(1, sink_num,REFERENCE_TO sync);}

/** Get the media clock of an AVB sink.
 *  \param sink_num   the local sink number
 *  \param sync         the media clock number
 */
inline int get_avb_sink_sync(int sink_num,REFERENCE_PARAM(int, sync))
 {return getset_avb_sink_sync(0, sink_num,sync);}

int getset_avb_sink_vlan(int set, int h0,REFERENCE_PARAM(int, a0));

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
inline int set_avb_sink_vlan(int sink_num,int vlan)
{return getset_avb_sink_vlan(1, sink_num,REFERENCE_TO vlan);}

/** Get the virtual lan id of an AVB sink.
 * \param sink_num the number of the sink
 * \param vlan     the vlan id of the sink
 */
inline int get_avb_sink_vlan(int sink_num,REFERENCE_PARAM(int, vlan))
 {return getset_avb_sink_vlan(0, sink_num,vlan);}

int getset_avb_sink_state(int set, int sink_num,REFERENCE_PARAM(enum avb_sink_state_t, state));

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
inline int set_avb_sink_state(int sink_num,enum avb_sink_state_t state)
{return getset_avb_sink_state(1, sink_num,REFERENCE_TO state);}

/** Get the state of an AVB sink.
 * \param sink_num the number of the sink
 * \param state the state of the sink
 */
inline int get_avb_sink_state(int sink_num,REFERENCE_PARAM(enum avb_sink_state_t, state))
 {return getset_avb_sink_state(0, sink_num,state);}

int getset_avb_sink_map(int set, int sink_num,int map[], REFERENCE_PARAM(int, len));

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
inline int set_avb_sink_map(int sink_num,int map[], int len)
{return getset_avb_sink_map(1, sink_num,map, REFERENCE_TO len);}

/** Get the map of an AVB sink.
 * \param sink_num   the number of the sink
 * \param map        array containing the media output FIFOs that the
 *                   stream will be split into
 * \param len        the length of the map; should equal to the number
 *                   of channels in the stream
 */
inline int get_avb_sink_map(int sink_num,int map[], REFERENCE_PARAM(int, len))
 {return getset_avb_sink_map(0, sink_num,map, len);}

int getset_avb_sink_id(int set, int sink_num,unsigned int stream_id[2]);

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
inline int set_avb_sink_id(int sink_num,unsigned int stream_id[2])
{return getset_avb_sink_id(1, sink_num,stream_id);}

/** Get the stream id that an AVB sink listens to.
 * \param sink_num      the number of the sink
 * \param stream_id     int array containing the 64-bit of the stream
 */
inline int get_avb_sink_id(int sink_num,unsigned int stream_id[2])
 {return getset_avb_sink_id(0, sink_num,stream_id);}

int getset_avb_sink_addr(int set, int sink_num,unsigned char addr[], REFERENCE_PARAM(int, len));

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
inline int set_avb_sink_addr(int sink_num,unsigned char addr[], int len)
{return getset_avb_sink_addr(1, sink_num,addr, REFERENCE_TO len);}

/** Get the incoming destination mac address of an avb sink.
 *  \param sink_num     The local sink number
 *  \param addr         The mac address as an array of 6 bytes.
 *  \param len          The length of the address, should always be equal to 6.
 */
inline int get_avb_sink_addr(int sink_num,unsigned char addr[], REFERENCE_PARAM(int, len))
 {return getset_avb_sink_addr(0, sink_num,addr, len);}

int get_media_outs(REFERENCE_PARAM(int, a0));
int get_media_ins(REFERENCE_PARAM(int, a0));

int getset_device_media_clock_source(int set, int clock_num,REFERENCE_PARAM(int, source));

/** Set the source of a media clock.
 *
 *  For clocks that are derived from an output FIFO. This function
 *  gets/sets which FIFO the clock should be derived from.
 * 
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 *
 **/
inline int set_device_media_clock_source(int clock_num,int source)
{return getset_device_media_clock_source(1, clock_num,REFERENCE_TO source);}

/** Get the source of a media clock.
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 */
inline int get_device_media_clock_source(int clock_num,REFERENCE_PARAM(int, source))
 {return getset_device_media_clock_source(0, clock_num,source);}

int getset_device_media_clock_rate(int set, int clock_num,REFERENCE_PARAM(int, rate));

/** Set the rate of a media clock.
 *
 *  Sets the rate of the media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 *
 **/
inline int set_device_media_clock_rate(int clock_num,int rate)
{return getset_device_media_clock_rate(1, clock_num,REFERENCE_TO rate);}

/** Get the rate of a media clock.
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 */
inline int get_device_media_clock_rate(int clock_num,REFERENCE_PARAM(int, rate))
 {return getset_device_media_clock_rate(0, clock_num,rate);}

int getset_device_media_clock_type(int set, int clock_num,REFERENCE_PARAM(enum device_media_clock_type_t, clock_type));

/** Set the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock 
 * 
 **/
inline int set_device_media_clock_type(int clock_num,enum device_media_clock_type_t clock_type)
{return getset_device_media_clock_type(1, clock_num,REFERENCE_TO clock_type);}

/** Get the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock
 */
inline int get_device_media_clock_type(int clock_num,REFERENCE_PARAM(enum device_media_clock_type_t, clock_type))
 {return getset_device_media_clock_type(0, clock_num,clock_type);}

int getset_device_media_clock_state(int set, int clock_num,REFERENCE_PARAM(enum device_media_clock_state_t, a0));

/** Set the state of a media clock.
 *
 *  This function can be used to enabled/disable a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 **/
inline int set_device_media_clock_state(int clock_num,enum device_media_clock_state_t state)
{return getset_device_media_clock_state(1, clock_num,REFERENCE_TO state);}

/** Get the state of a media clock.
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 */
inline int get_device_media_clock_state(int clock_num,REFERENCE_PARAM(enum device_media_clock_state_t, state))
 {return getset_device_media_clock_state(0, clock_num,state);}


#endif // _avb_api_h_
