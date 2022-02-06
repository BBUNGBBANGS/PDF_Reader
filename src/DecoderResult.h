#ifndef _DECODERESULT_H
#define _DECODERESULT_H
#include "decodestatus.h"
#if 0
typedef struct 
{
	DecodeStatus_t status = NoError;
	ByteArray _rawBytes;
	int _numBits = 0;
	std::wstring _text;
	std::wstring _ecLevel;
	int _errorsCorrected = -1;
	int _erasures = -1;
	StructuredAppendInfo _structuredAppend;
	bool _readerInit = false;
	std::shared_ptr<CustomData> _extra;

	DecoderResult(const DecoderResult &) = delete;
	DecoderResult& operator=(const DecoderResult &) = delete;

}DecoderResult_t;
#endif
#endif