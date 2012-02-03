Hardware development platforms
------------------------------

For initial development of AVB applications the following XMOS
development platforms are recommended for any low channel count
application (using the XS1-L2 device):

  * `XK-AVB-LC-SYS AVB Audio Endpoint <http://www.xmos.com/products/reference-designs/avbl2>`_

In addition the following kit can be used for development:

  * `XDK XS1-G Development Kit <http://www.xmos.com/products/development-kits/xs1-g-development-kit>`_

To develop with this kit is is recommended to also use the following add
on board:

  * `XAI Multichannel Audio Interface <http://www.xmos.com/products/development-kits/xai-multichannel-audio-interface>`_

Finally, the following board can also be used:

  * `XC-2 Ethernet Kit <http://www.xmos.com/products/development-kits/xc-2-ethernet-kit>`_

This board has no audio hardware built in; a CODEC and
frequency generator must be added to the board.

It is recommended to have at least two boards for developing streaming
audio applications.

For developing an application specific board for AVB please
refer to the hardware guides for the above boards which contain example
schematics, BOMs, design guidelines etc.

It is also recommended that an AVB compatible network switch be obtained and used while developing the system. While
the XMOS AVB solution can use a non-AVB switch, some of the features will need to be disabled.
