#ifndef _DECODERESULT_H
#define _DECODERESULT_H
#include "decodestatus.h"
#include "StructuredAppend.h"
#include <stdint.h>
typedef struct 
{
	DecodeStatus_t status;
	uint8_t * _rawBytes;
	int _numBits;
	unsigned char * _text; //wstring
	unsigned char * _ecLevel; //wstring
	int _errorsCorrected;
	int _erasures;
	StructuredAppendInfo_t _structuredAppend;
	uint8_t _readerInit; // bool
	uint8_t * _extra; //custom data
}DecoderResult_t;

#endif