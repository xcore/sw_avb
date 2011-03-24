
// Internal defintions
// LCD Timing defines
#define T_HFP                 600 // 400
#define T_HBP                 900 // 600
#define T_WH                  300 // 150
#define T_VBP                 20000 //20000
#define LCD_CLKDIV            24		// TODO: get this down to 26

#define LCD_GFXCMD_BUF_AUTO 0	// Automatic buffering
#define LCD_GFXCMD_BUF_MAN  1	// Manual buffer control
#define LCD_GFXCMD_START    2   // Start the server

#define LCD_SRMCMD_WR	    0
#define LCD_SRMCMD_RD	    1

// TODO check these ok for apps
#define LCD_CTOK_ENDFRAME   0x20 // End frame in autobuff mode
#define LCD_CTOK_KILL	    0x21 // Kill server

//Manual buffering specific
#define LCD_CTOK_PROFRAME   0x22 // Set producer frame in manbuff mode
#define LCD_CTOK_CONFRAME   0x23 // Set consumer fram in manbuff mode
