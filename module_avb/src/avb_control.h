#ifndef _avb_control_h_
#define _avb_control_h_

#ifndef REFERENCE_TO
#ifdef __XC__
#define REFERENCE_TO
#else
#define REFERENCE_TO &
#endif
#endif

#include "avb_control_types.h"


int getset_avb_source_presentation(int set, int h0,REFERENCE_PARAM(int, a0));

/** Get/set the presentation time offset of an AVB source.
 *
 *  Gets/sets the presentation time offset of a source i.e. the 
 *  time after sampling that the stream should be played. The default
 *  value for this is 2ms.
 * 
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num the local source number to set
 *  \param offset the presentation offset in ms
 *
 *
 **/
inline int set_avb_source_presentation(int source_num,int offset)
{return getset_avb_source_presentation(1, source_num,REFERENCE_TO offset);}

inline int get_avb_source_presentation(int source_num,
                                       REFERENCE_PARAM(int, offset))
{return getset_avb_source_presentation(0, source_num,offset);}


int getset_avb_source_map(int set, int h0,int a0[], REFERENCE_PARAM(int, a0_len));

/** Get/set the channel map of an avb source.
 *
 *  Gets/sets the channel map of a source i.e. the list of
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

inline int get_avb_source_map(int source_num,int map[], REFERENCE_PARAM(int, len))
 {return getset_avb_source_map(0, source_num,map, len);}

int getset_avb_source_dest(int set, int h0,unsigned char a0[], REFERENCE_PARAM(int, a0_len));

/** Get/set the destination address of an avb source.
 *
 *  Gets/sets the destination MAC address of a source. 
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num the local source number
 *  \param addr the destination address as an array of 6 bytes
 *  \param len the length of the address, should always be equal to 6
 *
 **/
inline int set_avb_source_dest(int source_num,unsigned char addr[], int len)
{return getset_avb_source_dest(1, source_num,addr, REFERENCE_TO len);}

inline int get_avb_source_dest(int source_num,unsigned char addr[], REFERENCE_PARAM(int, len))
{return getset_avb_source_dest(0, source_num,addr, len);}

int getset_avb_source_format(int set, int h0,REFERENCE_PARAM(enum avb_source_format_t, a0),REFERENCE_PARAM(int, a1));

/** Get/Set the format of an AVB source.
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
inline int set_avb_source_format(int source_num,enum avb_source_format_t format,int rate)
{return getset_avb_source_format(1, source_num,REFERENCE_TO format,REFERENCE_TO rate);}

inline int get_avb_source_format(int source_num,REFERENCE_PARAM(enum avb_source_format_t, format),REFERENCE_PARAM(int, rate))
 {return getset_avb_source_format(0, source_num,format,rate);}


int getset_avb_source_channels(int set, int h0,REFERENCE_PARAM(int, a0));

/** Get/Set the channel count of an AVB source.
 *
 *  Gets/sets the number of channels in the stream.
 *
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *  
 *  \param source_num the local source number
 *  \param n          the number of channels
 */
inline int set_avb_source_channels(int source_num,int n)
{return getset_avb_source_channels(1, source_num,REFERENCE_TO n);}

inline int get_avb_source_channels(int source_num,REFERENCE_PARAM(int, n))
 {return getset_avb_source_channels(0, source_num,n);}


int getset_avb_source_sync(int set, int h0,REFERENCE_PARAM(int, a0));

/** Get/Set the media clock of an AVB source.
 *
 *  Gets/sets the media clock of the stream.
 *  
 *  \param source_num the local source number
 *  \param mclock     the media clock number
 */
inline int set_avb_source_sync(int source_num,int mclock)
{return getset_avb_source_sync(1, source_num,REFERENCE_TO mclock);}

inline int get_avb_source_sync(int source_num,REFERENCE_PARAM(int, mclock))
 {return getset_avb_source_sync(0, source_num,mclock);}

int getset_avb_source_name(int set, int h0,char a0[]);

/** Get/Set the name of an AVB source.
 *
 *  Gets/sets a human readable name for the stream. This name 
 *  must be fewer than ``AVB_MAX_NAME_LEN`` which can be set in ``avb_conf.h``.
 *  
 *  \param source_num the local source number
 *  \param name       the string containing the name
 */
inline int set_avb_source_name(int source_num,char name[])
{return getset_avb_source_name(1, source_num, name);}

inline int get_avb_source_name(int source_num,char name[])
 {return getset_avb_source_name(0, source_num ,name);}

int get_avb_source_id(int h0,unsigned int a0[2]);
int getset_avb_source_vlan(int set, int h0,REFERENCE_PARAM(int, a0));

/** Get/set the destination vlan of an AVB source.
 *
 *  Gets/sets the vlan that the source will transmit on. This defaults 
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

inline int get_avb_source_vlan(int source_num,REFERENCE_PARAM(int, vlan))
 {return getset_avb_source_vlan(0, source_num, vlan);}



int getset_avb_source_state(int set, int h0,REFERENCE_PARAM(enum avb_source_state_t, a0));

/** Get/set the current state of an AVB source.
 *
 *  Gets/sets the current state of an AVB source. You cannot set the
 *  state to ``ENABLED``. Changing the state to ``POTENTIAL`` turns the stream
 *  on and it will automatically change to ``ENABLED`` when connected to 
 *  a listener and streaming.
 *  
 *  \param source_num the local source number
 *  \param state      the state of the source
 */

inline int set_avb_source_state(int source_num,enum avb_source_state_t state)
{return getset_avb_source_state(1, source_num,REFERENCE_TO state);}

inline int get_avb_source_state(int source_num,REFERENCE_PARAM(enum avb_source_state_t, state))
 {return getset_avb_source_state(0, source_num, state);}


int get_avb_sources(REFERENCE_PARAM(int, a0));
int get_avb_sinks(REFERENCE_PARAM(int, a0));
int getset_avb_sink_map(int set, int sink_num,int a0[], REFERENCE_PARAM(int, a0_len));

/** Get/set the map of an AVB sink.
 *
 *  Gets/sets the map i.e. the mapping from the 1722 stream to output FIFOs.
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

inline int get_avb_sink_map(int sink_num,int map[], REFERENCE_PARAM(int, len))
 {return getset_avb_sink_map(0, sink_num,map, len);}

int getset_avb_sink_channels(int set, int sink_num,REFERENCE_PARAM(int, a0));

/** Get/set the number of channels of an AVB sink.
 *
 *  Gets/sets the number of channels of an AVB sink.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 *  \param sink_num the number of the sink
 *  \param n the number of channels
 *
 */
inline int set_avb_sink_channels(int sink_num,int n)
{return getset_avb_sink_channels(1, sink_num,REFERENCE_TO n);}

inline int get_avb_sink_channels(int sink_num,REFERENCE_PARAM(int, n))
 {return getset_avb_sink_channels(0, sink_num,n);}

int getset_avb_sink_sync(int set, int sink_num,REFERENCE_PARAM(int, a0));

/** Get/set the media clock sync of an AVB sink.
 *
 *  Gets/sets which media clock is used to synchronize the incoming stream.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num the number of the sink
 * \param sync     the media clock number of the sink
 *
 */
inline int set_avb_sink_sync(int sink_num,int sync)
{return getset_avb_sink_sync(1, sink_num,REFERENCE_TO sync);}

inline int get_avb_sink_sync(int sink_num,REFERENCE_PARAM(int, sync))
 {return getset_avb_sink_sync(0, sink_num, sync);}


int getset_avb_sink_vlan(int set, int sink_num,REFERENCE_PARAM(int, a0));

/** Get/set the virtual lan id of an AVB sink.
 *
 *  Gets/sets the vlan id of the incoming stream.
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

inline int get_avb_sink_vlan(int sink_num,REFERENCE_PARAM(int, vlan))
 {return getset_avb_sink_vlan(0, sink_num, vlan);}


int getset_avb_sink_addr(int set, int h0,unsigned char a0[], REFERENCE_PARAM(int, a0_len));

/** Get/set the incoming destination mac address of an avb sink.
 *
 *  Get/set the incoming destination mac address of a sink. 
 *  This needs to be set if the address is a multicast address so
 *  the endpoint can register for that multicast group with the switch.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 *  \param sink_num The local sink number
 *  \param addr The mac address as an array of 6 bytes.
 *  \param len The length of the address, should always be equal to 6.
 *
 **/
inline int set_avb_sink_addr(int sink_num,unsigned char addr[], int len)
{return getset_avb_sink_addr(1, sink_num,addr, REFERENCE_TO len);}

inline int get_avb_sink_addr(int sink_num,unsigned char addr[], REFERENCE_PARAM(int, len))
{return getset_avb_sink_addr(0, sink_num,addr, len);}


int getset_avb_sink_name(int set, int sink_num,char a0[]);

/** Get/set the name of an AVB sink.
 *
 *  Gets/sets the name of the sink (to be reported by higher level 
 *  protocols).
 *
 *  \param sink_num the number of the sink
 *  \param name     the name string 
 *
 */
inline int set_avb_sink_name(int sink_num,char name[])
{return getset_avb_sink_name(1, sink_num,name);}

inline int get_avb_sink_name(int sink_num,char name[])
 {return getset_avb_sink_name(0, sink_num,name);}

int getset_avb_sink_id(int set, int sink_num, unsigned int a1[2]);

/** Get/set the stream id that an AVB sink listens to.
 *
 *  Gets/sets the stream id that an AVB sink listens to.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num  the number of the sink
 * \param streamId  int array containing the 64-bit of the 
 *                  stream
 *
 */
inline int set_avb_sink_id(int sink_num, unsigned int streamId[2])
{return getset_avb_sink_id(1, sink_num, streamId);}

inline int get_avb_sink_id(int sink_num, unsigned int streamId[2])
 {return getset_avb_sink_id(0, sink_num, streamId);}

int getset_avb_sink_state(int set, int sink_num,REFERENCE_PARAM(enum avb_sink_state_t, a0));

/** Get/set the state of an AVB sink.
 *
 *  Gets/sets the current state of an AVB sink. You cannot set the
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

inline int get_avb_sink_state(int sink_num,REFERENCE_PARAM(enum avb_sink_state_t, state))
 {return getset_avb_sink_state(0, sink_num,state);}

int getset_media_out_name(int set, int h0,char a0[]);

inline int set_media_out_name(int h0,char a0[])
{return getset_media_out_name(1, h0,a0);}

inline int get_media_out_name(int h0,char a0[])
 {return getset_media_out_name(0, h0,a0);}

int get_media_out_type(int h0,char a0[]);
int get_media_outs(REFERENCE_PARAM(int, a0));
int get_media_ins(REFERENCE_PARAM(int, a0));
int getset_media_in_name(int set, int h0,char a0[]);

inline int set_media_in_name(int h0,char a0[])
{return getset_media_in_name(1, h0,a0);}

inline int get_media_in_name(int h0,char a0[])
 {return getset_media_in_name(0, h0,a0);}

int get_media_in_type(int h0,char a0[]);
int get_device_media_clocks(REFERENCE_PARAM(int, a0));


/** Get the name of the device.
 * 
 *  \param device_name_string array to be filled with the device name
 **/
int get_device_name(char device_name_string[]);

/** Get the name of the system the device is part of.
 * 
 *  \param device_name_string array to be filled with the system name
 **/
int get_device_system(char system_name_string[]);

/** Get the name of the device vendor.
 * 
 *  \param vendor_name_string array to be filled with the vendor name
 **/
int get_device_identity_vendor(char vendor_name_string[]);

/** Get the id of the vendor
 * 
 *  \param vendor_id_string array to be filled with the vendor id
 **/
int get_device_identity_vendor_id(char vendor_id_string[]);

/** Get the name of the product.
 * 
 *  \param product_string array to be filled with the product name
 **/
int get_device_identity_product(char product_string[]);

/** Get the version of the product
 * 
 *  \param version_string array to be filled with the version
 **/
int get_device_identity_version(char version_string[]);

/** Get the serial number of the device.
 * 
 *  \param serial_no_string array to be filled with the serial number
 **/
int get_device_identity_serial(char serial_no_string[]);


int getset_device_media_clock_source(int set, int h0,REFERENCE_PARAM(int, a0));

/** Get/set the source of a media clock.
 *
 *  For clocks that are derived from an output FIFO. This function
 *  gets/sets which FIFO the clock should be derived from.
 * 
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 *
 **/
inline int set_device_media_clock_source(int clock_num, int source)
{return getset_device_media_clock_source(1, clock_num,REFERENCE_TO source);}

inline int get_device_media_clock_source(int clock_num, REFERENCE_PARAM(int, source))
 {return getset_device_media_clock_source(0, clock_num,source);}

int getset_device_media_clock_rate(int set, int clock_num,REFERENCE_PARAM(int, a0));

/** Get/set the rate of a media clock.
 *
 *  Sets the rate of the media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 *
 **/
inline int set_device_media_clock_rate(int clock_num,int rate)
{return getset_device_media_clock_rate(1, clock_num,REFERENCE_TO rate);}

inline int get_device_media_clock_rate(int clock_num,REFERENCE_PARAM(int, rate))
 {return getset_device_media_clock_rate(0, clock_num,rate);}

int getset_device_media_clock_type(int set, int clock_num,REFERENCE_PARAM(enum device_media_clock_type_t, a0));

/** Get/set the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock 
 * 
 **/
inline int set_device_media_clock_type(int clock_num,enum device_media_clock_type_t clock_type)
{return getset_device_media_clock_type(1, clock_num,REFERENCE_TO clock_type);}

inline int get_device_media_clock_type(int clock_num,REFERENCE_PARAM(enum device_media_clock_type_t, clock_type))
 {return getset_device_media_clock_type(0, clock_num,clock_type);}

int getset_device_media_clock_state(int set, int clock_num,REFERENCE_PARAM(enum device_media_clock_state_t, a0));

/** Get/set the state of a media clock.
 *
 *  This function can be used to enabled/disable a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 **/
inline int set_device_media_clock_state(int clock_num,enum device_media_clock_state_t state)
{return getset_device_media_clock_state(1, clock_num,REFERENCE_TO state);}

inline int get_device_media_clock_state(int clock_num,REFERENCE_PARAM(enum device_media_clock_state_t, state))
 {return getset_device_media_clock_state(0, clock_num,state);}


#ifndef __XC__
int get_avb_source_srp_state(int h0, srp_stream_state **st);
int get_avb_sink_srp_state(int h0, srp_stream_state **st);
#endif

int get_avb_ptp_gm(unsigned char a0[], REFERENCE_PARAM(int, a0_len));
int get_avb_ptp_ports(REFERENCE_PARAM(int, a0));
int get_avb_ptp_rateratio(REFERENCE_PARAM(int, a0));
int get_avb_ptp_port_pdelay(int h0,REFERENCE_PARAM(int, a0));

#endif // _avb_control_h_


