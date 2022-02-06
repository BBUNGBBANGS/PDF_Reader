
#include "PDFReader.h"
#include "DecodeStatus.h"
#include "DecodeHints.h"
#include "PDFDetector.h"

DecodeStatus_t DoDecode(GenericLuminanceSource_t * image, unsigned char multiple, unsigned char * results,unsigned char * characterSet)
{

	printf(" DoDecode: img=%d, multiple=%d, res=%d, str=%d \n", image, multiple, results, characterSet);
	Detector_Result_t * detectorResult;
	DecodeStatus_t status;
	status = Detect(image, multiple, detectorResult);
	if (StatusIsError(status)) 
	{
		return status;
	}

	ResultPoint_t * points;
	#if 0
	for (const auto& points : detectorResult.points) 
	{
		DecoderResult_t decoderResult =
			ScanningDecoder::Decode(*detectorResult.bits, points[4], points[5], points[6], points[7],
									GetMinCodewordWidth(points), GetMaxCodewordWidth(points), characterSet);
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

