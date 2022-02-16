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

#include "BarcodeFormat.h"
#include <stdint.h>

#if 0
struct BarcodeFormatName
{
	BarcodeFormat format;
	const char* name;
};
#endif
#if 0
static BarcodeFormatName NAMES[] = {
	//{BarcodeFormat::None, "None"},
	//{BarcodeFormat::Aztec, "Aztec"},
	//{BarcodeFormat::Codabar, "Codabar"},
	//{BarcodeFormat::Code39, "Code39"},
	//{BarcodeFormat::Code93, "Code93"},
	//{BarcodeFormat::Code128, "Code128"},
	//{BarcodeFormat::DataBar, "DataBar"},
	//{BarcodeFormat::DataBarExpanded, "DataBarExpanded"},
	//{BarcodeFormat::DataMatrix, "DataMatrix"},
	//{BarcodeFormat::EAN8, "EAN-8"},
	//{BarcodeFormat::EAN13, "EAN-13"},
	//{BarcodeFormat::ITF, "ITF"},
	//{BarcodeFormat::MaxiCode, "MaxiCode"},
	{BarcodeFormat::PDF417, "PDF417"},
	//{BarcodeFormat::QRCode, "QRCode"},
	//{BarcodeFormat::UPCA, "UPC-A"},
	//{BarcodeFormat::UPCE, "UPC-E"},
	//{BarcodeFormat::OneDCodes, "1D-Codes"},
	//{BarcodeFormat::TwoDCodes, "2D-Codes"},
};
#endif

const char * ToString_Format(BarcodeFormats_t format,char * ret)
{
	for(uint8_t i=0;i<20;i++)
	{
		ret[i] = 0;
	}
	switch(format)
	{
		case NONE :
			sprintf(ret,"NONE");
		break; 
		case Aztec :
			sprintf(ret,"Aztec");
		break; 
		case Codabar :
			sprintf(ret,"Codabar");
		break; 
		case Code39 :
			sprintf(ret,"Code39");
		break; 
		case Code93 :
			sprintf(ret,"Code93");
		break; 
		case Code128 :
			sprintf(ret,"Code128");
		break; 
		case DataBar :
			sprintf(ret,"DataBar");
		break; 
		case DataBarExpanded :
			sprintf(ret,"DataBarExpanded");
		break; 
		case DataMatrix :
			sprintf(ret,"DataMatrix");
		break; 
		case EAN8 :
			sprintf(ret,"EAN8");
		break; 
		case EAN13 :
			sprintf(ret,"EAN13");
		break; 
		case ITF :
			sprintf(ret,"ITF");
		break; 
		case MaxiCode :
			sprintf(ret,"MaxiCode");
		break; 
		case PDF417 :
			sprintf(ret,"PDF417");
		break; 
		case QRCode :
			sprintf(ret,"QRCode");
		break; 
		case UPCA :
			sprintf(ret,"UPCA");
		break; 
		case UPCE :
			sprintf(ret,"UPCE");
		break; 
		default :
			sprintf(ret,"NULL");
		break;
	}
	return ret;
}

const char * ToString_Status(DecodeStatus_t status,char * ret)
{
	for(uint8_t i=0;i<20;i++)
	{
		ret[i] = 0;
	}

	switch(status)
	{
		case NoError :
			sprintf(ret,"NoError");
		break;
		case NotFound :
			sprintf(ret,"NotFound");
		break;
		case FormatError :
			sprintf(ret,"FormatError");
		break;
		case ChecksumError :
			sprintf(ret,"ChecksumError");
		break;
	}
	return ret;
}
#if 0
std::string ToString(BarcodeFormats formats)
{
	if (formats.empty())
		return ToString(BarcodeFormat::None);

	std::string res;
	for (auto f : formats)
		res += ToString(f) + std::string("|");

	return res.substr(0, res.size() - 1);
}
#endif
#if 0
static std::string NormalizeFormatString(std::string str)
{
	std::transform(str.begin(), str.end(), str.begin(), [](char c) { return (char)std::tolower(c); });
	str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return Contains("_-[]", c); }), str.end());
	return str;
}
#endif
#if 0
static BarcodeFormat ParseFormatString(const std::string& str)
{
	auto i = FindIf(NAMES, [str](auto& v) { return NormalizeFormatString(v.name) == str; });
	return i == std::end(NAMES) ? BarcodeFormat::None : i->format;
}
#endif
#if 0
BarcodeFormat BarcodeFormatFromString(const std::string& str)
{
	return ParseFormatString(NormalizeFormatString(str));
}
#endif
#if 0
BarcodeFormats BarcodeFormatsFromString(const std::string& str)
{
	auto normalized = NormalizeFormatString(str);
	std::replace_if(
		normalized.begin(), normalized.end(), [](char c) { return Contains(" ,", c); }, '|');
	std::istringstream input(normalized);
	BarcodeFormats res;
	for (std::string token; std::getline(input, token, '|');) {
		if(!token.empty()) {
			auto bc = ParseFormatString(token);
			if (bc == BarcodeFormat::None)
				throw std::invalid_argument("This is not a valid barcode format: " + token);
			res |= bc;
		}
	}
	return res;
}
#endif

