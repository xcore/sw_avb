.. _sec_app_tutorial:

An XMOS AVB application (tutorial)
----------------------------------

This tutorial walks through the application code for the XR-AVB-LC-BRD
demonstration application. This application provides both a talker
transmitting a single eight channel stream and a listener that can
receive an eight channel stream. 

The code for this demo is found in the ``app_xr_avb_lc_demo/src`` directory.

avb_conf.h
~~~~~~~~~~

The ``avb_conf.h`` file sets the required defines for configuring the
AVB system. Every application must include this file and the required
and optional that can be set in this file are described in Section :ref:`sec_defines_api`.

First, the file sets up the ethernet buffering. This is a balance
between the required memory for the rest of the application and the
amount of buffering needed for the audio. 

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: ethernet 
   :end-before: AVB configuration
   :strip-leading-whitespace:

Some general settings are needed for memory allocation across the
whole AVB code base. Here the maximum name length (use for
remote control identification) and the maximum channels per stream are set:

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: AVB configuration
   :end-before: Listener configuration
   :strip-leading-whitespace:

The application can listen to a single eight channel stream. This requires a single sink that can be handled by a single listener unit:

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: Listener configuration 
   :end-before: Talker configuration
   :strip-leading-whitespace:

The application will produce a single eight channel stream. This requires a single source that can be handled by a single talker unit:

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: Talker configuration
   :end-before: Media configuration
   :strip-leading-whitespace:

The audio I/O side of the application must be configured. The board has eight digital I/Os in and out so this
determines the number of input/output FIFOs. In addition the
maximum sample rate of these inputs is set. A media unit is one that controls
media FIFOs. Due to the way the I2S component
works, two media units are required (one for input and one for output). 

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: Media configuration
   :end-before: Media clock configuration
   :strip-leading-whitespace:

The demo is synchronous in that it has one clock for both the input
and output (this will end up being a clock divided down from the PTP
clock). 

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: Media clock configuration
   :end-before: Add synths
   :strip-leading-whitespace:

Finally, the XR-AVB-LC-BRD has eight digital inputs but only two of
them are connected to an ADC on board. For this demo another setting is added, which causes the I2S component to ignore the input on
every stereo pair but the first, and instead adds synthesized sine
waves to these inputs:

.. literalinclude:: app_xr_avb_lc_demo/src/avb_conf.h
   :start-after: Add synths
   :end-before: endif
   :strip-leading-whitespace:


The toplevel main
~~~~~~~~~~~~~~~~~

The main file for the demo is ``xr_avb_demo.xc``. This file contains
three main parts:

  * First, the file declares the ports needed by the application.
  * Second, the top-level ``main()`` function runs the main
    components that make up the application.
  * Finally, the ``demo()`` function implements the main control
    thread that implements the demo.

The demo runs at a particular sample rate. This affects the AVB
control thread and also the thread that drives the external PLL. To
co-ordinate this, the program sets some #defines relating to the sample
rate: 

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // this is the sample rate
   :end-before: // Set the period inbetween
   :strip-leading-whitespace:

Here, ``MASTER_TO_WORDCLOCK_RATIO`` controls the ratio between the master
clock and the wordclock, which must match the setting in the clock generation
PLL.

The next part of the file (not shown) declares the ports for ethernet, audio
CODECs and the PLL.

The next part of the file contains the top-level main that runs the
components of the application. It is of the form::

  int main(void) {
    channel declarations
    ...
    par {
      ...
      component functions
      ...
    }
  }


The first component functions are the ethernet components: the
ethernet MAC and the TCP/IP stack server. The ethernet component
section reads the MAC address out of OTP and then runs the ethernet
server based on this. The channel arrays ``rx_link``, ``tx_link`` and
``xtcp`` are connected to other parts of the system that use the
ethernet and TCP/IP stack:

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // AVB - Ethernet
   :end-before: // AVB - PTP
   :strip-leading-whitespace:

The next components that are run are also core components of an AVB
application, namely the PTP server and the media clock server. Note
that the initialization of the PLL is run before the PTP server simply
because it must be initialized on core 1. The actual code that
drives the PLL is on core 0. In order to save threads, the GPIO and the
PTP servers are combined into a single thread.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // AVB - PTP
   :end-before: // AVB - Audio
   :strip-leading-whitespace:

As mentioned previously, the application has one outgoing stream which
is handled by a single talker unit. The talker unit is instantiated
next with a call to :c:func:`avb_1722_talker`:

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // AVB Talker
   :end-before: // AVB Listener
   :strip-leading-whitespace:

There is also a single listener unit, which takes incoming packets
and splits them into media FIFOs. However, the I2S component takes
samples to play over an XC channel. So :c:func:`media_output_to_xc_channel_split_lr` is called to take samples from the shared memory media FIFOs and output them over an XC channel.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // AVB Listener
   :end-before: // Xlog
   :strip-leading-whitespace:

The above components comprise the core of the AVB system. In addition
there is a debugging thread and a couple of control threads. The
debugging thread calls the XLog server which redirects print
statements across the system to pass over the UART port to the XTAG2. 
The resulting print statements can be viewed using ``xrun`` with the
``--uart`` option.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Xlog
   :end-before: // Application
   :strip-leading-whitespace:

Finally, the application has an application specific control
thread. 

First the AVB system must be initialized with a call to
:c:func:`avb_init`.  This call is needed before any other AVB API calls.

This function takes channels connected to all the different components
of the system to be able to control them. 

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Application threads
   :end-before: }
   :strip-leading-whitespace:


The main control thread
~~~~~~~~~~~~~~~~~~~~~~~

The main control thread is implemented in the function ``demo``:

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: /** The main application
   :end-before: timer
   :strip-leading-whitespace:

This demo uses Zeroconf to advertise a configuration
protocol. The name is registered as ``xmos_attero_endpoint``, so on the local
network it will have a DNS name of  ``xmos_attero_endpoint.local``. The server ``attero-cfg`` which is a configuration service
over UDP on port ``ATTERO_CFG_PORT`` (with the value 40404) is also advertised. This service can be discovered by the AtteroTech host configuration utility
(see [AttQS10]_). The control API server itself must be initialized so that it can handle requests to this port.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Initialize Zeroconf
   :end-before: // Initialize the media clock
   :strip-leading-whitespace:

The next section of code configures the clocking and the source
streams the demo will transmit. Firstly, as mentioned earlier in the
walkthrough, this demo has one media clock which is derived from the
global PTP clock. This allows all endpoints to run a talker and
listener on the same clock with a minimum of configuration. Another 
possible scheme would be for every endpoint to have a media clock that
is derived from the same AVB stream.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Initialize the media clock
   :end-before: // Main loop
   :strip-leading-whitespace:

After the initial configuration the application enters its main
control loop. This is in the standard XC form of a "while (1), select"
loop. The loop iterates and at each point selects one of the case
statements to handle. The cases may be activated by an incoming packet, a
timer event for periodic processing or some communication from the
``gpio`` thread.

::
   
   while (1) {
      ...      
      select {
         case ...
            break;
         case ...
            break;
         ...
      }
   }


The first case handled is an incoming AVB control packet from the
MAC. The :c:func:`avb_get_control_packet` function fires to this case
and places the packet in the array ``buf``.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Receive any incoming AVB packets
   :end-before: // Process AVB
   :strip-leading-whitespace:


This packet may be an 802.1Qat packet or a 1722 MAAP packet. First we
pass it to the AVB packet handler (it will ignore the packet
if it is not a relevant protocol).

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Process AVB
   :end-before: }
   :strip-leading-whitespace:


This result of this processing may be a report of a failed route which
in this application is just ignored. Alternatively, the result may be
that we have lost our reserved
addresses (since some other node on the network has a claim to
them). In this case we request new address for our streams.

The next event the main loop responds to is an incoming TCP/IP packet.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Process TCP/IP
   :end-before: // Receive any events
   :strip-leading-whitespace:


Here, two handlers are called. One to handle any Zeroconf packets and
one to handle any control protocol packets. The control protocol
packets are used to communicate with the Atterotech PC control
application. The code used to do this within the firmware is in the
module ``module_avb_attero_cfg``.


The ``gpio`` thread controls the buttons on the device. It can signal
the main control thread of certain events. The next case handles this:

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Receive any events from user button
   :end-before: // Periodic
   :strip-leading-whitespace:

One possibility is that the ``STREAM_SEL`` button is pressed to
change the stream being listened to. This just sets the
``change_stream`` variable to be passed to the stream manager of the
application later.

The other possibility is that the ``CHAN_SEL`` button is pressed which
changes the channels being listened to within the stream. This
disables the listener sink, reconfigures the mapping between the
incoming stream and the output FIFOs and then re-enables the stream.

The final event that can be responded to is a periodic event that
occurs via an XCore timer. This event happens once every 50us.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // Periodic processing
   :end-before: // Call the stream manager
   :strip-leading-whitespace:

The :c:func:`avb_periodic` function performs general AVB periodic
processing and may return a report that the MAAP addresses that were
requested have been allocated. At this point we can set the
destination of the talker stream and enable it.

The other piece of periodic processing is to call the demo's stream
manager. This is a function contained in ``demo_stream_manager.xc``,
which manages the streams that have been seen and the stream that is being
listened to.

.. literalinclude:: app_xr_avb_lc_demo/src/xr_avb_demo.xc
   :start-after: // what is being
   :end-before: break;
   :strip-leading-whitespace:

The demo stream manager
~~~~~~~~~~~~~~~~~~~~~~~

The demo stream manager is contained in the file
``demo_stream_manager.xc``. It maintains a table of streams that have
been seen in the array ``stream_table``. A stream is first seen 
via the function :c:func:`avb_check_for_new_stream`.

.. literalinclude:: app_xr_avb_lc_demo/src/demo_stream_manager.xc
   :start-after: // check if there is a new 
   :end-before: // if so
   :strip-leading-whitespace:

If there is no current stream and a new stream is seen, or if the
``change_stream`` variable is set (due to a button press, see the
preceding Section), then the current listened to stream is updated.
This is done by reconfiguring the sink in this section of code:

.. literalinclude:: app_xr_avb_lc_demo/src/demo_stream_manager.xc
   :start-after: map the new stream
   :end-before: }
   :strip-leading-whitespace:





