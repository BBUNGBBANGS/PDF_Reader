#ifndef _PDFDETECTIONRESULT_H
#define _PDFDETECTIONRESULT_H

#include "PDFBarcodeMetadata.h"
#include "PDFBoundingBox.h"
#include "PDFDetectionResultColumn.h"

typedef struct 
{
	BarcodeMetadata_t _barcodeMetadata;
	DetectionResultColumn_t _detectionResultColumns[100];
	BoundingBox_t _boundingBox;
}DetectionResult_t;
extern void Detection_Result_init(const BarcodeMetadata_t * barcodeMetadata, const BoundingBox_t * boundingBox, DetectionResult_t * result);
extern const DetectionResultColumn_t allColumns(DetectionResult_t * detectionResult,int row,int size);
#endif
