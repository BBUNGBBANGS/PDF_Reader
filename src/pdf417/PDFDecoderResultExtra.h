#ifndef _PDFDECODERRESULTEXTRA_H
#define _PDFDECODERRESULTEXTRA_H

#include <stdint.h>

typedef struct
{
	int _segmentIndex;
	char * _fileId;
	int * _optionalData;
	uint8_t _lastSegment;
	int _segmentCount;
	char * _sender;
	char * _addressee;
	char * _fileName;
	int64_t _fileSize;
	int64_t _timestamp;
	int _checksum;
}DecoderResultExtra_t;

#endif