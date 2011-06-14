#ifndef _avb_control_h_
#define _avb_control_h_

/*
 *  This file is for the benefit of Doxygen - don't remove the predeclarations.
 */



/** \fn set_avb_source_presentation
 *
 *  Set the presentation time offset of an AVB source.
 *
 *  Sets the presentation time offset of a source i.e. the
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
void set_avb_source_presentation(unsigned source_num, unsigned offset);

/** \fn get_avb_source_presentation
 *
 */
void get_avb_source_presentation(unsigned source_num, unsigned& offset);

/** \fn set_avb_source_map
 *
 *  Set the channel map of an avb source.
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
void set_avb_source_map(unsigned source_num, unsigned map[], unsigned len);

/** \fn get_avb_source_map
 *
 */
void get_avb_source_map(unsigned source_num, unsigned map[], unsigned& len);

/** \fn set_avb_source_dest
 *
 *  Set the destination address of an avb source.
 *
 *  Sets the destination MAC address of a source.
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *
 *  \param source_num the local source number
 *  \param addr the destination address as an array of 6 bytes
 *  \param len the length of the address, should always be equal to 6
 *
 **/
void set_avb_source_dest(unsigned source_num, char addr[], unsigned len);

/** \fn set_avb_source_dest
 *
 */
void get_avb_source_dest(unsigned source_num, char addr[], unsigned& len);

/** \fn set_avb_source_format
 *
 *  Set the format of an AVB source.
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
void set_avb_source_format(unsigned source_num, unsigned format, unsigned rate);

/** \fn get_avb_source_format
 *
 */
void get_avb_source_format(unsigned source_num, unsigned format, unsigned& rate);

/** \fn set_avb_source_channels
 *
 *  Set the channel count of an AVB source.
 *
 *  Sets the number of channels in the stream.
 *
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *  
 *  \param source_num the local source number
 *  \param n          the number of channels
 */
void set_avb_source_channels(unsigned source_num, unsigned n);

/** \fn get_avb_source_channels
 */
void get_avb_source_channels(unsigned source_num, unsigned& n);

/** \fn set_avb_source_sync
 *
 *  Set the media clock of an AVB source.
 *
 *  Sets the media clock of the stream.
 *  
 *  \param source_num the local source number
 *  \param mclock     the media clock number
 */
void set_avb_source_sync(unsigned source_num, unsigned mclock);

/** \fn get_avb_source_sync
 */
void get_avb_source_sync(unsigned source_num, unsigned& mclock);

/** \fn set_avb_source_name
 *
 *  Set the name of an AVB source.
 *
 *  Sets a human readable name for the stream. This name
 *  must be fewer than ``AVB_MAX_NAME_LEN`` which can be set in ``avb_conf.h``.
 *  
 *  \param source_num the local source number
 *  \param name       the string containing the name
 */
void set_avb_source_name(unsigned source_num, char name[]);

/** \fn get_avb_source_name
 */
void get_avb_source_name(unsigned source_num, char name[]);

/** \fn set_avb_source_vlan
 *
 *  Set the destination vlan of an AVB source.
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
void set_avb_source_vlan(unsigned source_num, unsigned vlan);

/** \fn get_avb_source_vlan
 */
void get_avb_source_vlan(unsigned source_num, unsigned& vlan);

/** \fn set_avb_source_state
 *
 *  Set the current state of an AVB source.
 *
 *  Sets the current state of an AVB source. You cannot set the
 *  state to ``ENABLED``. Changing the state to ``POTENTIAL`` turns the stream
 *  on and it will automatically change to ``ENABLED`` when connected to 
 *  a listener and streaming.
 *  
 *  \param source_num the local source number
 *  \param state      the state of the source
 */
void set_avb_source_state(unsigned source_num, avb_source_state_t state);

/** \fn get_avb_source_state
 */
void get_avb_source_state(unsigned source_num, avb_source_state_t& state);

/** \fn set_avb_sink_map
 *
 *  Set the map of an AVB sink.
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
void set_avb_sink_map(unsigned sink_num, unsigned map[], unsigned len);

/** \fn get_avb_sink_map
 */
void get_avb_sink_map(unsigned sink_num, unsigned map[], unsigned& len);

/** \fn set_avb_sink_channels
 *
 *  Set the number of channels of an AVB sink.
 *
 *  Sets the number of channels of an AVB sink.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 *  \param sink_num the number of the sink
 *  \param n the number of channels
 *
 */
void set_avb_sink_channels(unsigned sink_num, unsigned n);

/** \fn get_avb_sink_channels
 */
void get_avb_sink_channels(unsigned sink_num, unsigned& n);

/** \fn set_avb_sink_sync
 *
 *  Set the media clock sync of an AVB sink.
 *
 *  Sets which media clock is used to synchronize the incoming stream.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num the number of the sink
 * \param sync     the media clock number of the sink
 *
 */
void set_avb_sink_sync(unsigned sink_num, unsigned sync);

/** \fn get_avb_sink_sync
 */
void get_avb_sink_sync(unsigned sink_num, unsigned& sync);

/** \fn set_avb_sink_vlan
 *
 *  Set the virtual lan id of an AVB sink.
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
void set_avb_sink_vlan(unsigned sink_num, unsigned vlan);

/** \fn get_avb_sink_vlan
 */
void get_avb_sink_vlan(unsigned sink_num, unsigned& vlan);

/** \fn set_avb_sink_addr
 *
 *  Set the incoming destination mac address of an avb sink.
 *
 *  Set the incoming destination mac address of a sink.
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
void set_avb_sink_addr(unsigned sink_num, char addr[], unsigned len);

/** \fn get_avb_sink_addr
 */
void get_avb_sink_addr(unsigned sink_num, char addr[], unsigned& len);

/** \fn set_avb_sink_name
 *
 *  Set the name of an AVB sink.
 *
 *  Sets the name of the sink (to be reported by higher level
 *  protocols).
 *
 *  \param sink_num the number of the sink
 *  \param name     the name string 
 *
 */
void set_avb_sink_name(unsigned sink_num, char name[]);

/** \fn get_avb_sink_name
 */
void get_avb_sink_name(unsigned sink_num, char name[]);

/** \fn set_avb_sink_id
 *
 *  Set the stream id that an AVB sink listens to.
 *
 *  Sets the stream id that an AVB sink listens to.
 *
 *  This setting will not take effect until the next time the sink
 *  state moves from disabled to potential.
 *
 * \param sink_num  the number of the sink
 * \param streamId  int array containing the 64-bit of the 
 *                  stream
 *
 */
void set_avb_sink_id(unsigned sink_num, unsigned streamId[]);

/** \fn get_avb_sink_id
 */
void get_avb_sink_id(unsigned sink_num, unsigned streamId[]);

/** \fn set_avb_sink_state
 *
 *  Set the state of an AVB sink.
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
void set_avb_sink_state(unsigned sink_num, avb_sink_state_t state);

/** \fn get_avb_sink_state
 *
 */
void get_avb_sink_state(unsigned sink_num, avb_sink_state_t state);

/** \fn get_device_name
 *
 *  Get the name of the device.
 * 
 *  \param device_name_string array to be filled with the device name
 **/
int get_device_name(char device_name_string[]);

/** \fn get_device_system
 *
 *  Get the name of the system the device is part of.
 * 
 *  \param device_name_string array to be filled with the system name
 **/
int get_device_system(char device_name_string[]);

/** \fn get_device_identity_vendor
 *
 *  Get the name of the device vendor.
 * 
 *  \param vendor_name_string array to be filled with the vendor name
 **/
int get_device_identity_vendor(char vendor_name_string[]);

/** \fn get_device_identity_vendor_id
 *
 *  Get the id of the vendor
 * 
 *  \param vendor_id_string array to be filled with the vendor id
 **/
int get_device_identity_vendor_id(char vendor_id_string[]);

/** \fn get_device_identity_product
 *
 *  Get the name of the product.
 * 
 *  \param product_string array to be filled with the product name
 **/
int get_device_identity_product(char product_string[]);

/** \fn get_device_identity_version
 *
 *  Get the version of the product
 * 
 *  \param version_string array to be filled with the version
 **/
int get_device_identity_version(char version_string);

/** \fn get_device_identity_serial
 *
 *  Get the serial number of the device.
 * 
 *  \param serial_no_string array to be filled with the serial number
 **/
int get_device_identity_serial(char serial_no_string[]);

/** \fn set_device_media_clock_source
 *
 *  Set the source of a media clock.
 *
 *  For clocks that are derived from an output FIFO. This function
 *  gets/sets which FIFO the clock should be derived from.
 * 
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 *
 **/
int set_device_media_clock_source(unsigned clock_num, device_media_clock_state_t source);

/** \fn set_device_media_clock_rate
 *
 *  Set the rate of a media clock.
 *
 *  Sets the rate of the media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 *
 **/
int set_device_media_clock_rate(int clock_num, int rate);

/** \fn set_device_media_clock_type
 *
 *  Set the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock 
 * 
 **/
int set_device_media_clock_type(int clock_num, device_media_clock_type_t clock_type);

/** \fn get_device_media_clock_type
 *
 *  Get the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock
 */
int get_device_media_clock_type(int clock_num, device_media_clock_type_t& clock_type);

/** \fn set_device_media_clock_state
 *
 *  Get/set the state of a media clock.
 *
 *  This function can be used to enabled/disable a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 **/
int set_device_media_clock_state(int clock_num, device_media_clock_state_t state);


#endif // _avb_control_h_


