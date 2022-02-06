#ifndef _BITMATRIX_H
#define _BITMATRIX_H

#include <stdint.h>

typedef struct
{
	int _width;
	int _height;
	int _rowSize;
	uint8_t * bits;
	// There is nothing wrong to support this but disable to make it explicit since we may copy something very big here.
	// Use copy() below.
}BitMatrix_t;

extern void getPatternRow(BitMatrix_t * m, int r, uint16_t * p_row);
#endif