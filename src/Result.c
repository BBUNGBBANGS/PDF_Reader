/*
* Copyright 2016 Nu-book Inc.
* Copyright 2016 ZXing authors
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*      http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*/

#include "Result.h"

#include "DecoderResult.h"
#include "TextDecoder.h"
#include <math.h>

#if 0
Result::Result(std::wstring&& text, Position&& position, BarcodeFormat format, ByteArray&& rawBytes,
			   const bool readerInit)
	: _format(format), _text(std::move(text)), _position(std::move(position)), _rawBytes(std::move(rawBytes)),
	  _readerInit(readerInit)
{
	_numBits = Size(_rawBytes) * 8;
}

Result::Result(const std::string& text, int y, int xStart, int xStop, BarcodeFormat format, ByteArray&& rawBytes,
			   const bool readerInit)
	: Result(TextDecoder::FromLatin1(text), Line(y, xStart, xStop), format, std::move(rawBytes), readerInit)
{}

Result::Result(DecoderResult&& decodeResult, Position&& position, BarcodeFormat format)
	: _status(decodeResult.errorCode()), _format(format), _text(std::move(decodeResult).text()),
	  _position(std::move(position)), _rawBytes(std::move(decodeResult).rawBytes()), _numBits(decodeResult.numBits()),
	  _ecLevel(decodeResult.ecLevel()), _sai(decodeResult.structuredAppend()), _readerInit(decodeResult.readerInit())
{
	// TODO: keep that for one release so people get the deprecation warning with a still intact functionality
	if (isPartOfSequence()) {
		_metadata.put(ResultMetadata::STRUCTURED_APPEND_SEQUENCE, sequenceIndex());
		_metadata.put(ResultMetadata::STRUCTURED_APPEND_CODE_COUNT, sequenceSize());
		if (_format == BarcodeFormat::QRCode)
			_metadata.put(ResultMetadata::STRUCTURED_APPEND_PARITY, std::stoi(sequenceId()));
	}

	// TODO: add type opaque and code specific 'extra data'? (see DecoderResult::extra())
}
#endif	
static PointF_t normalized(PointT_t * centerline)
{
	double length;
	PointF_t result;
	length = sqrt(centerline->x * centerline->x + centerline->y * centerline->y);
	result.x = centerline->x / length;
	result.y = centerline->y / length;
	return result;
}
static double orientation(result_t * result) 
{
	PointT_t centerLine;
	centerLine.x = (double)(result->_position[1].x + result->_position[2].x) - (result->_position[0].x + result->_position[3].x);
	centerLine.y = (double)(result->_position[1].y + result->_position[2].y) - (result->_position[0].y + result->_position[3].y);
	if ((centerLine.x == 0)&&(centerLine.y == 0))
	{
		return 0.0;
	}

	PointF_t centerLineF = normalized(&centerLine);
	return atan2(centerLineF.y, centerLineF.x);
}
int Result_Orientation(result_t * result)
{
	const double std_numbers_pi_v = 3.14159265358979323846; // TODO: c++20 <numbers>
	return lround(orientation(result) * 180 / std_numbers_pi_v);
}

