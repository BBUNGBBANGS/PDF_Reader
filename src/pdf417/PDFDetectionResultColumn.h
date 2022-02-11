#ifndef _PDFDETECTIONRESULTCOLUMN_H
#define _PDFDETECTIONRESULTCOLUMN_H

#include "PDFBoundingBox.h"
#include "PDFCodeword.h"

typedef enum  
{
	None,
	Left,
	Right,
}RowIndicator_t;

typedef struct 
{
	BoundingBox_t _boundingBox;
	Codeword_t _codewords[500];
	RowIndicator_t _rowIndicator;
}DetectionResultColumn_t;

extern void DetectionResultColumn(const BoundingBox_t * boundingBox, RowIndicator_t rowIndicator);

#endif
