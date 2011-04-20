#ifndef _avb_control_h_
#define _avb_control_h_

/** \fn set_avb_source_presentation
 *
 *  Get/set the presentation time offset of an AVB source.
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

/** \fn set_avb_source_map
 *
 *  Get/set the channel map of an avb source.
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

/** \fn set_avb_source_dest
 *
 *  Get/set the destination address of an avb source.
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

/** \fn set_avb_source_format
 *
 *  Get/Set the format of an AVB source.
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

/** \fn set_avb_source_channels
 *
 *  Get/Set the channel count of an AVB source.
 *
 *  Gets/sets the number of channels in the stream.
 *
 *  This setting will not take effect until the next time the source
 *  state moves from disabled to potential.
 *  
 *  \param source_num the local source number
 *  \param n          the number of channels
 */

/** \fn set_avb_source_sync
 *
 *  Get/Set the media clock of an AVB source.
 *
 *  Gets/sets the media clock of the stream.
 *  
 *  \param source_num the local source number
 *  \param mclock     the media clock number
 */

/** \fn set_avb_source_name
 *
 *  Get/Set the name of an AVB source.
 *
 *  Gets/sets a human readable name for the stream. This name 
 *  must be fewer than ``AVB_MAX_NAME_LEN`` which can be set in ``avb_conf.h``.
 *  
 *  \param source_num the local source number
 *  \param name       the string containing the name
 */

/** \fn set_avb_source_vlan
 *
 *  Get/set the destination vlan of an AVB source.
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

/** \fn set_avb_source_state
 *
 *  Get/set the current state of an AVB source.
 *
 *  Gets/sets the current state of an AVB source. You cannot set the
 *  state to ``ENABLED``. Changing the state to ``POTENTIAL`` turns the stream
 *  on and it will automatically change to ``ENABLED`` when connected to 
 *  a listener and streaming.
 *  
 *  \param source_num the local source number
 *  \param state      the state of the source
 */

/** \fn set_avb_sink_map
 *
 *  Get/set the map of an AVB sink.
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

/** \fn set_avb_sink_channels
 *
 *  Get/set the number of channels of an AVB sink.
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

/** \fn set_avb_sink_sync
 *
 *  Get/set the media clock sync of an AVB sink.
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

/** \fn set_avb_sink_vlan
 *
 *  Get/set the virtual lan id of an AVB sink.
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

/** \fn set_avb_sink_addr
 *
 *  Get/set the incoming destination mac address of an avb sink.
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

/** \fn set_avb_sink_name
 *
 *  Get/set the name of an AVB sink.
 *
 *  Gets/sets the name of the sink (to be reported by higher level 
 *  protocols).
 *
 *  \param sink_num the number of the sink
 *  \param name     the name string 
 *
 */

/** \fn set_avb_sink_id
 *
 *  Get/set the stream id that an AVB sink listens to.
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

/** \fn set_avb_sink_state
 *
 *  Get/set the state of an AVB sink.
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

/** \fn get_device_name
 *
 *  Get the name of the device.
 * 
 *  \param device_name_string array to be filled with the device name
 **/

/** \fn get_device_system
 *
 *  Get the name of the system the device is part of.
 * 
 *  \param device_name_string array to be filled with the system name
 **/

/** \fn get_device_identity_vendor
 *
 *  Get the name of the device vendor.
 * 
 *  \param vendor_name_string array to be filled with the vendor name
 **/

/** \fn get_device_identity_vendor_id
 *
 *  Get the id of the vendor
 * 
 *  \param vendor_id_string array to be filled with the vendor id
 **/

/** \fn get_device_identity_product
 *
 *  Get the name of the product.
 * 
 *  \param product_string array to be filled with the product name
 **/

/** \fn get_device_identity_version
 *
 *  Get the version of the product
 * 
 *  \param version_string array to be filled with the version
 **/

/** \fn get_device_identity_serial
 *
 *  Get the serial number of the device.
 * 
 *  \param serial_no_string array to be filled with the serial number
 **/

/** \fn set_device_media_clock_source
 *
 *  Get/set the source of a media clock.
 *
 *  For clocks that are derived from an output FIFO. This function
 *  gets/sets which FIFO the clock should be derived from.
 * 
 *  \param clock_num the number of the media clock
 *  \param source the output FIFO number to base the clock on
 *
 **/

/** \fn set_device_media_clock_rate
 *
 *  Get/set the rate of a media clock.
 *
 *  Sets the rate of the media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param rate the rate of the clock in Hz
 *
 **/

/** \fn set_device_media_clock_type
 *
 *  Get/set the type of a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param clock_type the type of the clock 
 * 
 **/

/** \fn set_device_media_clock_state
 *
 *  Get/set the state of a media clock.
 *
 *  This function can be used to enabled/disable a media clock.
 *
 *  \param clock_num the number of the media clock
 *  \param state the state of the clock
 **/

#endif // _avb_control_h_


