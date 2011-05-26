#include <string.h>
#include "avb_device_defines.h"

int get_device_system(char a0[]) 
{
  strcpy(a0, AVB_DEVICE_SYSTEM);
  return 1;
}

int get_device_identity_serial(char a0[])
{
  strcpy(a0, AVB_DEVICE_SERIAL);
  return 1;
}

int get_device_identity_version(char a0[])
{
  strcpy(a0, AVB_DEVICE_VERSION);
  return 1;
}

int get_device_identity_vendor_id(char a0[])
{
  strcpy(a0, AVB_DEVICE_VENDOR_ID);
  return 1;
}

int get_device_identity_product(char a0[])
{
  strcpy(a0, AVB_DEVICE_PRODUCT);
  return 1;
}

int get_device_identity_vendor(char a0[])
{ 
  strcpy(a0, AVB_DEVICE_VENDOR);
  return 1;
}

int get_device_name(char a0[])
{
  strcpy(a0, AVB_DEVICE_NAME);
  return 1;
}


