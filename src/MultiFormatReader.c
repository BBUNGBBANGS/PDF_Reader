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

#include "MultiFormatReader.h"
#include "pdf417/PDFReader.h"
#include "DecodeStatus.h"
#include "Result.h"

result_t MultiFormatReader(GenericLuminanceSource_t * image,DecodeHints_t * hints) 
{
	result_t result;
	uint8_t readers = 1;
	// If we have only one reader in our list, just return whatever that decoded.
	// This preserves information (e.g. ChecksumError) instead of just returning 'NotFound'.
	if (readers == 1)//readers 사이즈가 1일때 진입
	{
		result = decode(image,hints);
		return result;
	}

	for (uint8_t i=0;i<readers;i++) 
	{
		result = decode(image,hints);
  		if (result._status == NoError)
		{
			return result;
		}
	}

	result._status = NotFound;

	return result;
}