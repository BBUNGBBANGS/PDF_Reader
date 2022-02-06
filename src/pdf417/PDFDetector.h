#ifndef _PDFDETECTOR_H
#define _PDFDETECTOR_H

#include "DecodeStatus.h"
#include "GenericLuminanceSource.h"
#include "BitMatrix.h"
#include "ResultPoint.h"

typedef struct
{
	BitMatrix_t * bits;
	ResultPoint_t * points;
}Detector_Result_t;

extern DecodeStatus_t Detect(GenericLuminanceSource_t * image, unsigned char multiple, Detector_Result_t * result);
#endif
