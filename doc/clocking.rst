Media Clocks
------------

A media clock controls the rate at which information is passed to an
external media playing device. For example, an audio word clock that
governs the rate at which samples should be passed to an audio CODEC.
An XMOS AVB endpoint can keep track of several media clocks. 

A media clock can be synchronized to one of two sources:

 * An incoming clock signal on a port.
 * The word clock of a remote endpoint, derived from an incoming *IEEE P1722* audio stream.

A hardware interface can be tied to a particular media
clock, allowing the media output from the XMOS device to be
synchronized with other devices on the network.

All media clocks are maintained by the media clock server
component. This component maintains
the current state of all the media clocks in the system. It then
periodically updates other components with clock change information to
keep the system synchronized. The set of media clocks is determined by
an array passed to the server at startup.

The media clock server component also receives information from the
audio listener component to track timing information of incoming
*IEEE P1722* streams. It then sends control information back to
ensure the listening component honors the presentation time of the
incoming stream.

Driving an external clock generator
+++++++++++++++++++++++++++++++++++

A high quality, low jitter master clock is often required to drive an audio CODEC and must be synchronized with an AVB media clock.
The XS1 chip cannot provide this clock directly but can provide a
lower frequency source for a frequency synthesizer chip or external
PLL chip. 
The frequency synthesizer chip must be able to generate a high
frequency clock based on a lower frequency signal, such as the Cirrus Logic CS2100-CP. The
recommended configuration is as in the block diagram below:

.. only:: latex

 .. image:: images/ratectl.pdf
   :width: 70%
   :align: center

.. only:: html

 .. image:: images/ratectl.png
   :align: center

The XS1 device provides control to the frequency synthesizer and the
frequency synthesizer provides the audio master clock to the CODEC and XS1 device. The
sample bit and word clocks are then provided to the CODEC by
the XS1 device.