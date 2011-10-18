
#include "xs1_kernel.h"
#include "xs1_user.h"



.global write_sswitch_reg_blind, "f{si}(ui,ui,ui)"
.global write_sswitch_reg_blind.nstackwords
.linkset write_sswitch_reg_blind.nstackwords, 0
//.type read_sswitch_reg, @function
// r0 - coreid
// r1 - reg
// r2 - data
.cc_top write_sswitch_reg_blind.function, write_sswitch_reg_blind
.align 2
write_sswitch_reg_blind:
 // Check range of coreid
  shr r3, r0, XS1_CHAN_ID_PROCESSOR_SIZE + XS1_CHAN_ID_NODE_SIZE
   bt r3, write_switch_reg_fail

    // Check range of reg
     shr r3, r1, 16
      bt r3, write_switch_reg_fail

       // Allocate channel end
        getr r3, XS1_RES_TYPE_CHANEND

         // Set destination
          ldc r11, XS1_RES_TYPE_CONFIG | (XS1_CT_SSCTRL << XS1_CHAN_ID_CHANNUM_SHIFT)

          // r0 - l
          // r1 - reg
          // r2 - data
          // r3 - chanend
          // r11 - low half of dest
          write_switch_reg:
           shl r0, r0, XS1_CHAN_ID_PROCESSOR_SHIFT
            or r0, r0, r11
             setd res[r3], r0

              // Send packet
               ldc r11, XS1_CT_WRITEC // Too big for outct immediate
                outct res[r3], r11
                 mkmsk r0, 32
                  shl r0, r0, 8
                   shr r11, r1, 8
                    or r0, r0, r11
                     out res[r3], r0 // (0xffffff00) | (reg >> 8)
                      outt res[r3], r1 // reg & 0xff
                       out res[r3], r2
                        outct res[r3], XS1_CT_END

                         // Receive response
                          freer res[r3]
                           retsp 0
                           write_switch_reg_fail:
                            ldc r0, 0
                             retsp 0
                             .cc_bottom write_sswitch_reg_blind.function
