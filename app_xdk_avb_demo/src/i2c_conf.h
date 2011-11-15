
// The XDK board has pull down resistors on the external IO ports, so we need
// to enable single master mode in the I2C to make sure we drive 1 as well as 0
// out of the ports.  Failure to do this will make the lines low (0) when we are
// high impedance (supposed to be 1)

#define I2C_SINGLE_MASTER

