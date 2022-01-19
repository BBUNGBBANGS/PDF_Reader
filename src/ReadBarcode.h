
enum ImageFormat
{
	None = 0,
	Lum  = 0x01000000,
	RGB  = 0x03000102,
	BGR  = 0x03020100,
	RGBX = 0x04000102,
	XRGB = 0x04010203,
	BGRX = 0x04020100,
	XBGR = 0x04030201,
};

extern unsigned int * ReadBarcode(unsigned char *iv, int width,int height, unsigned int ImageFormat);




