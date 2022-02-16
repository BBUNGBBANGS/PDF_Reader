#ifndef _READBARCODE_H
#define _READBARCODE_H

#include "Result.h"
#include <stdint.h>
#include <stdio.h>
#include "GenericLuminanceSource.h"
typedef enum 
{
	None = 0,
	Lum  = 0x01000000,
	RGB  = 0x03000102,
	BGR  = 0x03020100,
	RGBX = 0x04000102,
	XRGB = 0x04010203,
	BGRX = 0x04020100,
	XBGR = 0x04030201,
}ImageFormat_t;

typedef struct 
{
	uint8_t* data; 
	int width; 
	int height;
	ImageFormat_t format; 
	int rowStride; 
	int pixStride;
}ImageView_t;

extern result_t ReadBarcode(unsigned char *iv, int width,int height, unsigned int ImageFormat);
#endif




