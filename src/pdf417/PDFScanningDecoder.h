#ifndef _PDFSCANNINGDECODER_H
#define _PDFSCANNINGDECODER_H

#include "DecoderResult.h"
#include "BitMatrix.h"
#include "ResultPoint.h"

extern DecoderResult_t * Decode(const BitMatrix_t * image, ResultPoint_t imageTopLeft, ResultPoint_t imageBottomLeft,
 								ResultPoint_t imageTopRight, ResultPoint_t imageBottomRight,
								int minCodewordWidth, int maxCodewordWidth, const unsigned char * characterSet);
#endif
