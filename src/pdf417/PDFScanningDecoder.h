#ifndef _PDFSCANNINGDECODER_H
#define _PDFSCANNINGDECODER_H

#include "DecoderResult.h"
#include "BitMatrix.h"
#include "ResultPoint.h"
#include "PDFDetectionResultColumn.h"

extern DecoderResult_t * Decode(const BitMatrix_t * image, ResultPoint_t imageTopLeft, ResultPoint_t imageBottomLeft,
 								ResultPoint_t imageTopRight, ResultPoint_t imageBottomRight,
								int minCodewordWidth, int maxCodewordWidth, const unsigned char * characterSet);

extern DetectionResultColumn_t rowIndicatorColumn;
extern uint8_t Bounding_Merge(const BoundingBox_t * leftBox, const BoundingBox_t * rightBox, BoundingBox_t * result);
extern int columnCount(BarcodeMetadata_t * Metadata);
extern int rowCount(BarcodeMetadata_t * Metadata);
#endif
