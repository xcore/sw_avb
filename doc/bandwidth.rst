IEEE 1722 Bandwidth Usage
=========================

The AVB standard requires audio data to be split into packets to be
transmitted over ethernet along with meta-information specified by the
IEEE 1722 transport protocol. This meta-information incurs an
overhead on the bandwidth of each stream of data. 

The protocol overhead is detailed in the following table:

.. list-table::
 :header-rows: 1

 * - Protocol
   - Overhead (bytes)
 * - Interframe gap
   - 20
 * - Ethernet header
   - 18
 * - IEEE 1722 header
   - 24
 * - 61883-6 header
   - 8
 * - CRC
   - 4

Each stream of audio data can contain several multiplexed channels of
audio. The higher the number of channels per stream, the more
efficient the audio transport is in terms of bandwidth. Note that the
IEC 61883-6 standard recommends transmitting single channel
streams as stereo with the right channel blank.

After the header in each packet, audio data is stored in AM824 format
which pads 24-bit data to 4 byte quadlets. The frame rate is 8kHz so
a 48kHz stream has 6 samples per packet, a 96kHz has 12
samples per packet and so on.

The following table shows the bandwidth for streams at
different sample rates with different number of channels per
stream. 

.. list-table::
 :header-rows: 1

 * - Sample Rate (kHz)
   - Channels/Stream
   - Mbps

 * - 48  
   - 1 
   - 7.81   
 * - 48  
   - 2       
   - 7.81   
 * - 48  
   - 4 
   - 10.88   
 * - 48  
   - 8 
   - 17.02  
 * - 48  
   - 16 
   - 29.31 
 * - 48  
   - 32 
   - 53.89 
 * - 96  
   - 1 
   - 10.88   
 * - 96  
   - 2 
   - 10.88  
 * - 96  
   - 4 
   - 17.02  
 * - 96  
   - 8 
   - 29.31  
 * - 96  
   - 16 
   - 53.89 


Note that the higher the number of channels per stream, the better the
bandwidth usage (e.g. 4 x 8 channels streams uses less
bandwidth than 16 x 2 channel streams). 
The IEEE 1722 standard also specifies that only
75% of available bandwidth can be used for AVB traffic. 
