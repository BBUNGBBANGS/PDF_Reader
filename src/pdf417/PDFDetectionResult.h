#ifndef _PDFDETECTIONRESULT_H
#define _PDFDETECTIONRESULT_H

#include "PDFBarcodeMetadata.h"
#include "PDFBoundingBox.h"
#include "PDFDetectionResultColumn.h"

typedef struct 
{
	BarcodeMetadata_t _barcodeMetadata;
	DetectionResultColumn_t * _detectionResultColumns;
	BoundingBox_t * _boundingBox;
}DetectionResult_t;

#endif
