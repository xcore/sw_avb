
#include "rgb_server.h"
#include "rgb_server_client.h"

void RGB_UpdatePatterns(chanend c_rgb, unsigned char red, unsigned char blue, unsigned char green)
{
  c_rgb <: RGB_CMD_UPDATE_PATTERNS;
  c_rgb <: (unsigned) red; 	// RED
  c_rgb <: (unsigned) blue;  	// BLUE
  c_rgb <: (unsigned) green; 	// GREEN
}

void RGB_EnableSpin_Blue(chanend c_rgb)
{
  c_rgb <: RGB_CMD_ENABLE_BLUE_SPIN;
}

void RGB_EnableSpin_Green(chanend c_rgb)
{
  c_rgb <: RGB_CMD_ENABLE_GREEN_SPIN;
}

void RGB_EnableSpin_Red(chanend c_rgb)
{
  c_rgb <: RGB_CMD_ENABLE_RED_SPIN;
}

void RGB_DisableSpin_Blue(chanend c_rgb)
{
  c_rgb <: RGB_CMD_DISABLE_BLUE_SPIN;
}

void RGB_DisableSpin_Green(chanend c_rgb)
{
  c_rgb <: RGB_CMD_DISABLE_GREEN_SPIN;
}

void RGB_DisableSpin_Red(chanend c_rgb)
{
  c_rgb <: RGB_CMD_DISABLE_RED_SPIN;
}

unsigned char RGB_GetPattern_Blue(chanend c_rgb)
{
  unsigned tmp;
  c_rgb <: RGB_CMD_GET_BLUE_PATTERN;
  c_rgb :> tmp;
 
  return (unsigned char) tmp;
}

unsigned char RGB_GetPattern_Green(chanend c_rgb)
{
  unsigned tmp;
  c_rgb <: RGB_CMD_GET_GREEN_PATTERN;
  c_rgb :> tmp;
 
  return (unsigned char) tmp;

}

unsigned char RGB_GetPattern_Red(chanend c_rgb)
{
  unsigned tmp;
  c_rgb <: RGB_CMD_GET_RED_PATTERN;
  c_rgb :> tmp;
 
  return (unsigned char) tmp;
}

void RGB_Kill(chanend c_rgb)
{
  c_rgb <: RGB_CMD_KILL;
}






