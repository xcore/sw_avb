#ifndef _LCDDRIVER_H__
#define _LCDDRIVER_H_

struct lcd_resources {
  out port p_lcd_hsy;
  out port p_lcd_dtm;
  out port p_lcd_clk;
  buffered out port:32 p_lcd_rgb;
  clock clk_lcd;
};

void lcd_driver(struct lcd_resources &r_lcd, chanend lcd_data[], chanend lcd_ctl);

void lcd_text_display(chanend text_data, chanend lcd_data);

#endif  /* _LCDDRIVER_H_ */

