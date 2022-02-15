
#include "PDFReader.h"
#include "DecodeStatus.h"
#include "DecodeHints.h"
#include "PDFDetector.h"
#include "PDFCodewordDecoder.h"
#include "PDFScanningDecoder.h"
#include "DecoderResult.h"
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

DecodeStatus_t DoDecode(GenericLuminanceSource_t * image, unsigned char multiple, unsigned char * results,unsigned char * characterSet)
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
		#if 0
		if (decoderResult.isValid()) 
		{
			auto point = [&](int i) { return points[i].value(); };

			Result result(std::move(decoderResult), {point(0), point(2), point(3), point(1)}, BarcodeFormat::PDF417);
			results.push_back(result);

			if (!multiple) 
			{
				return DecodeStatus::NoError;
			}
		}
		else
		if (!multiple) 
		{
			return decoderResult.errorCode();
		}
	}

	return results.empty() ? DecodeStatus::NotFound : DecodeStatus::NoError;
	#endif
}

unsigned char * decode(GenericLuminanceSource_t * image,DecodeHints_t * hints)
{
	unsigned int isPure = hints->isPure;
	unsigned int characterSet = hints->characterSet;
	unsigned char result = 0;
	unsigned char results = 0;
	DecodeStatus_t status;
	
	if (isPure) 
	{
		//result = DecodePure(image, characterSet);
		//if (res.status() != DecodeStatus::ChecksumError)
		return result;
		// This falls through and tries the non-pure code path if we have a checksum error. This approach is
		// currently the best option to deal with 'aliased' input like e.g. 03-aliased.png
	}

	status = DoDecode(image, 0, results, characterSet);

	if (StatusIsOK(status)) 
	{
		return result;
	}
	return result;
}

