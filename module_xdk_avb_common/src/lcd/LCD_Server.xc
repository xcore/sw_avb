/**
 * @ModuleName Generic LCD Server Component.
 * @Author Ross Owen
 * @Date 15/05/2008
 * @Version 1.0
 * @Description: Generic LCD Server Component.
 *
 *
 **/

#include <platform.h>
#include <xs1.h>
#include "LCD_Comp_Def.h"
#include "LCD_Defines_Internal.h"
//#include "SRAM_ClientComp.h"
//#include "xlog_client.h"
#include "char_buffer.h"
#include "LcdDriver.h"
#include <platform.h>

//#include "LCD_FrameIndex.h"
//#include "LCD_ServerActive.h"
//include "LCD_CTCheck.h"

#define CHAR_FIFO_INDEX 0
#define BLENDED_FIFO_INDEX 1
#define BLENDED_FIFO_SIZE 1024

#define BLENDED_DATA_MOVE_SIZE (BLENDED_FIFO_SIZE >> 3)

unsigned fgColour = LCD_WHITE;         // Default text colour

void lcdphysicalkilled()
{
  while(1);
}


unsigned colours[] =  {LCD_RED, LCD_GREEN, LCD_BLUE};

void lcd_shutdown(chanend lcd, out port dclk, clock clk)
{
  outct(lcd, XS1_CT_END);
  inct(lcd);
  set_port_use_off(dclk);
  set_clock_off(clk);
}

#pragma unsafe arrays
static void LCD_Physical(timer t, chanend c[], chanend lcd_switch_ctl, out port p_lcd_hsy, buffered out port:32 p_lcd_rgb )
{
  unsigned tmp=0;
  unsigned data = LCD_BLACK;
  unsigned VCount, HCount;
  const unsigned tVBP = T_VBP;
#ifdef LCD_START_WITH_XLOG
  int i=1;
#else
  int i=0;
#endif
  
  //for (int i=0;i<2;i++)
    //start_streaming_slave(c[i]);

  while(1)
  {
    t :> tmp;
    select {
      case lcd_switch_ctl :> i:
        break;
      default:
        break;
    }    
    t when timerafter(tmp+tVBP) :> tmp;

    
    
 //   outuint(c[i], 1); 			// Send Frame start to blender...

    //    start_streaming_master(c[i]);
    outct(c[i], XS1_CT_END);
    chkct(c[i], XS1_CT_ACK);
   
    for(VCount = 0; VCount < LCD_HEIGHT_PX; VCount+=1)
    { t:> tmp;
      t when timerafter(tmp + T_WH) :> tmp;

      p_lcd_hsy <: 1;
      p_lcd_hsy <: 1;
      t :> tmp;
      t when timerafter(tmp+T_HBP) :> tmp;
    
      //DTMG_port <: 1; 				// Not required, strobed out port used
 
      for(HCount = 0; HCount < LCD_WIDTH_PX; HCount++)
      {
        // Channel input from blender thread (2 x RGB565 pixels)
        data = inuint(c[i]);
        
        // Output blended data to RGB port
        // LCD is RGB666 all our data is RGB565, do RGB565 -> RGB666 conversion...
        tmp =  ((data&0xf800)<<2)| ((data&0x7e0)<<1) | ((data&0x1f)<<1);
    
        p_lcd_rgb <: tmp;

        //data = data >> 16;

        //tmp =  ((data&0xf800)<<2)| ((data&0x7e0)<<1) | ((data&0x1f)<<1);
          
        //p_lcd_rgb <: tmp;
      }
      //DTMG_port <: 0; 				// Not required, strobed out port used
      //DTMG_port <: 0;

      
      t :> tmp;    
      t when timerafter( tmp + T_HFP) :> tmp;

      p_lcd_hsy <: 0;
      p_lcd_hsy <: 0;
    }
    
    //stop_streaming_master(c[i]);
    chkct(c[i], XS1_CT_END);
    outct(c[i], XS1_CT_END);
  }
}

/** LCD_SRAM_Server
  * @param c_lcd channel from app to this lcd server
  * @param c_sram channel to sram server
  * @brief main lcd server thread when using sram
  * @return void
  */
void lcd_driver(struct lcd_resources &r_lcd,
                chanend c_lcd_con[],
                chanend lcd_switch_ctl)
{
  timer t;

  set_thread_fast_mode_on();

  
  // Port init for physical interface
  // TODO: LCD PORT INIT FUNCTION
  set_clock_div(r_lcd.clk_lcd, LCD_CLKDIV);	// Set clock divide appropriately (off ref clock)
  set_port_inv(r_lcd.p_lcd_clk);		         // Invert clk (LCD latches on neg edge)

  set_port_clock(r_lcd.p_lcd_hsy, r_lcd.clk_lcd);	// Clock ports from clock
  set_port_clock(r_lcd.p_lcd_clk, r_lcd.clk_lcd);
  set_port_clock(r_lcd.p_lcd_rgb, r_lcd.clk_lcd);

  configure_out_port_strobed_master(r_lcd.p_lcd_rgb, r_lcd.p_lcd_dtm, r_lcd.clk_lcd, 0);

  set_port_mode_clock(r_lcd.p_lcd_clk);	   // Set clock port to outclock mode

  start_clock(r_lcd.clk_lcd);			         // Start clockblock
  
  LCD_Physical(t, c_lcd_con, lcd_switch_ctl, r_lcd.p_lcd_hsy, r_lcd.p_lcd_rgb);

  set_port_mode_data(r_lcd.p_lcd_clk);	// Set clock port to outclock mode

}




