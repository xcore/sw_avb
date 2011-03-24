/*
 * @RGB LED Driver
 * @Description RGB Led driver
 *
 *
 */

#include "rgb_driver.h"

void DriveLeds(out port blank, out port csel, out port sclk, out port sin, out port latch, unsigned holdLength, unsigned  patterns[3])
{
  unsigned currentColour;

  latch <: 1;

  for (currentColour = 0; currentColour < 3; currentColour += 1)
  {
    unsigned i;
    unsigned currentPattern;

    csel <: currentColour;

    currentPattern = patterns[currentColour];

    for (i = 0; i < 8; i += 1)
    {
      sclk <: 0;
      sync(sclk);
      sin <: >> currentPattern;
      sync(sin);
      sclk <: 1;
      sync(sclk);
    }

    latch <: 0;
    sclk <: 1;
    latch <: 1;

    for (i = 0; i < holdLength; i += 1)
    {
      sclk <: 0;
    }
  }
}
    


