
#include "PDFReader.h"
#include "DecodeStatus.h"
#include "DecodeHints.h"
#include "PDFDetector.h"
#include "PDFCodewordDecoder.h"
#include "PDFScanningDecoder.h"
#include "DecoderResult.h"
#include "Result.h"
#include <stdlib.h>

static const int MODULES_IN_STOP_PATTERN = 18;

#define INT_MAX       2147483647

static int GetMinWidth(ResultPoint_t * p1, ResultPoint_t * p2)
{
	if (p1->m_hasValue == 0 || p2->m_hasValue == 0) 
	{
		// the division prevents an integer overflow (see below). 120 million is still sufficiently large.
		return (int)INT_MAX / MODULES_IN_CODEWORD;
	}

	return abs((int)(p1->m_value.x) - (int)(p2->m_value.x));
}

static int GetMinCodewordWidth(ResultPoint_t * p)
{
	return __min(__min(GetMinWidth(p, p+4), GetMinWidth(p+6, p+2) * MODULES_IN_CODEWORD / MODULES_IN_STOP_PATTERN),
					__min(GetMinWidth(p+1, p+5), GetMinWidth(p+7, p+3) * MODULES_IN_CODEWORD / MODULES_IN_STOP_PATTERN));
}

static int GetMaxWidth(ResultPoint_t * p1, ResultPoint_t * p2)
{
	if (p1->m_hasValue == 0 || p2->m_hasValue == 0) 
	{
		return 0;
	}

	return abs((int)(p1->m_value.x) - (int)(p2->m_value.x));
}

static int GetMaxCodewordWidth(ResultPoint_t * p)
{
	return __max(__max(GetMaxWidth(p, p+4), GetMaxWidth(p+6, p+2) * MODULES_IN_CODEWORD / MODULES_IN_STOP_PATTERN),
					__max(GetMaxWidth(p+1, p+5), GetMaxWidth(p+7, p+3) * MODULES_IN_CODEWORD / MODULES_IN_STOP_PATTERN));
}

DecodeStatus_t DoDecode(GenericLuminanceSource_t * image, unsigned char multiple, result_t * results,unsigned char * characterSet)
{

	printf(" DoDecode: img=%d, multiple=%d, res=%d, str=%d \n", image, multiple, results, characterSet);
	Detector_Result_t detectorResult;
	DecodeStatus_t status;
	status = Detect(image, multiple, &detectorResult);
	if (StatusIsError(status)) 
	{
		return status;
	}

	ResultPoint_t * points = detectorResult.points;
	DecoderResult_t decoderResult;
	result_init(&decoderResult);

	//for (const auto& points : detectorResult.points) 
	//{
		decoderResult = Decode(detectorResult.bits, points[4], points[5], points[6], points[7],
									GetMinCodewordWidth(points), GetMaxCodewordWidth(points), characterSet);

		if (decoderResult.status == NoError) 
		{
			results->_status = decoderResult.status;
			results->_format = PDF417;
			results->_text = decoderResult._text;
			results->_position[0].x = (int)points[0].m_value.x;
			results->_position[0].y = (int)points[0].m_value.y;
			results->_position[1].x = (int)points[2].m_value.x;
			results->_position[1].y = (int)points[2].m_value.y;
			results->_position[2].x = (int)points[3].m_value.x;
			results->_position[2].y = (int)points[3].m_value.y;
			results->_position[3].x = (int)points[1].m_value.x;
			results->_position[3].y = (int)points[1].m_value.y;
			results->_rawBytes = decoderResult._rawBytes;
			results->_numBits = decoderResult._numBits;
			results->_ecLevel = decoderResult._ecLevel;
			//results->_metadata = decoderResult.;
			results->_sai = decoderResult._structuredAppend;
			results->_readerInit = decoderResult._readerInit;

			if (!multiple) 
			{
				return NoError;
			}
		}
		else if (!multiple) 
		{
			return decoderResult.status;
		}

	//}

	return results->_status ? NotFound : NoError;

}

result_t decode(GenericLuminanceSource_t * image,DecodeHints_t * hints)
{
	unsigned int isPure = hints->isPure;
	unsigned int characterSet = hints->characterSet;
	result_t results;
	DecodeStatus_t status;
	
	if (isPure) 
	{
		//result = DecodePure(image, characterSet);
		//if (res.status() != DecodeStatus::ChecksumError)
		return results;
		// This falls through and tries the non-pure code path if we have a checksum error. This approach is
		// currently the best option to deal with 'aliased' input like e.g. 03-aliased.png
	}

	status = DoDecode(image, 0, &results, characterSet);

	if (StatusIsOK(status)) 
	{
		return results;
	}

	return results;
}

