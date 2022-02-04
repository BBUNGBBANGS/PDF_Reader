#ifndef _PDFDETECTOR_H
#define _PDFDETECTOR_H

#include "DecodeStatus.h"
#include "GenericLuminanceSource.h"

extern DecodeStatus_t Detect(GenericLuminanceSource_t * image, unsigned char multiple, unsigned char * result);
#endif
