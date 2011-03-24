Ethernet Client API
===================

.. c:function:: int mac_rx(chanend c_mac,            unsigned char buffer[],            unsigned int  &src_port)
  
    This function receives a complete frame (i.e. src/dest MAC address,
    type & payload),  excluding pre-amble, SoF & CRC32.
  
    :param c_mac: chanend connect to the ethernet server
    :param buffer: The buffer to fill with the incoming packet
    :param src_port: A reference parameter to be filled with the ethernet
                     port the packet came from.
  
    **Returns**:
    
       The number of bytes in the frame.
  
    **Notes**:
      1. This function is a blocking call, 
         (i.e. it will wait until a complete packet is 
         received).
      2. Only the packets whih pass CRC32 are processed.
 
.. c:function:: int mac_rx_timed(chanend c_mac,                  unsigned char buffer[],                  REFERENCE_PARAM(unsigned int, time),                 REFERENCE_PARAM(unsigned int, src_port))
  
    This function receives a complete frame (i.e. src/dest MAC address,
    type & payload),  excluding pre-amble, SoF & CRC32. It also timestamps
    the arrival of the frame
  
    :param c_mac: chanend connect to the ethernet server
    :param buffer: The buffer to fill with the incoming packet
    :param time: A reference parameter to be filled with the timestamp of
                 the packet
    :param src_port: A reference parameter to be filled with the ethernet
                     port the packet came from.
  
    **Returns**:
    
       The number of bytes in the frame.
  
    **Notes**:
      1. This function is a blocking call, 
         (i.e. it will wait until a complete packet is 
         received).
      2. Only the packets whih pass CRC32 are processed.
 

