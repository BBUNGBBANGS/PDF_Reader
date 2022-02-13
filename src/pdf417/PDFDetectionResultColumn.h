#ifndef _PDFDETECTIONRESULTCOLUMN_H
#define _PDFDETECTIONRESULTCOLUMN_H

#include "PDFBoundingBox.h"
#include "PDFCodeword.h"
#include "PDFBarcodeMetadata.h"

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
}DetectionResultColumn_val_t;
typedef struct 
{
	uint8_t m_hasValue;
	DetectionResultColumn_val_t m_value;
}DetectionResultColumn_t;
extern Codeword_t * allCodewords(DetectionResultColumn_t * RowIndicatorColumn);
extern void DetectionResultColumn(const BoundingBox_t * boundingBox, RowIndicator_t rowInd, DetectionResultColumn_t * RowIndicatorColumn);
extern void adjustCompleteIndicatorColumnRowNumbers(DetectionResultColumn_t * detectionResultColumn,const BarcodeMetadata_t * barcodeMetadata);
extern Codeword_t codewordNearby(DetectionResultColumn_t * RowIndicatorColumn,int imageRow,int size);
extern uint8_t getBarcodeMetadata(DetectionResultColumn_t * RowIndicatorColumn, BarcodeMetadata_t * result, int length);
extern uint8_t getRowHeights(DetectionResultColumn_t * rowIndicatorColumn,int * result,int length,int * size);
#endif
