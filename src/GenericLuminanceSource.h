#ifndef _GENERICLUMINANCESOURCE_H
#define _GENERICLUMINANCESOURCE_H

#include <stdint.h>
#include <stdio.h>
typedef struct 
{
    unsigned char * pixels;
    int left;
    int top;
    int width;
    int height;
    int rowBytes;
}GenericLuminanceSource_t;

extern unsigned char * GenericLuminanceSource(int left, int top, int width, int height, unsigned char * bytes, int rowBytes, int pixelBytes, int redIndex, int greenIndex, int blueIndex);
extern const uint8_t * getMatrix(GenericLuminanceSource_t * source, uint8_t * buffer, int * outRowBytes, uint8_t forceCopy);
#endif
