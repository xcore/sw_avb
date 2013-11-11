#include <xs1.h>
#include <platform.h>
#include "reboot.h"

/* Reboots XMOS device by writing to the PLL config register */
void device_reboot(void)
{
    unsigned int pllVal;
    unsigned int localTileId = get_local_tile_id();
    unsigned int tileId;
    unsigned int tileArrayLength;

    asm volatile ("ldc %0, tile.globound":"=r"(tileArrayLength));

    /* Reset all remote tiles */
    for(int i = 0; i < tileArrayLength; i++)
    {
        /* Cannot cast tileref to unsigned */
        tileId = get_tile_id(tile[i]);

        /* Do not reboot local tile yet */
        if (localTileId != tileId)
        {
            read_sswitch_reg(tileId, 6, pllVal);
            write_sswitch_reg_no_ack(tileId, 6, pllVal);
        }
    }

    /* Finally reboot this tile */
    read_sswitch_reg(localTileId, 6, pllVal);
    write_sswitch_reg_no_ack(localTileId, 6, pllVal);

}
