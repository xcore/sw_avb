.. _sec_resource:

Resource Usage
--------------

Available Chip Resources
++++++++++++++++++++++++

Each XMOS device has a set of resources detailed in the following
table. The resources are split amongst different tiles on the device
which may affect how resources can be used:

.. list-table::
 :header-rows: 1
 :widths: 22 7 7 7 10

 * - Device
   - Logical Cores
   - MIPS/Core
   - Memory (KB)
   - Ports
 * - XS1-L16A-128-QF124-C10
   - 16
   - 1000
   - 128
   - | 32 x 1bit
     | 12 x 4bit
     | 7 x 8bit
     | 3 x 16bit
 * - XS1-L12A-128-QF124-C10
   - 12
   - 1000
   - 128
   - | 32 x 1bit
     | 12 x 4bit
     | 7 x 8bit
     | 3 x 16bit
 * - XS1-L10A-128-QF124-C10
   - 10
   - 1000
   - 128
   - | 32 x 1bit
     | 12 x 4bit
     | 7 x 8bit
     | 3 x 16bit

.. note::
 
   Note that some ports overlap on the device so, for example,
   using a 16 bit port may make some 1 bit ports unavailable. See
   the device datasheets for details.

The following sections detail the resource required for each
component. Please note that the memory requirements for code size
should be taken as a rough guide since exact memory usage depends
on the integration of components (which components are on which
tile etc.) in the final build of the application.

Ethernet Component
++++++++++++++++++

Each endpoint requires an Ethernet MAC layer.

.. list-table::
  :header-rows: 1

  * - Component 
    - Logical Cores 
    - MIPS/Core       
    - Memory (KB)           
    - Ports
  * - Ethernet 
    - 5
    - 50 
    - 15 code, 1.5 per buffer
    - 6 x 1bit, 2 x 4bit

PTP Component
+++++++++++++

Every AVB endpoint must include a PTP component.

.. list-table::
  :header-rows: 1

  * - Component 
    - Logical Cores 
    - MIPS/Core       
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
    - Logical Cores 
    - MIPS/Core       
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
    - Logical Cores 
    - MIPS/Core       
    - Memory (KB)           
    - Ports
  * - PLL driver
    - 0 - 1
    - 50
    - 0.5
    - 1 x 1bit + ports to configure PLL

.. note::
 
   PTP, Media Clock Server and PLL driver components may be combined into a single logical core running at 100 MIPS if
   the number of channels is constrained.


Audio Component(s)
++++++++++++++++++

Each endpoint may have several listener and talker components. Each
listener/talker component is capable of handling four IEEE P1722
streams and up to 12 channels of audio.

.. list-table::
  :header-rows: 1
  :widths: 14 8 12 12 10

  * - Component 
    - Logical Cores 
    - MIPS/Core       
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

.. note::
 
   The Talker and Listener components may be combined into a single logical core running at 100 MIPS if
   the number of streams is 1 and the number of channels is <= 4 per stream.

The amount of resource required for audio processing depends on the
interface and the number of audio channels required. The overheads
for the interface are:


.. list-table::
  :header-rows: 1
  :widths: 11 8 10 11 20

  * - Component 
    - Logical Cores 
    - MIPS/Core       
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
handle per logical core:

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
can be made *e.g.* you could use 2 logical cores to handle 16 channels 
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

Configuration/Control
+++++++++++++++++++++

In addition to the other components 
there are application dependant tasks that control
other I/O. For general configuration and slow I/O a minimum of
1 logical core (50 MIPS) should be reserved.


