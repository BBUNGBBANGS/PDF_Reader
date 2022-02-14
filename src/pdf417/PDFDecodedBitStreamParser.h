#ifndef _PDFDECODEDBITSTREAMPARSER_H
#define _PDFDECODEDBITSTREAMPARSER_H

#include "DecoderResult.h"

extern DecoderResult_t BitStreamDecode(const int * codewords, int ecLevel, const unsigned char * characterSet);

#endif