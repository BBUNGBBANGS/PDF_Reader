#ifndef _PDFDECODEDBITSTREAMPARSER_H
#define _PDFDECODEDBITSTREAMPARSER_H

#include "DecoderResult.h"

extern void BitStreamDecode(DecoderResult_t * DecoderResult, const int * codewords, int ecLevel, const unsigned char * characterSet);

#endif