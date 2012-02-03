.. _sec_resource:

Resource Usage
--------------

Available Chip Resources
++++++++++++++++++++++++

Each XMOS device has a set of resources detailed in the following
table. The resources are split amongst different cores on the device
which may affect how resources can be used:

.. list-table::
 :header-rows: 1
 :widths: 14 10 10 10 10

 * - Device
   - Threads
   - MIPS
   - Memory (KB)
   - Ports
 * - XS1-G4-512BGA
   - 32
   - 1600
   - 256
   - | 64 x 1bit
     | 16 x 4bit
     | 8 x 16bit 
     | 4 x 32bit  
 * - XS1-L2-124QFN-C5
   - 16
   - 500
   - 128
   - | 16 x 1bit
     | 6 x 4bit
     | 4 x 8bit
     | 2 x 16bit
 * - XS1-L2-124QFN-C4
   - 16
   - 400
   - 128
   - | 16 x 1bit
     | 6 x 4bit
     | 4 x 8bit
     | 2 x 16bit

.. note::
 
   Note that some ports overlap on the device so, for example,
   using a 32 bit port makes some 1 bit ports unavailable. See
   the device datasheets for details.

The following sections detail the resource required for each
component. Please note that the memory requirements for code size
should be taken as a rough guide since exact memory usage depends
on the integration of components (which components are on which
core etc.) in the final build of the application.

Ethernet Component
++++++++++++++++++

Each endpoint requires a driver for the ethernet PHY.

.. list-table::
  :header-rows: 1

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory (KB)           
    - Ports
  * - Ethernet 
    - 5
    - 50 
    - | 15 code
      | 1.5 per buffer
    - | 6 x 1bit 
      | 2 x 4bit

PTP Component
+++++++++++++

Every AVB endpoint must include a PTP component.

.. list-table::
  :header-rows: 1

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory (KB)           
    - Ports
  * - PTP
    - 1
    - 50
    - 7
    - None


Media Clock Server
++++++++++++++++++

Every AVB endpoint must include a media clock server.

.. list-table::
  :header-rows: 1

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory (KB)
    - Ports
  * - Media Clock Server
    - 1
    - 50
    - 1
    - None

If the endpoint drives an external PLL, a PLL driver component
is required.

.. list-table::
  :header-rows: 1

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory (KB)           
    - Ports
  * - PLL driver
    - 1
    - 50
    - 0.5
    - | 1 x 1bit 
      | + ports to configure PLL


Audio Component(s)
++++++++++++++++++

Each endpoint may have several listener and talker components. Each
listener/talker component is capable of handling eight IEEE P1722
streams and up to 24 channels of audio.

.. list-table::
  :header-rows: 1
  :widths: 14 8 12 12 10

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory (KB)           
    - Ports
  * - 1722 listener unit
    - 1
    - 50
    - 5
    - None
  * - 1722 talker unit
    - 1
    - 50
    - 5
    - None

The amount of resource required for audio processing depends on the
interface and the number of audio channels required. The overheads
for the interface are:


.. list-table::
  :header-rows: 1
  :widths: 11 8 12 11 18

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory(KB)            
    - Ports
  * - I2S
    - 1
    - 50
    - 0.5
    - | 3 x 1bit 
      | 1 x 1bit per stereo channel
  * - TDM
    - 1
    - 50
    - 0.5
    - | 3 x 1bit 
      | 1 x 1bit per 8 channels

The following table shows that number of channels an interface can
handle per thread:

.. list-table::
  :header-rows: 1

  * - Component 
    - Sample Rate (kHz)
    - Channels
  * - I2S
    - 44.1/48       
    - 8 in and 8 out
  * - I2S
    - 88.2/96       
    - 4 in and 4 out
  * - TDM
    - 48       
    - 8 in and 8 out


Note that several instances of the audio interface component
can be made *e.g.* you could use 2 threads to handle 16 channels 
of I2S. The following table shows how much buffering
memory is required depending on the number of audio channels.

.. list-table::
 :header-rows: 1

 * - Sample Rate (kHz)
   - Audio Channels
   - Memory (KB)
 * - 44.1
   - n in/m out
   - 0.5 x (n+m)
 * - 48
   - n in/m out
   - 0.5 x (n+m)
 * - 88.2
   - n in/m out
   - 1 x (n+m)
 * - 96
   - n in/m out
   - 1 x (n+m)

TCP Stack
+++++++++

The uIP IP/UDP/TCP stack requires the following resource.


.. list-table::
  :header-rows: 1

  * - Component 
    - Threads 
    - MIPS/Thread       
    - Memory (KB)           
    - Ports
  * - uIP server
    - 1
    - 50
    - 30
    - None

Configuration/Control
+++++++++++++++++++++

In addition to the other components 
there are application dependant threads that control
other I/O. For general configuration and slow I/O a minimum of
1 thread (50 MIPS) should be reserved.


