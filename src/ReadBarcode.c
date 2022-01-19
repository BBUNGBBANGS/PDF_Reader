/*
* Copyright 2019 Axel Waggershauser
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

#include "ReadBarcode.h"
#include "DecodeHints.h"
#include <stdio.h>

DecodeHints_t DecodeHints;

int test_val;

static void DecodeHints_Init(void);

unsigned int * ReadBarcode(unsigned char *iv, int width,int height, unsigned int ImageFormat)
{
	DecodeHints_t hints;
    unsigned int * ret ;

	DecodeHints_Init();

	ret = &test_val;
    printf("Result ReadBarcode : iv= %#x", &iv);
	printf(" hints= %#x\n",&hints);

    printf("address : %#x , Width : %d , height : %d , format :  %#x \n",iv,width,height,ImageFormat);
	//GenericLuminanceSource(0, 0, width, height, const void* bytes, int rowBytes, int pixelBytes, int redIndex, int greenIndex, int blueIndex, void*);
    #if 0
	return ReadBarcode(
			{
				0,
				0,
				iv._width,
				iv._height,
				iv._data,
				iv._rowStride,
				iv._pixStride,
				RedIndex(iv._format),
				GreenIndex(iv._format),
				BlueIndex(iv._format),
				nullptr
			},
			hints);
    #endif
    return (ret);
}

static void ReadBarcode_Internal(void)
{
	//printf("static Result ReadBarcode : source=",source);
	//printf(" hints=\n",hints);
}

static void DecodeHints_Init(void)
{
	DecodeHints.tryHarder = 1;
	DecodeHints.tryRotate = 1;
	DecodeHints.isPure = 0;
	DecodeHints.returnCodabarStartEnd = 0;
}

