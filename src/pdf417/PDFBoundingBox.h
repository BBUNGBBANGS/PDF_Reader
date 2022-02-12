#ifndef _PDFBOUNDINGBOX_H
#define _PDFBOUNDINGBOX_H

#include "ResultPoint.h"

typedef struct
{
	int _imgWidth;
	int _imgHeight;
	ResultPoint_t _topLeft;
	ResultPoint_t _bottomLeft;
	ResultPoint_t _topRight;
	ResultPoint_t _bottomRight;
	int _minX;
	int _maxX;
	int _minY;
	int _maxY;
}BoundingBox_val_t;

typedef struct 
{
	uint8_t m_hasValue;
	BoundingBox_val_t m_value;
}BoundingBox_t;

extern uint8_t Create(int imgWidth, int imgHeight, const ResultPoint_t topLeft, const ResultPoint_t bottomLeft, const ResultPoint_t topRight, const ResultPoint_t bottomRight, BoundingBox_t * result);
extern uint8_t AddMissingRows(const BoundingBox_t * box, int missingStartRows, int missingEndRows, uint8_t isLeft, BoundingBox_t * result);
extern ResultPoint_t topLeft(BoundingBox_t * box);
extern ResultPoint_t topRight(BoundingBox_t * box);
extern ResultPoint_t bottomLeft(BoundingBox_t * box);
extern ResultPoint_t bottomRight(BoundingBox_t * box);
extern void copy(BoundingBox_t * box,BoundingBox_t * result);

#endif