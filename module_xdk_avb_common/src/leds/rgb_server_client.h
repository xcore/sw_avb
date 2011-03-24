void RGB_UpdatePatterns(chanend c_rgb, unsigned char red, unsigned char blue, unsigned char green);

void RGB_EnableSpin_Red(chanend c_rgb);
void RGB_EnableSpin_Blue(chanend c_rgb);
void RGB_EnableSpin_Green(chanend c_rgb);
void RGB_DisableSpin_Red(chanend c_rgb);
void RGB_DisableSpin_Green(chanend c_rgb);
void RGB_DisableSpin_Blue(chanend c_rgb);

unsigned char RGB_GetPattern_Blue(chanend c_rgb);
unsigned char RGB_GetPattern_Green(chanend c_rgb);
unsigned char RGB_GetPattern_Red(chanend c_rgb);

void RGB_Kill(chanend c_rgb);
