#ifndef _RESULT_H
#define _RESULT_H
#include "BarcodeFormat.h"
#include "ByteArray.h"
#include "DecodeStatus.h"
#include "ResultPoint.h"
#include "StructuredAppend.h"
#include <stdint.h>

typedef struct 
{
	int * _contents;/* data */
}ResultMetadata_t;

typedef struct
{
	DecodeStatus_t _status;
	BarcodeFormats_t _format;
	unsigned char * _text;
	PointI_t _position[4];
	uint8_t * _rawBytes;
	int _numBits;
	unsigned char * _ecLevel;
	ResultMetadata_t _metadata;
	StructuredAppendInfo_t _sai;
	uint8_t _readerInit;
}result_t;
extern int Result_Orientation(result_t * result);
#endif
